
#pragma once

#include <cassert>

#include <utttil/perf.hpp>

namespace utttil {
namespace iou {


inline int socket()
{
	int sock;
	sock = ::socket(PF_INET, SOCK_STREAM, 0);
	if (sock == -1) {
		std::cerr << "socket() " << strerror(errno) << std::endl;
		return -1;
	}
	int enable = 1;
	if (::setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
		std::cerr << "setsockopt(SO_REUSEADDR) " << strerror(errno) << std::endl;
		return -1;
	}
	return sock;
}

inline int server_socket_tcp(int port)
{
	int sock = socket();
	if (sock == -1)
		return -1;

	sockaddr_in srv_addr;
	::memset(&srv_addr, 0, sizeof(srv_addr));
	srv_addr.sin_family = AF_INET;
	srv_addr.sin_port = ::htons(port);
	srv_addr.sin_addr.s_addr = ::htonl(INADDR_ANY);
	if (::bind(sock, (const sockaddr *)&srv_addr, sizeof(srv_addr)) < 0) {
		std::cerr << "bind() " << strerror(errno) << std::endl;;
		return -1;
	}
	if (::listen(sock, 1024) < 0) {
		std::cerr << "listen() " << strerror(errno) << std::endl;;
		return -1;
	}
	return sock;
}
inline int client_socket_tcp(const std::string & addr, int port)
{
	int sock = socket();
	if (sock == -1)
		return -1;

	sockaddr_in srv_addr;
	::memset(&srv_addr, 0, sizeof(srv_addr));
	srv_addr.sin_family = AF_INET;
	srv_addr.sin_port = ::htons(port);
	srv_addr.sin_addr.s_addr = ::inet_addr(addr.c_str());
	if (::connect(sock, (const sockaddr *)&srv_addr, sizeof(srv_addr)) < 0) {
		std::cerr << "connect() " << strerror(errno) << std::endl;;
		return -1;
	}
	return sock;
}

enum Action
{
	None   = 0,
	Accept = 1,
	Read   = 2,
	Write  = 3,
	Accept_Deferred = 4,
	Read_Deferred   = 5,
	Write_Deferred  = 6,
};

struct no_msg_t {};

template<typename MsgT=no_msg_t>
struct peer
{
	size_t id;
	int fd;
	io_uring & ring;

	// acceptor
	inline static constexpr size_t accept_inbox_capacity_bits = 8;
	sockaddr_in accept_client_addr;
	socklen_t client_addr_len = sizeof(sockaddr_in);
	utttil::ring_buffer<std::shared_ptr<peer>> accept_inbox;

	// writer
	inline static constexpr size_t outbox_capacity_bits = 16;
	inline static constexpr size_t outbox_msg_capacity_bits = 10;
	iovec write_iov[2];
	utttil::ring_buffer<char> outbox;
	utttil::ring_buffer<MsgT> outbox_msg;

	// reader
	inline static constexpr size_t inbox_capacity_bits = 16;
	inline static constexpr size_t inbox_msg_capacity_bits = 10;
	iovec read_iov[2];
	utttil::ring_buffer<char> inbox;
	utttil::ring_buffer<MsgT> inbox_msg;

	peer(size_t id_, int fd_, io_uring & ring_)
		: id(id_)
		, fd(fd_)
		, ring(ring_)
		, accept_inbox(accept_inbox_capacity_bits)
		, outbox      (      outbox_capacity_bits)
		, outbox_msg  (  outbox_msg_capacity_bits)
		, inbox       (       inbox_capacity_bits)
		, inbox_msg   (   inbox_msg_capacity_bits)
	{}

	static std::shared_ptr<peer> connect(size_t id, io_uring & ring, const utttil::url & url)
	{
		int fd = -1;
		if (url.protocol == "tcp")
			fd = client_socket_tcp(url.host.c_str(), std::stoull(url.port));
		if (fd == -1) {
			std::cout << "connect() failed on: " << url << std::endl;
			return nullptr;
		}
		return std::make_shared<peer>(id, fd, ring);
	}
	static std::shared_ptr<peer> bind(size_t id, io_uring & ring, const utttil::url & url)
	{
		int fd = -1;
		if (url.protocol == "tcp")
			fd = server_socket_tcp(std::stoull(url.port));
		if (fd == -1) {
			std::cout << "bind() failed on: " << url << std::endl;
			return nullptr;
		}
		return std::make_shared<peer>(id, fd, ring);
	}

	void signal(Action action)
	{
		io_uring_sqe * sqe = io_uring_get_sqe(&ring);
		while(sqe == nullptr)
		{
			_mm_pause();
			sqe = io_uring_get_sqe(&ring);
		}
		io_uring_prep_nop(sqe);
		size_t user_data = (id << 3) | action;
		io_uring_sqe_set_data(sqe, (void*)user_data);
		io_uring_submit(&ring);
	}

	void accept_loop()
	{
		if (fd == -1)
			return;
		if (accept_inbox.full())
		{
			//std::cout << "send Action::Accept_Deferred" << std::endl;
			return signal(Action::Accept_Deferred);
		}
		io_uring_sqe * sqe = io_uring_get_sqe(&ring);
		while(sqe == nullptr)
		{
			_mm_pause();
			sqe = io_uring_get_sqe(&ring);
		}
		io_uring_prep_accept(sqe, fd, (sockaddr*) &accept_client_addr, &client_addr_len, 0);
		size_t user_data = (id << 3) | Action::Accept;
		io_uring_sqe_set_data(sqe, (void*)user_data);
		io_uring_submit(&ring);
		return;
	}
	void write_loop()
	{
		//std::cout << __func__ << std::endl;
		if (fd == -1) {
			std::cout << __func__ << " stopped fd == -1" << std::endl;
			return;
		}
		if (outbox.empty()) {
			pack();
			//std::cout << __func__ << " deferred outbox empty" << std::endl;
			return signal(Action::Write_Deferred);
		}
		std::tie(write_iov[0].iov_base, write_iov[0].iov_len) = outbox.front_stretch();
		std::tie(write_iov[1].iov_base, write_iov[1].iov_len) = outbox.front_stretch_2();
		//std::cout << "sending write signal for " << write_iov[0].iov_len << std::endl;

		struct io_uring_sqe *sqe = io_uring_get_sqe(&ring);
		while(sqe == nullptr)
		{
			_mm_pause();
			sqe = io_uring_get_sqe(&ring);
		}
		io_uring_prep_writev(sqe, fd, &write_iov[0], 1 + (write_iov[1].iov_len > 0), 0);
		size_t user_data = (id << 3) | Action::Write;
		io_uring_sqe_set_data(sqe, (void*)user_data);
		io_uring_submit(&ring);

		pack();
	}
	void read_loop()
	{
		//std::cout << __func__ << std::endl;
		if (fd == -1)
			return;
		if (inbox.full())
		{
			unpack();
			//std::cout << "send Action::Read_Deferred" << std::endl;
			return signal(Action::Read_Deferred);
		}
		std::tie(read_iov[0].iov_base, read_iov[0].iov_len) = inbox.back_stretch();
		std::tie(read_iov[1].iov_base, read_iov[1].iov_len) = inbox.back_stretch_2();
		io_uring_sqe * sqe = io_uring_get_sqe(&ring);
		while(sqe == nullptr)
		{
			_mm_pause();
			sqe = io_uring_get_sqe(&ring);
		}
		io_uring_prep_readv(sqe, fd, &read_iov[0], 1 + (read_iov[1].iov_len > 0), 0);
		size_t user_data = (id << 3) | Action::Read;
		io_uring_sqe_set_data(sqe, (void*)user_data);
		io_uring_submit(&ring);

		unpack();
	}

	std::shared_ptr<peer> accepted(size_t id, int fd)
	{
		auto new_peer_sptr = std::make_shared<peer>(id, fd, ring);;
		accept_inbox.push_back(new_peer_sptr);
		accept_loop();
		return new_peer_sptr;
	}
	void sent(size_t count)
	{
		//std::cout << __func__ << count << std::endl;
		//std::cout << "sent() outbox size: " << outbox.size() << std::endl;
		assert(count <= outbox.size());
		outbox.advance_front(count);
		write_loop();
	}
	void rcvd(size_t count)
	{
		//std::cout << __func__ << count << std::endl;
		assert(count <= inbox.free_size());
		inbox.advance_back(count);
		read_loop();
	}

	void pack()
	{
		while ( ! outbox_msg.empty())
		{
			MsgT & msg = outbox_msg.front();

			auto size_preview_serializer = utttil::srlz::to_binary(utttil::srlz::device::null_writer());
			size_preview_serializer << msg;
			size_t msg_size = size_preview_serializer.write.size();
			size_preview_serializer << msg_size; // add size field
			size_t total_size = size_preview_serializer.write.size();

			if (outbox.free_size() < total_size) {
				return;
			}
			try {
				auto stretch1 = outbox.back_stretch();
				auto stretch2 = outbox.back_stretch_2();
				auto s = utttil::srlz::to_binary(utttil::srlz::device::ring_buffer_writer(stretch1, stretch2, total_size));
				s << msg_size;
				s << msg;
				outbox.advance_back(s.write.size());
				outbox_msg.pop_front();
			} catch (utttil::srlz::device::stream_end_exception &) {
				std::cout << "send() stream_end_exception" << std::endl;
				return;
			}
		}
	}
	void unpack()
	{
		while ( ! inbox_msg.full())
		{
			auto deserializer = utttil::srlz::from_binary(utttil::srlz::device::ring_buffer_reader(inbox, inbox.size()));
			size_t msg_size;
			size_t total_size;
			try {
				deserializer >> msg_size;
			} catch (utttil::srlz::device::stream_end_exception &) {
				break;
			}
			total_size = msg_size + deserializer.read.size();
			if (total_size > inbox.size()) {
				break;
			}
			MsgT & msg = inbox_msg.back();
			try {
				deserializer >> msg;
			} catch (utttil::srlz::device::stream_end_exception &) {
				std::cout << ">> failed" << std::endl;
				break;
			}
			assert(deserializer.read.size() == total_size);
			inbox_msg.advance_back();
			inbox.advance_front(deserializer.read.size());
		}
	}


	void async_write(const char * data, size_t len)
	{
		while(len > 0)
		{
			while (outbox.full())
			{
				if (fd == -1)
					return;
				_mm_pause();
			}
			auto stretch = outbox.back_stretch();
			size_t len_to_write = std::min(len, std::get<1>(stretch));
			memcpy(std::get<0>(stretch), data, len_to_write);
			outbox.advance_back(len_to_write);

			data += len_to_write;
			len  -= len_to_write;
		}
		//std::cout << "async_write: " << data.size() << std::endl;
	}
	void async_send(const MsgT & msg)
	{
		outbox_msg.push_back(msg);
	}
	void async_send(MsgT && msg)
	{
		outbox_msg.push_back(std::move(msg));
	}

};

template<>
inline void peer<no_msg_t>::pack() {}
template<>
inline void peer<no_msg_t>::unpack() {}

}} // namespace
