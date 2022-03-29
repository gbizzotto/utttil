
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
#include <string>

#include <utttil/ring_buffer.hpp>
#include <utttil/url.hpp>
#include <utttil/srlz.hpp>

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


inline int socket_udp()
{
	int sock = ::socket(PF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0);
	if (sock == -1) {
		std::cerr << "socket() " << ::strerror(errno) << std::endl;
		return -1;
	}
	//if (set_non_blocking(sock) < 0)
	//	return -1;
	int enable = 1;
	if (::setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
		std::cerr << "setsockopt(SO_REUSEADDR) " << ::strerror(errno) << std::endl;
		return -1;
	}
	return sock;
}
inline int server_socket_udpm()
{
	int sock = socket_udp();
	if (sock == -1)
		return -1;
	return sock;
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

struct no_msg_t {};

template<typename MsgT=no_msg_t>
struct peer
{
	int fd;
	bool good_;

	inline static constexpr size_t accept_inbox_capacity_bits = 8;
	inline static constexpr size_t outbox_capacity_bits = 16;
	inline static constexpr size_t outbox_msg_capacity_bits = 10;
	inline static constexpr size_t inbox_capacity_bits = 16;
	inline static constexpr size_t inbox_msg_capacity_bits = 10;

	peer(int fd_)
		: fd(fd_)
		, good_(true)
	{}

	virtual bool does_accept() { return false; }
	virtual bool does_read  () { return false; }
	virtual bool does_write () { return false; }

	virtual void close()
	{
		if (fd != -1)
		{
			::close(fd);
			fd = -1;
		}
	}
	virtual bool good() const { return (fd != -1) & good_; }

	virtual std::shared_ptr<peer<MsgT>> accept()  { assert(false); return nullptr; };
	virtual size_t write()                        { assert(false); return 0; }
	virtual size_t read ()                        { assert(false); return 0; }
	virtual void   pack()                         { assert(false); }
	virtual void unpack()                         { assert(false); }
	virtual void async_write(const char*, size_t) { assert(false); }
	virtual void async_send(const MsgT  &)        { assert(false); }
	virtual void async_send(      MsgT &&)        { assert(false); }

	virtual utttil::ring_buffer<std::shared_ptr<peer<MsgT>>, peer<MsgT>::accept_inbox_capacity_bits> * get_accept_inbox() { return nullptr; }
	virtual utttil::ring_buffer<char                       , peer<MsgT>::      outbox_capacity_bits> * get_outbox      () { return nullptr; }
	virtual utttil::ring_buffer<MsgT                       , peer<MsgT>::  outbox_msg_capacity_bits> * get_outbox_msg  () { return nullptr; }
	virtual utttil::ring_buffer<char                       , peer<MsgT>::       inbox_capacity_bits> * get_inbox       () { return nullptr; }
	virtual utttil::ring_buffer<MsgT                       , peer<MsgT>::   inbox_msg_capacity_bits> * get_inbox_msg   () { return nullptr; }
};

template<typename MsgT>
struct tcp_socket : peer<MsgT>
{
	// writer
	sockaddr_in sendto_addr;
	sockaddr_in *sendto_addr_ptr;
	size_t sendto_addr_len;
	iovec write_iov[1];
	utttil::ring_buffer<char, peer<MsgT>::outbox_capacity_bits> outbox;
	utttil::ring_buffer<MsgT, peer<MsgT>::outbox_msg_capacity_bits> outbox_msg;

	// reader
	sockaddr_in recvfrom_addr;
	sockaddr_in *recvfrom_addr_ptr;
	size_t recvfrom_addr_len;
	iovec read_iov[1];
	utttil::ring_buffer<char, peer<MsgT>::inbox_capacity_bits> inbox;
	utttil::ring_buffer<MsgT, peer<MsgT>::inbox_msg_capacity_bits> inbox_msg;

	tcp_socket(int fd_)
		: peer<MsgT>(fd_)
		, sendto_addr_ptr(nullptr)
		, sendto_addr_len(0)
	{}
	tcp_socket(const utttil::url & url)
		: tcp_socket(client_socket_tcp(url.host.c_str(), std::stoull(url.port))) // delegate
	{}

	bool does_accept() override { return false; }
	bool does_read  () override { return true ; }
	bool does_write () override { return true ; }

	utttil::ring_buffer<char, peer<MsgT>::      outbox_capacity_bits> * get_outbox      () override { return &outbox    ; }
	utttil::ring_buffer<MsgT, peer<MsgT>::  outbox_msg_capacity_bits> * get_outbox_msg  () override { return &outbox_msg; }
	utttil::ring_buffer<char, peer<MsgT>::       inbox_capacity_bits> * get_inbox       () override { return &inbox    ; }
	utttil::ring_buffer<MsgT, peer<MsgT>::   inbox_msg_capacity_bits> * get_inbox_msg   () override { return &inbox_msg; }

	size_t write() override
	{
		//print_outbox();
		pack();
		if (outbox.empty())
			return 0;
		//print_outbox();
		auto stretch = outbox.front_stretch();
		std::get<1>(stretch) = std::min(1400ul, std::get<1>(stretch));
		//std::cout << "Printint from outbox: " << std::hex;
		//for(const char *ptr = std::get<0>(stretch), *end=ptr+std::get<1>(stretch) ; ptr<end ; ptr++)
		//	std::cout << (unsigned int)*ptr << ' ';
		//std::cout << std::endl << std::dec;
		int count = ::sendto(this->fd, std::get<0>(stretch), std::get<1>(stretch), 0, (sockaddr*)sendto_addr_ptr, sendto_addr_len);
		if (count > 0) {
			//std::cout << "wrote " << count << std::endl;
			outbox.advance_front(count);
			//print_outbox();
		} else if (count < 0 && errno != 0 && errno != EAGAIN) {
			std::cout << "sendto() good = false because of errno: " << errno << " - " << strerror(errno) << std::endl;
			this->good_ = false;
		}
		return count;
	}
	size_t read() override
	{
		unpack();
		if (inbox.full())
			return 0;
		//print_inbox();
		auto stretch = inbox.back_stretch();
		//std::cout << "read() recv() up to " << std::get<1>(stretch) << " B" << std::endl;
		int count = ::read(this->fd, std::get<0>(stretch), std::get<1>(stretch));
		if (count > 0) {
			//std::cout << "read " << count << std::endl;
			inbox.advance_back(count);
			//print_inbox();
			unpack();
			//print_inbox();
		} else if (count < 0 && errno != 0 && errno != EAGAIN) {
			std::cout << "read() good = false because of errno: " << errno << " - " << strerror(errno) << std::endl;
			this->good_ = false;
		}
		return count;
	}

	void pack() override
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
	void unpack() override
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

	void async_write(const char * data, size_t len) override
	{
		//std::cout << "async_write() outbox size: " << outbox.size() << std::endl;
		
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
		//std::cout << "async_write: " << data.size() << std::endl;
	}
	void async_send(const MsgT & msg) override
	{
		outbox_msg.push_back(msg);
	}
	void async_send(MsgT && msg) override
	{
		outbox_msg.push_back(std::move(msg));
	}
};

template<typename MsgT=no_msg_t>
struct tcp_server : peer<MsgT>
{
	// acceptor
	sockaddr_in accept_client_addr;
	socklen_t client_addr_len = sizeof(sockaddr_in);
	utttil::ring_buffer<std::shared_ptr<peer<MsgT>>, peer<MsgT>::accept_inbox_capacity_bits> accept_inbox;

	tcp_server(const utttil::url & url)
		: peer<MsgT>(server_socket_tcp(std::stoull(url.port)))
	{}

	bool does_accept() override { return true ; }
	bool does_read  () override { return false; }
	bool does_write () override { return false; }

	utttil::ring_buffer<std::shared_ptr<peer<MsgT>>, peer<MsgT>::accept_inbox_capacity_bits> * get_accept_inbox() override { return &accept_inbox; }

	std::shared_ptr<peer<MsgT>> accept() override
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
		auto new_peer_sptr = std::make_shared<tcp_socket<MsgT>>(new_fd);
		accept_inbox.push_back(new_peer_sptr);
		return new_peer_sptr;
	}
};


template<typename MsgT=no_msg_t>
struct udpm_client : peer<MsgT>
{
	// reader
	sockaddr_in recvfrom_addr;
	sockaddr_in *recvfrom_addr_ptr;
	size_t recvfrom_addr_len;
	inline static constexpr size_t inbox_capacity_bits = 16;
	inline static constexpr size_t inbox_msg_capacity_bits = 10;
	iovec read_iov[1];
	utttil::ring_buffer<char, inbox_capacity_bits> inbox;
	utttil::ring_buffer<MsgT, inbox_msg_capacity_bits> inbox_msg;

	udpm_client(const utttil::url & url)
		: peer<MsgT>(client_socket_udpm(url.host.c_str(), std::stoull(url.port)))
	{}

	bool does_accept() override { return false; }
	bool does_read  () override { return true ; }
	bool does_write () override { return false; }

	utttil::ring_buffer<char, peer<MsgT>::       inbox_capacity_bits> * get_inbox       () override { return &inbox    ; }
	utttil::ring_buffer<MsgT, peer<MsgT>::   inbox_msg_capacity_bits> * get_inbox_msg   () override { return &inbox_msg; }

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
	size_t read() override
	{
		unpack();
		if (inbox.full())
			return 0;
		//print_inbox();
		auto stretch = inbox.back_stretch();
		//std::cout << "read() recv() up to " << std::get<1>(stretch) << " B" << std::endl;
		int count = ::read(this->fd, std::get<0>(stretch), std::get<1>(stretch));
		if (count > 0) {
			//std::cout << "read " << count << std::endl;
			inbox.advance_back(count);
			//print_inbox();
			unpack();
			//print_inbox();
		} else if (count < 0 && errno != 0 && errno != EAGAIN) {
			std::cout << "read() good = false because of errno: " << errno << " - " << strerror(errno) << std::endl;
			this->good_ = false;
		}
		return count;
	}	
	void unpack() override
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
};

template<typename MsgT=no_msg_t>
struct udpm_server : peer<MsgT>
{
	// writer
	sockaddr_in sendto_addr;
	sockaddr_in *sendto_addr_ptr;
	size_t sendto_addr_len;
	inline static constexpr size_t outbox_capacity_bits = 16;
	inline static constexpr size_t outbox_msg_capacity_bits = 10;
	iovec write_iov[1];
	utttil::ring_buffer<char, outbox_capacity_bits> outbox;
	utttil::ring_buffer<MsgT, outbox_msg_capacity_bits> outbox_msg;

	udpm_server(const utttil::url & url)
		: peer<MsgT>(server_socket_udpm())
		, sendto_addr_ptr(nullptr)
		, sendto_addr_len(0)
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

	utttil::ring_buffer<char, peer<MsgT>::      outbox_capacity_bits> * get_outbox      () override { return &outbox    ; }
	utttil::ring_buffer<MsgT, peer<MsgT>::  outbox_msg_capacity_bits> * get_outbox_msg  () override { return &outbox_msg; }

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
	size_t write() override
	{
		//print_outbox();
		pack();
		if (outbox.empty())
			return 0;
		//print_outbox();
		auto stretch = outbox.front_stretch();
		std::get<1>(stretch) = std::min(1400ul, std::get<1>(stretch));
		//std::cout << "Printint from outbox: " << std::hex;
		//for(const char *ptr = std::get<0>(stretch), *end=ptr+std::get<1>(stretch) ; ptr<end ; ptr++)
		//	std::cout << (unsigned int)*ptr << ' ';
		//std::cout << std::endl << std::dec;
		int count = ::sendto(this->fd, std::get<0>(stretch), std::get<1>(stretch), 0, (sockaddr*)sendto_addr_ptr, sendto_addr_len);
		if (count > 0) {
			//std::cout << "wrote " << count << std::endl;
			outbox.advance_front(count);
			//print_outbox();
		} else if (count < 0 && errno != 0 && errno != EAGAIN) {
			std::cout << "sendto() good = false because of errno: " << errno << " - " << strerror(errno) << std::endl;
			this->good_ = false;
		}
		return count;
	}

	void pack() override
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

	void async_write(const char * data, size_t len) override
	{
		//std::cout << "async_write() outbox size: " << outbox.size() << std::endl;
		
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
		//std::cout << "async_write: " << data.size() << std::endl;
	}
	void async_send(const MsgT & msg) override
	{
		outbox_msg.push_back(msg);
	}
	void async_send(MsgT && msg) override
	{
		outbox_msg.push_back(std::move(msg));
	}
};


template<typename MsgT=no_msg_t>
struct context
{
	using value_type = MsgT;

	std::thread ta;
	std::thread tr;
	std::thread tw;
	std::atomic_bool go_on = true;
	utttil::ring_buffer<std::shared_ptr<peer<MsgT>>, 8> new_accept_peers;
	utttil::ring_buffer<std::shared_ptr<peer<MsgT>>, 8> new_read_peers;
	utttil::ring_buffer<std::shared_ptr<peer<MsgT>>, 8> new_write_peers;
	size_t next_id = 1;

	~context()
	{
		stop();
	}

	void run()
	{
		start_accept();
		start_read  ();
		start_write ();
	}
	void start_accept() { ta = std::thread([&](){ this->loop_accept(); }); }
	void start_read  () { tr = std::thread([&](){ this->loop_read  (); }); }
	void start_write () { tw = std::thread([&](){ this->loop_write (); }); }
	void stop()
	{
		go_on = false;
		if (ta.joinable())
			ta.join();
		if (tr.joinable())
			tr.join();
		if (tw.joinable())
			tw.join();
	}

	void add(std::shared_ptr<peer<MsgT>> peer_sptr)
	{
		if (peer_sptr->does_accept())
			new_accept_peers.push_back(peer_sptr);
		if (peer_sptr->does_read())
			new_read_peers.push_back(peer_sptr);
		if (peer_sptr->does_write())
			new_write_peers.push_back(peer_sptr);
	}

	std::shared_ptr<peer<MsgT>> bind(const utttil::url url)
	{
		std::shared_ptr<peer<MsgT>> peer_sptr;
		if (url.protocol == "tcp")
			peer_sptr = std::make_shared<tcp_server<MsgT>>(url);
		else if (url.protocol == "udpm")
			peer_sptr = std::make_shared<udpm_server<MsgT>>(url);
		if ( ! peer_sptr || ! peer_sptr->good())
		{
			std::cout << "bind failed" << std::endl;
			return nullptr;
		}
		std::cout << "bound" << std::endl;
		add(peer_sptr);
		return peer_sptr;
	}

	std::shared_ptr<peer<MsgT>> connect(const utttil::url url)
	{
		std::shared_ptr<peer<MsgT>> peer_sptr;
		if (url.protocol == "tcp")
			peer_sptr = std::make_shared<tcp_socket<MsgT>>(url);
		else if (url.protocol == "udpm")
			peer_sptr = std::make_shared<udpm_client<MsgT>>(url);
		if ( ! peer_sptr || ! peer_sptr->good())
		{
			std::cout << "connect failed" << std::endl;
			return nullptr;
		}
		std::cout << "connected" << std::endl;
		add(peer_sptr);
		return peer_sptr;
	}

	void loop_accept()
	{
		std::vector<std::shared_ptr<peer<MsgT>>> accept_peers;
		while(go_on)
		{
			while ( ! new_accept_peers.empty())
			{
				accept_peers.push_back(new_accept_peers.front());
				new_accept_peers.pop_front();
			}
			if (accept_peers.empty())
			{
				_mm_pause();
				continue;
			}
			for(int i=accept_peers.size()-1 ; i>=0 ; i--)
			{
				std::shared_ptr<peer<MsgT>> peer_sptr = accept_peers[i];
				auto new_peer_sptr = peer_sptr->accept();
				if ( ! new_peer_sptr)
					continue;
				else if ( ! peer_sptr->good())
				{
					std::cout << "erase accept peer" << std::endl;
					accept_peers.erase(accept_peers.begin()+i);
				}
				std::cout << "accepted" << std::endl;
				add(new_peer_sptr);
			}
		}
	}
	void loop_read()
	{
		std::vector<std::shared_ptr<peer<MsgT>>> read_peers;
		while(go_on)
		{
			while ( ! new_read_peers.empty())
			{
				read_peers.push_back(new_read_peers.front());
				new_read_peers.pop_front();
			}
			if (read_peers.empty())
			{
				_mm_pause();
				continue;
			}
			for(int i=read_peers.size()-1 ; i>=0 ; i--)
			{
				auto peer_sptr = read_peers[i];
				size_t count = peer_sptr->read();
				if (count < 0)
				{
					if ( ! peer_sptr->good())
					{
						std::cout << "erase read peer" << std::endl;
						read_peers.erase(read_peers.begin()+i); // TODO swap with to end before erasing
					}
					continue;
				}
			}
		}
	}
	void loop_write()
	{
		std::vector<std::shared_ptr<peer<MsgT>>> write_peers;
		while(go_on)
		{
			while ( ! new_write_peers.empty())
			{
				write_peers.push_back(new_write_peers.front());
				new_write_peers.pop_front();
			}
			if (write_peers.empty())
			{
				_mm_pause();
				continue;
			}
			for(int i=write_peers.size()-1 ; i>=0 ; i--)
			{
				//std::cout << "write peer " << i << std::endl;
				std::shared_ptr<peer<MsgT>> peer_sptr = write_peers[i];
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
