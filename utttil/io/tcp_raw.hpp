
#pragma once

#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include <utttil/ring_buffer.hpp>
#include <utttil/url.hpp>
#include <utttil/srlz.hpp>

#include <utttil/io/peer.hpp>

namespace utttil {
namespace io {

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
	if (set_non_blocking(fd) < 0)
	{
		::close(fd);
		return -1;
	}
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
	if (set_non_blocking(fd) < 0)
	{
		::close(fd);
		return -1;
	}
	return fd;
}

template<typename DataT=int>
struct tcp_socket_raw : peer_raw<DataT>
{
	utttil::ring_buffer<char> outbox;
	utttil::ring_buffer<char>  inbox;

	tcp_socket_raw(int fd_)
		: peer_raw<DataT>(fd_)
		, outbox    (peer_raw<DataT>::outbox_capacity_bits)
		, inbox     (peer_raw<DataT>:: inbox_capacity_bits)
	{}
	tcp_socket_raw(const utttil::url & url)
		: tcp_socket_raw(client_socket_tcp(url.host.c_str(), std::stoull(url.port))) // delegate
	{
		std::cout << "fd: " << this->fd << " url: " << url.to_string() << std::endl;
	}
	tcp_socket_raw(tcp_socket_raw && other)
		: peer_raw<DataT>(std::move(other))
		, outbox(std::move(other.outbox))
		,  inbox(std::move(other. inbox))
	{}

	bool does_accept() override { return false; }
	bool does_read  () override { return true ; }
	bool does_write () override { return true ; }

	utttil::ring_buffer<char> * get_outbox() override { return &outbox ; }
	utttil::ring_buffer<char> * get_inbox () override { return & inbox ; }

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
	
	int write() override
	{
		if (outbox.empty())
			return 0;
		auto stretch = outbox.front_stretch();
		std::get<1>(stretch) = std::min(1400ul, std::get<1>(stretch)); // this seems to be an optimal size for performance
		int count = ::send(this->fd, std::get<0>(stretch), std::get<1>(stretch), 0);
		if (count > 0) {
			outbox.advance_front(count);
		} else if (count < 0 && errno != 0 && errno != EAGAIN) {
			std::cout << "tcp_socket_raw write() good = false because of errno: " << errno << " - " << strerror(errno) << std::endl;
			this->good_ = false;
		}
		return count;
	}
	int read() override
	{
		if (inbox.full())
			return 0;
		auto stretch = inbox.back_stretch();
		int count = ::read(this->fd, std::get<0>(stretch), std::get<1>(stretch));
		if (count > 0) {
			inbox.advance_back(count);
		} else if (count < 0 && errno != 0 && errno != EAGAIN) {
			std::cout << "tcp_socket_raw read() good = false because of errno: " << errno << " - " << strerror(errno) << std::endl;
			this->good_ = false;
			return count;
		}
		return 0;
	}

	void async_write(const char * data, size_t len) override
	{
		while(len > 0)
		{
			while (outbox.full())
			{
				if (this->fd == -1)
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
	}
};

template<typename DataT=int>
struct tcp_server_raw : peer_raw<DataT>
{
	// acceptor
	sockaddr_in accept_client_addr;
	socklen_t client_addr_len = sizeof(sockaddr_in);
	utttil::ring_buffer<std::shared_ptr<peer_raw<DataT>>> accept_inbox;

	tcp_server_raw(const utttil::url & url)
		: peer_raw<DataT>(server_socket_tcp(std::stoull(url.port)))
		, accept_inbox(peer_raw<DataT>::accept_inbox_capacity_bits)
	{
		std::cout << "fd: " << this->fd << " url: " << url.to_string() << std::endl;
	}

	bool does_accept() override { return true ; }
	bool does_read  () override { return false; }
	bool does_write () override { return false; }

	utttil::ring_buffer<std::shared_ptr<peer_raw<DataT>>> * get_accept_inbox() override { return &accept_inbox; }

	std::shared_ptr<peer> accept() override
	{
		int new_fd = ::accept4(this->fd, (sockaddr*) &accept_client_addr, &client_addr_len, SOCK_NONBLOCK);
		if (new_fd == -1)
		{
			if (errno != 0 && errno != EAGAIN) {
				std::cout << "accept() good = false because of errno: " << errno << " - " << strerror(errno) << std::endl;
				this->good_ = false;
			}
			return nullptr;
		}
		auto new_peer_sptr = std::make_shared<tcp_socket_raw<DataT>>(new_fd);
		accept_inbox.push_back(new_peer_sptr);
		return new_peer_sptr;
	}
};

}} // namespace
