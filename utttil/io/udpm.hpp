
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


template<typename MsgIn=no_msg_t, typename MsgOut=no_msg_t>
struct udpm_client : peer<MsgIn,MsgOut>
{
	// reader
	char   recv_buffer[1400];
	int    recv_size;
	utttil::ring_buffer<MsgIn> inbox_msg;

	udpm_client(const utttil::url & url)
		: peer<MsgIn,MsgOut>(client_socket_udpm(url.host.c_str(), std::stoull(url.port)))
		, recv_size(0)
		, inbox_msg(peer<MsgIn,MsgOut>::inbox_msg_capacity_bits)
	{}

	bool does_accept() override { return false; }
	bool does_read  () override { return true ; }
	bool does_write () override { return false; }

	utttil::ring_buffer<MsgIn> * get_inbox_msg() override { return &inbox_msg; }

	size_t read() override
	{
		recv_size = ::read(this->fd, recv_buffer, sizeof(recv_buffer));
		if (recv_size > 0) {
			unpack();
		} else if (recv_size < 0 && errno != 0 && errno != EAGAIN) {
			std::cout << "read() good = false because of errno: " << errno << " - " << strerror(errno) << std::endl;
			this->good_ = false;
		}
		return recv_size;
	}
	void unpack() override
	{
		auto deserializer = utttil::srlz::from_binary(utttil::srlz::device::ptr_reader(recv_buffer));
		while ( ! inbox_msg.full())
		{
			size_t msg_size;
			size_t total_size;
			try {
				deserializer >> msg_size;
			} catch (utttil::srlz::device::stream_end_exception &) {
				break;
			}
			total_size = msg_size + deserializer.read.size();
			if (total_size > (size_t)recv_size) {
				break;
			}
			MsgIn & msg = inbox_msg.back();
			try {
				deserializer >> msg;
			} catch (utttil::srlz::device::stream_end_exception &) {
				std::cout << ">> failed" << std::endl;
				break;
			}
			inbox_msg.advance_back(1);
		}
		recv_size = 0;
	}
};

template<typename MsgIn=no_msg_t, typename MsgOut=no_msg_t>
struct udpm_server : peer<MsgIn,MsgOut>
{
	// writer
	sockaddr_in sendto_addr;
	sockaddr_in *sendto_addr_ptr;
	size_t sendto_addr_len;

	char   send_buffer[1400];
	int    send_size;
	utttil::ring_buffer<MsgOut> outbox_msg;

	udpm_server(const utttil::url & url)
		: peer<MsgIn,MsgOut>(server_socket_udpm())
		, sendto_addr_ptr(nullptr)
		, sendto_addr_len(0)
		, send_size(0)
		, outbox_msg(peer<MsgIn,MsgOut>::outbox_msg_capacity_bits)
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

	utttil::ring_buffer<MsgOut> * get_outbox_msg() override { return &outbox_msg; }

	size_t write() override
	{
		//print_outbox();
		pack();
		if (send_size == 0)
			return 0;
		//print_outbox();
		//std::cout << "Printint from outbox: " << std::hex;
		//for(const char *ptr = std::get<0>(stretch), *end=ptr+std::get<1>(stretch) ; ptr<end ; ptr++)
		//	std::cout << (unsigned int)*ptr << ' ';
		//std::cout << std::endl << std::dec;
		int count = ::sendto(this->fd, send_buffer, send_size, 0, (sockaddr*)sendto_addr_ptr, sendto_addr_len);
		if (count > 0) {
			//std::cout << "wrote " << count << std::endl;
			assert(count == send_size);
			send_size = 0;
			//print_outbox();
		} else if (count < 0 && errno != 0 && errno != EAGAIN) {
			std::cout << "sendto() good = false because of errno: " << errno << " - " << strerror(errno) << std::endl;
			this->good_ = false;
		}
		return count;
	}

	void pack() override
	{
		if (send_size != 0)
			return;
		while ( ! outbox_msg.empty())
		{
			MsgOut & msg = outbox_msg.front();

			auto size_preview_serializer = utttil::srlz::to_binary(utttil::srlz::device::null_writer());
			size_preview_serializer << msg;
			size_t msg_size = size_preview_serializer.write.size();
			size_preview_serializer << msg_size; // add size field
			size_t total_size = size_preview_serializer.write.size();

			if (sizeof(send_buffer)-send_size < total_size) {
				return;
			}
			try {
				auto s = utttil::srlz::to_binary(utttil::srlz::device::ptr_writer(&send_buffer[send_size]));
				s << msg_size;
				s << msg;
				send_size += s.write.size();
				outbox_msg.pop_front();
			} catch (utttil::srlz::device::stream_end_exception &) {
				std::cout << "send() stream_end_exception" << std::endl;
				return;
			}
		}
	}

	void async_send(const MsgOut & msg) override
	{
		outbox_msg.push_back(msg);
	}
	void async_send(MsgOut && msg) override
	{
		outbox_msg.push_back(std::move(msg));
	}
};

}} // namespace
