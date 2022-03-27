
#pragma once

#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include <vector>
#include <memory>
#include <thread>

#include <utttil/ring_buffer.hpp>
#include <utttil/url.hpp>
#include <utttil/srlz.hpp>

namespace utttil {
namespace io {

/*
inline int set_non_blocking(int fd)
{
	int flags = ::fcntl(fd, F_GETFL, 0);
	if (flags == -1)
	{
		::close(fd);
		return -1;
	}
	if (::fcntl(fd, F_SETFD, flags | O_NONBLOCK) != 0)
	{
		std::cerr << "fcntl() " << ::strerror(errno) << std::endl;
		::close(fd);
		return -1;
	}
	return 0;
}
*/

inline int socket_tcp()
{
	int fd;
	fd = ::socket(PF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (fd == -1) {
		std::cerr << "socket() " << ::strerror(errno) << std::endl;
		return -1;
	}
	//if (set_non_blocking(fd) < 0)
	//	return -1;
	int enable = 1;
	if (::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
		std::cerr << "setsockopt(SO_REUSEADDR) " << ::strerror(errno) << std::endl;
		return -1;
	}
	return fd;
}
inline int server_socket_tcp(int port)
{
	int fd = socket_tcp();
	if (fd == -1)
		return -1;

	sockaddr_in srv_addr;
	::memset(&srv_addr, 0, sizeof(srv_addr));
	srv_addr.sin_family = AF_INET;
	srv_addr.sin_port = ::htons(port);
	srv_addr.sin_addr.s_addr = ::htonl(INADDR_ANY);
	if (::bind(fd, (const sockaddr *)&srv_addr, sizeof(srv_addr)) < 0) {
		std::cerr << "bind() " << strerror(errno) << std::endl;;
		return -1;
	}
	if (::listen(fd, 32) < 0) {
		std::cerr << "listen() " << strerror(errno) << std::endl;;
		return -1;
	}
	return fd;
}
bool wait_for_tcp_connect(int fd)
{
	socklen_t optlen;
	int optval;
	for (;;)
	{
		if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &optval, &optlen) == -1)
			std::cout << "getsockopt error" << std::endl;
		if (optval == 0)
			return true;
		if (optval != EINPROGRESS)
		{
			std::cout << "connect error: " << strerror(errno) << std::endl;
			return false;
		}
	}
}
inline int client_socket_tcp(const std::string & addr, int port)
{
	int fd = socket_tcp();
	if (fd == -1)
		return -1;

	sockaddr_in srv_addr;
	::memset(&srv_addr, 0, sizeof(srv_addr));
	srv_addr.sin_family = AF_INET;
	srv_addr.sin_port = ::htons(port);
	srv_addr.sin_addr.s_addr = ::inet_addr(addr.c_str());
	if (::connect(fd, (const sockaddr *)&srv_addr, sizeof(srv_addr)) == 0)
		return fd;
	if (errno != EINPROGRESS)
		return -1;
	if (wait_for_tcp_connect(fd))
		return fd;
	std::cerr << "connect() " << strerror(errno) << std::endl;
		return -1;
}

/*
inline int server_socket_udpm(int port)
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
inline int client_socket_udmp(const std::string & addr, int port)
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
*/
template<typename MsgT>
struct peer
{
	int fd;
	bool good_ = true;

	// acceptor
	inline static constexpr size_t accept_inbox_capacity_bits = 8;
	sockaddr_in accept_client_addr;
	socklen_t client_addr_len = sizeof(sockaddr_in);
	utttil::ring_buffer<std::shared_ptr<peer>, accept_inbox_capacity_bits> accept_inbox;

	// writer
	inline static constexpr size_t outbox_capacity_bits = 16;
	inline static constexpr size_t outbox_msg_capacity_bits = 10;
	iovec write_iov[1];
	utttil::ring_buffer<char, outbox_capacity_bits> outbox;
	utttil::ring_buffer<MsgT, outbox_msg_capacity_bits> outbox_msg;

	// reader
	inline static constexpr size_t inbox_capacity_bits = 16;
	inline static constexpr size_t inbox_msg_capacity_bits = 10;
	iovec read_iov[1];
	utttil::ring_buffer<char, inbox_capacity_bits> inbox;
	utttil::ring_buffer<MsgT, inbox_msg_capacity_bits> inbox_msg;

	utttil::ring_buffer<int,10> written;

	peer(int fd_)
		: fd(fd_)
	{}

	void close()
	{
		if (fd != -1)
		{
			::close(fd);
			fd = -1;
		}
	}

	bool good() const { return good_; }

	static std::shared_ptr<peer> connect(const utttil::url & url)
	{
		int fd = -1;
		if (url.protocol == "tcp")
			fd = client_socket_tcp(url.host.c_str(), std::stoull(url.port));
		//else if (url.protocol == "udpm")
		//	fd = client_socket_udpm(url.host.c_str(), std::stoull(url.port));
		if (fd == -1) {
			std::cout << "client_socket() failed" << std::endl;
			return nullptr;
		}
		return std::make_shared<peer>(fd);
	}
	static std::shared_ptr<peer> bind(const utttil::url & url)
	{
		int fd = -1;
		if (url.protocol == "tcp")
			fd = server_socket_tcp(std::stoull(url.port));
		//else if (url.protocol == "udpm")
		//	fd = server_socket_udpm(std::stoull(url.port));
		if (fd == -1) {
			std::cout << "server_socket() failed" << std::endl;
			return nullptr;
		}
		return std::make_shared<peer>(fd);
	}

	std::shared_ptr<peer<MsgT>> accept()
	{
		int new_fd = ::accept4(fd, (sockaddr*) &accept_client_addr, &client_addr_len, SOCK_NONBLOCK);
		if (new_fd == -1)
		{
			if (errno != 0 && errno != EAGAIN) {
				std::cout << "accept() good = false because of errno: " << errno << std::endl;
				good_ = false;
			}
			return nullptr;
		}
		//if (set_non_blocking(new_fd) < 0)
		//{
		//	::close(new_fd);
		//	return nullptr;
		//}
		auto new_peer_sptr = std::make_shared<peer<MsgT>>(new_fd);
		return new_peer_sptr;
	}
	void print_outbox()
	{
		auto stretch  = outbox.front_stretch();
		auto stretch2 = outbox.front_stretch_2();
		std::cout << "outbox: " << std::hex;
		for(const char *ptr = std::get<0>(stretch), *end=ptr+std::get<1>(stretch) ; ptr<end ; ptr++)
			std::cout << (unsigned int)*ptr << ' ';
		std::cout << std::endl;
		for(const char *ptr = std::get<0>(stretch2), *end=ptr+std::get<1>(stretch2) ; ptr<end ; ptr++)
			std::cout << (unsigned int)*ptr << ' ';
		std::cout << std::endl << std::dec;
	}
	size_t write()
	{
		//print_outbox();
		pack();
		if (outbox.empty())
			return 0;
		//print_outbox();
		auto stretch = outbox.front_stretch();
		//std::cout << "Printint from outbox: " << std::hex;
		//for(const char *ptr = std::get<0>(stretch), *end=ptr+std::get<1>(stretch) ; ptr<end ; ptr++)
		//	std::cout << (unsigned int)*ptr << ' ';
		//std::cout << std::endl << std::dec;
		int count = ::sendto(fd, std::get<0>(stretch), std::get<1>(stretch), 0, nullptr, 0);
		if (count > 0) {
			//std::cout << "wrote " << count << std::endl;
			outbox.advance_front(count);
			//print_outbox();
		} else if (count < 0 && errno != 0 && errno != EAGAIN) {
			std::cout << "write() good = false because of errno: " << errno << std::endl;
			good_ = false;
		}
		return count;
	}
	void print_inbox()
	{
		std::cout << "front_: " << inbox.front_ << std::endl;
		std::cout << "front_ & Mask: " << (inbox.front_ & inbox.Mask) << std::endl;
		std::cout << "back_: " << inbox.back_ << std::endl;
		std::cout << "back_ & Mask: " << (inbox.back_ & inbox.Mask) << std::endl;
		auto stretch  = inbox.front_stretch();
		auto stretch2 = inbox.front_stretch_2();
		std::cout << "inbox: " << std::hex;
		for(const char *ptr = std::get<0>(stretch), *end=ptr+std::get<1>(stretch) ; ptr<end ; ptr++)
			std::cout << (unsigned int)*ptr << ' ';
		std::cout << std::endl;
		for(const char *ptr = std::get<0>(stretch2), *end=ptr+std::get<1>(stretch2) ; ptr<end ; ptr++)
			std::cout << (unsigned int)*ptr << ' ';
		std::cout << std::endl << std::dec;
	}
	size_t read()
	{
		unpack();
		if (inbox.full())
			return 0;
		//print_inbox();
		auto stretch = inbox.back_stretch();
		//std::cout << "read() recv() up to " << std::get<1>(stretch) << " B" << std::endl;
		int count = ::read(fd, std::get<0>(stretch), std::get<1>(stretch));
		if (count > 0) {
			//std::cout << "read " << count << std::endl;
			inbox.advance_back(count);
			//print_inbox();
			unpack();
			//print_inbox();
		} else if (count < 0 && errno != 0 && errno != EAGAIN) {
			std::cout << "read() good = false because of errno: " << errno << std::endl;
			good_ = false;
		}
		return count;
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
			//assert(msg_size == 45);
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
			inbox_msg.advance_back(1);
			inbox.advance_front(deserializer.read.size());
		}
	}

	void async_write(const char * data, size_t len)
	{
		//std::cout << "async_write() outbox size: " << outbox.size() << std::endl;
		
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

struct no_msg_t {};

template<typename MsgT=no_msg_t>
struct context
{
	using value_type = MsgT;

	std::thread ta;
	std::thread tr;
	std::thread tw;
	std::atomic_bool go_on = true;
	std::vector<std::shared_ptr<peer<MsgT>>> accept_peers;
	std::vector<std::shared_ptr<peer<MsgT>>> read_peers;
	std::vector<std::shared_ptr<peer<MsgT>>> write_peers;
	size_t next_id = 1;

	context()
	{
		accept_peers.reserve(100);
		read_peers.reserve(100);
		write_peers.reserve(100);
	}

	~context()
	{
		stop();
	}

	void start_accept() { ta = std::thread([&](){ this->loop_accept(); }); }
	void start_read  () { tr = std::thread([&](){ this->loop_read  (); }); }
	void start_write () { tw = std::thread([&](){ this->loop_write (); }); }
	void stop()
	{
		go_on = false;
		for (auto & psptr : accept_peers)
			psptr->close();
		for (auto & psptr : read_peers)
			psptr->close();
		for (auto & psptr : write_peers)
			psptr->close();
		if (ta.joinable())
			ta.join();
		if (tr.joinable())
			tr.join();
		if (tw.joinable())
			tw.join();
	}

	std::shared_ptr<peer<MsgT>> bind(const utttil::url url)
	{
		std::shared_ptr<peer<MsgT>> peer_sptr = peer<MsgT>::bind(url);
		if ( ! peer_sptr)
		{
			std::cout << "biund failed" << std::endl;
			return nullptr;
		}
		std::cout << "bound" << std::endl;
		accept_peers.push_back(peer_sptr);
		return peer_sptr;
	}

	std::shared_ptr<peer<MsgT>> connect(const utttil::url url)
	{
		std::shared_ptr<peer<MsgT>> peer_sptr = peer<MsgT>::connect(url);
		if ( ! peer_sptr)
			return nullptr;

		read_peers .push_back(peer_sptr);
		write_peers.push_back(peer_sptr);
		return peer_sptr;
	}

	void loop_accept()
	{
		while(go_on)
		{
			if (accept_peers.empty())
			{
				_mm_pause();
				continue;
			}
			for(int i=accept_peers.size()-1 ; i>=0 ; i--)
			{
				std::shared_ptr<peer<MsgT>> & peer_sptr = accept_peers[i];
				auto new_peer_sptr = peer_sptr->accept();
				if ( ! new_peer_sptr)
				{
					if ( ! peer_sptr->good())
					{
						std::cout << "erase accept peer" << std::endl;
						accept_peers.erase(accept_peers.begin()+i);
					}
					continue;
				}
				std::cout << "accepted" << std::endl;
				peer_sptr->accept_inbox.push_back(new_peer_sptr);
				read_peers .push_back(new_peer_sptr);
				write_peers.push_back(new_peer_sptr);
			}
		}
	}
	void loop_read()
	{
		while(go_on)
		{
			if (read_peers.empty())
			{
				_mm_pause();
				continue;
			}
			for(int i=read_peers.size()-1 ; i>=0 ; i--)
			{
				std::shared_ptr<peer<MsgT>> & peer_sptr = read_peers[i];
				size_t count = peer_sptr->read();
				if (count < 0)
				{
					if ( ! peer_sptr->good())
					{
						std::cout << "erase read peer" << std::endl;
						read_peers.erase(read_peers.begin()+i);
					}
					continue;
				}
			}
		}
	}
	void loop_write()
	{
		while(go_on)
		{
			if (write_peers.empty())
			{
				_mm_pause();
				continue;
			}
			for(int i=write_peers.size()-1 ; i>=0 ; i--)
			{
				//std::cout << "write peer " << i << std::endl;
				std::shared_ptr<peer<MsgT>> & peer_sptr = write_peers[i];
				size_t count = peer_sptr->write();
				if (count < 0)
				{
					if ( ! peer_sptr->good())
					{
						std::cout << "erase write peer" << std::endl;
						write_peers.erase(write_peers.begin()+i);
					}
					continue;
				}
			}
		}
	}
};

}} // namespace
