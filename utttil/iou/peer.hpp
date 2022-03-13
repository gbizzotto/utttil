
#pragma once

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
inline int server_socket(int port)
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
inline int client_socket(const std::string & addr, int port)
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

using buffer = std::vector<char>;

struct inbox_reader
{
	utttil::ring_buffer<buffer> & inbox;
	typename utttil::ring_buffer<buffer>::iterator it_inbox;
	typename buffer::iterator it_vec;
	inline inbox_reader(utttil::ring_buffer<buffer> & inbox_)
		: inbox(inbox_)
		, it_inbox(inbox.begin())
		, it_vec(it_inbox->begin())
	{}
	inbox_reader(const inbox_reader & other)
		: inbox(other.inbox)
		, it_inbox(other.it_inbox)
		, it_vec  (other.it_vec  )
	{}
	inbox_reader & operator=(const inbox_reader & other)
	{
		it_inbox = other.it_inbox;
		it_vec   = other.it_vec;
		return *this;
	}
	inline const auto & operator()()
	{
		while (it_vec == it_inbox->end())
		{
			++it_inbox;
			if (it_inbox == inbox.end())
				throw utttil::srlz::device::stream_end_exception();
			it_vec = it_inbox->begin();
		}
		//printf("%.02X-\n", *it_vec);
		return *it_vec++;
	}
	inline size_t inbox_size_left()
	{
		auto it = it_inbox;
		size_t result = std::distance(it_vec, it_inbox->end());
		for (++it ; it!=inbox.end() ; ++it)
			result += it->size();
		return result;
	}
	inline void update_inbox()
	{
		while (it_vec == it_inbox->end())
		{
			++it_inbox;
			if (it_inbox == inbox.end())
				break;
			it_vec = it_inbox->begin();
		}		//std::cout << "update_inbox" << std::endl;
		if (it_inbox != inbox.end())
			it_inbox->erase(it_inbox->begin(), it_vec);
		inbox.erase(inbox.begin(), it_inbox);
	}
};


enum Action
{
	None   = 0,
	Accept = 1,
	Read   = 2,
	Write  = 3,
	Pack   = 4,
};

struct no_msg_t {};

template<typename MsgT=no_msg_t>
struct peer
{
	size_t id;
	int fd;
	io_uring & ring;

	// acceptor
	const static size_t default_accept_inbox_size_ = 16;
	sockaddr_in accept_client_addr;
	socklen_t client_addr_len = sizeof(sockaddr_in);
	utttil::ring_buffer<std::shared_ptr<peer>> accept_inbox;

	// writer
	const static size_t default_outbox_size = 16;
	iovec write_iov[1];
	utttil::ring_buffer<buffer> outbox;
	const static size_t default_outbox_msg_size;
	utttil::ring_buffer<MsgT> outbox_msg;

	// reader
	const static size_t default_input_ring_buffer_size = 16;
	const static size_t min_input_chunk_size = 2048;
	const static size_t max_input_chunk_size = 2048;
	iovec read_iov[1];
	utttil::ring_buffer<buffer> inbox;
	size_t input_chunk_size;
	std::vector<no_init<char>> recv_buffer;
	const static size_t default_inbox_msg_size;
	utttil::ring_buffer<MsgT> inbox_msg;

	peer( size_t id_
		, int fd_
		, io_uring & ring_
		, size_t accept_inbox_size_      = default_accept_inbox_size_
		, size_t output_ring_buffer_size = default_outbox_size
		, size_t outbox_msg_size         = default_outbox_msg_size
		, size_t input_ring_buffer_size  = default_input_ring_buffer_size
		, size_t input_chunk_size_       = min_input_chunk_size
		, size_t inbox_msg_size          = default_inbox_msg_size
		)
		: id(id_)
		, fd(fd_)
		, ring(ring_)
		, accept_inbox(accept_inbox_size_)
		, outbox(output_ring_buffer_size)
		, outbox_msg(outbox_msg_size)
		, inbox(input_ring_buffer_size)
		, input_chunk_size(input_chunk_size_)
		, inbox_msg(inbox_msg_size)
	{}

	static std::shared_ptr<peer> connect(size_t id, io_uring & ring, const utttil::url & url)
	{
		int fd = client_socket(url.host.c_str(), std::stoull(url.port));
		if (fd == -1) {
			std::cout << "client_socket() failed" << std::endl;
			return nullptr;
		}
		return std::make_shared<peer>(id, fd, ring);
	}
	static std::shared_ptr<peer> bind(size_t id, io_uring & ring, const utttil::url & url)
	{
		int fd = server_socket(std::stoull(url.port));
		if (fd == -1) {
			std::cout << "client_socket() failed" << std::endl;
			return nullptr;
		}
		return std::make_shared<peer>(id, fd, ring);
	}

	size_t outbox_size()
	{
		size_t result = 0;
		for (auto it=outbox.begin() ; it!=outbox.end() ; ++it)
			result += it->size();
		return result;
	}
	size_t inbox_size()
	{
		size_t result = 0;
		for (auto it=inbox.begin() ; it!=inbox.end() ; ++it)
			result += it->size();
		return result;
	}

	void async_accept()
	{
		if (fd == -1)
			return;
		io_uring_sqe * sqe = io_uring_get_sqe(&ring);
		if (sqe == nullptr)
			return;
		io_uring_prep_accept(sqe, fd, (sockaddr*) &accept_client_addr, &client_addr_len, 0);
		size_t user_data = (id << 3) | Action::Accept;
		io_uring_sqe_set_data(sqe, (void*)user_data);
		io_uring_submit(&ring);
	}
	void async_write(buffer && data)
	{
		while(outbox.full())
			_mm_pause();
		outbox.emplace_back(std::move(data));
		write_iov[0].iov_base = outbox.back().data();
		write_iov[0].iov_len = outbox.back().size();

		struct io_uring_sqe *sqe = io_uring_get_sqe(&ring);
		io_uring_prep_writev(sqe, fd, &write_iov[0], 1, 0);
		size_t user_data = (id << 3) | Action::Write;
		io_uring_sqe_set_data(sqe, (void*)user_data);
		io_uring_submit(&ring);
	}
	void async_read()
	{
		recv_buffer.resize(input_chunk_size);
		read_iov[0].iov_base = recv_buffer.data();
		read_iov[0].iov_len = recv_buffer.size();

		io_uring_sqe * sqe = io_uring_get_sqe(&ring);
		io_uring_prep_readv(sqe, fd, &read_iov[0], 1, 0);
		size_t user_data = (id << 3) | Action::Read;
		io_uring_sqe_set_data(sqe, (void*)user_data);
		io_uring_submit(&ring);
	}

	void pack_signal()
	{
		io_uring_sqe * sqe = io_uring_get_sqe(&ring);
		io_uring_prep_nop(sqe);
		size_t user_data = (id << 3) | Action::Pack;
		io_uring_sqe_set_data(sqe, (void*)user_data);
		io_uring_submit(&ring);
	}

	void async_send(const MsgT & msg)
	{
		outbox_msg.push_back(msg);
		pack_signal(); // serialize later
	}
	void async_send(MsgT && msg)
	{
		outbox_msg.push_back(std::move(msg));
		pack_signal(); // serialize later
	}
	void send(const MsgT & msg)
	{
		buffer v;
		v.reserve(sizeof(msg)+2);
		v.push_back(0);
		v.push_back(0);
		auto s = utttil::srlz::to_binary(utttil::srlz::device::back_inserter(v));
		s << msg;
		size_t size = v.size() - 2;
		v.front() = (size/256) & 0xFF;
		*std::next(v.begin()) = size & 0xFF;
		async_write(std::move(v));
	}

	std::shared_ptr<peer> accepted(size_t id, int fd)
	{
		return std::make_shared<peer>(id, fd, ring);
	}
	void sent(size_t count)
	{
		while (outbox.size() > 0 && count >= outbox.front().size())
		{
			count -= outbox.front().size();
			outbox.pop_front();
		}
		if (count > 0 && count < outbox.front().size())
			outbox.front().erase(outbox.front().begin(), outbox.front().begin()+count);
	}
	void rcvd(size_t count)
	{
		if (count == recv_buffer.size() && input_chunk_size < max_input_chunk_size) // buffer full
			input_chunk_size *= 2;
		else if (count < recv_buffer.size() / 2 && input_chunk_size > min_input_chunk_size) // buffer mostly empty
			input_chunk_size /= 2;
		recv_buffer.resize(count);
		while(inbox.full())
			_mm_pause();
		inbox.push_back(std::move(*(buffer*)&recv_buffer));
		async_read();
		unpack();
	}

	void pack()
	{
		if (outbox_msg.empty())
			return; // nothing to do
		if (outbox.full())
			pack_signal(); // try later
		send(outbox_msg.front());
		outbox_msg.pop_front();
	}

	void unpack()
	{
		//std::cout << "unpack" << std::endl;
		if (inbox.empty()) {
			std::cout << "unpack but inbox is empty" << std::endl;
			return;
		}
		if (inbox_msg.full()) {
			std::cout << "unpack but inbox_msg is full" << std::endl;
			return;
		}

		auto deserializer = utttil::srlz::from_binary(inbox_reader(inbox));
		auto checkpoint(deserializer.read);
		for (;;)
		{
			try {
				if (deserializer.read.inbox_size_left() < 2)
					break;
				size_t msg_size = (deserializer.read() << 8) + deserializer.read();
				if (msg_size == 0) {
					std::cout << "iou msg size is zero, this can't be good..." << std::endl;
					break;
				}
				if (msg_size > deserializer.read.inbox_size_left()) {
					//std::cout << "iou inbox does not contain a full msg" << std::endl;
					break;
				}
				MsgT msg;
				deserializer >> msg;				
				inbox_msg.push_back(std::move(msg)); // blocks

			} catch (utttil::srlz::device::stream_end_exception &) {
				break;
			}
			checkpoint = deserializer.read;
		}
		checkpoint.update_inbox();
	}
};

template<>
void peer<no_msg_t>::pack() {}
template<>
void peer<no_msg_t>::unpack() {}

template<typename T>
const size_t peer<T>::default_outbox_msg_size = 16;
template<>
inline const size_t peer<no_msg_t>::default_outbox_msg_size = 0;

template<typename T>
const size_t peer<T>::default_inbox_msg_size = 16;
template<>
inline const size_t peer<no_msg_t>::default_inbox_msg_size = 0;


}} // namespace
