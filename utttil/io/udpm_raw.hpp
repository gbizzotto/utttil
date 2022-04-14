
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

inline int socket_udp()
{
	int sock = ::socket(PF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0);
	if (sock == -1) {
		std::cerr << "socket() " << ::strerror(errno) << std::endl;
		return -1;
	}
	int enable = 1;
	if (::setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
		std::cerr << "setsockopt(SO_REUSEADDR) " << ::strerror(errno) << std::endl;
		return -1;
	}
	return sock;
}
inline int server_socket_udpm()
{
	return socket_udp();
}
inline int client_socket_udpm(const std::string & addr, int port)
{
	int sock = ::socket(PF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0);
	if (sock == -1)
		return -1;

	sockaddr_in localSock = {};    // initialize to all zeroes
	localSock.sin_family      = AF_INET;
	localSock.sin_port        = htons(port);
	localSock.sin_addr.s_addr = INADDR_ANY;
	// Note from manpage that bind returns 0 on success
	if (0 != bind(sock, (sockaddr*)&localSock, sizeof(localSock)))
	{
		std::cout << "udpm client bind failed: " << strerror(errno) << std::endl;
		return -1;
	}

	// Join the multicast group on the local interface.  Note that this
	//    IP_ADD_MEMBERSHIP option must be called for each local interface over
	//    which the multicast datagrams are to be received.
	ip_mreq group = {};    // initialize to all zeroes
	group.imr_multiaddr.s_addr = inet_addr(addr.c_str());
	group.imr_interface.s_addr = inet_addr("0.0.0.0");
	if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&group, sizeof(group)) < 0)
	{
		std::cout << "udpm setsockopt failed: " << strerror(errno) << std::endl;
		return -1;
	}

	return sock;
}

template<typename DataT=int>
struct udpm_client_raw : peer_raw<DataT>
{
	utttil::ring_buffer<char> inbox;

	udpm_client_raw(const utttil::url & url)
		: peer_raw<DataT>(client_socket_udpm(url.host.c_str(), std::stoull(url.port)))
		, inbox(peer_raw<DataT>::inbox_capacity_bits)
	{}

	bool does_accept() override { return false; }
	bool does_read  () override { return true ; }
	bool does_write () override { return false; }

	utttil::ring_buffer<char> * get_inbox () override { return & inbox ; }
	
	int read() override
	{
		if (inbox.full())
			return 0;
		auto stretch = inbox.back_stretch();
		int count = ::read(this->fd, std::get<0>(stretch), std::get<1>(stretch));
		if (count > 0) {
			inbox.advance_back(count);
		} else if (count < 0 && errno != 0 && errno != EAGAIN) {
			std::cout << "read() good = false because of errno: " << errno << " - " << strerror(errno) << std::endl;
			this->good_ = false;
		}
		return count;
	}
};

template<typename DataT=int>
struct udpm_server_raw : peer_raw<DataT>
{
	// writer
	sockaddr_in sendto_addr;
	sockaddr_in *sendto_addr_ptr;
	size_t sendto_addr_len;

	utttil::ring_buffer<char> outbox;

	udpm_server_raw(const utttil::url & url)
		: peer_raw<DataT>(server_socket_udpm())
		, sendto_addr_ptr(nullptr)
		, sendto_addr_len(0)
		, outbox(peer_raw<DataT>::outbox_capacity_bits)
	{
	    memset(&sendto_addr, 0, sizeof(sendto_addr));
	    sendto_addr.sin_family = AF_INET;
	    sendto_addr.sin_addr.s_addr = inet_addr(url.host.c_str());
	    sendto_addr.sin_port = htons(std::stol(url.port));
	    sendto_addr_ptr = &sendto_addr;
	    sendto_addr_len = sizeof(sendto_addr);
	}

	bool does_accept() override { return false; }
	bool does_read  () override { return false; }
	bool does_write () override { return true ; }

	int write() override
	{
		if (outbox.empty())
			return 0;
		auto stretch = outbox.front_stretch();
		std::get<1>(stretch) = std::min(1400ul, std::get<1>(stretch));
		int count = ::sendto(this->fd, std::get<0>(stretch), std::get<1>(stretch), 0, (sockaddr*)sendto_addr_ptr, sendto_addr_len);
		if (count > 0) {
			outbox.advance_front(count);
		} else if (count < 0 && errno != 0 && errno != EAGAIN) {
			std::cout << "sendto() good = false because of errno: " << errno << " - " << strerror(errno) << std::endl;
			this->good_ = false;
		}
		return count;
	}
};

}} // namespace
