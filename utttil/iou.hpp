
#pragma once

#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>

#include <chrono>
#include <iostream>
#include <vector>
#include <cstring>
#include <optional>
#include <thread>
#include <atomic>
#include <functional>
#include <memory>

#include <liburing.h>

#include <utttil/url.hpp>
#include <utttil/ring_buffer.hpp>
#include <utttil/srlz.hpp>

namespace utttil {
namespace iou {

enum Action
{
	None   = 0,
	Accept = 1,
	Read   = 2,
	Write  = 3,
	Pack   = 4,
};

struct peer
{
	io_uring & ring;
	int file = 0;
    sockaddr_in client_addr;

	utttil::ring_buffer<std::vector<char>>  inbox = utttil::ring_buffer<std::vector<char>>(1024);
	utttil::ring_buffer<std::vector<char>> outbox = utttil::ring_buffer<std::vector<char>>(1024);
    iovec read_iov[1];
    iovec write_iov[1];
    size_t size_to_read = 16;

	peer * accepted = nullptr;
	inline static socklen_t client_addr_len = sizeof(accepted->client_addr);

	std::function<void (peer * p)> on_accept;
	std::function<void (peer * p)> on_data;
	std::function<void (peer * p)> on_close;

	peer(io_uring & ring_, int file_)
		: ring(ring_)
		, file(file_)
	{}

	void close()
	{
		// TODO use io_uring_prep_close ?
		::close(file);
	}

	void post_write(std::vector<char> && data)
	{
		outbox.emplace_back(std::move(data));
	    write_iov[0].iov_base = outbox.back().data();
	    write_iov[0].iov_len = outbox.back().size();

		struct io_uring_sqe *sqe = io_uring_get_sqe(&ring);
		io_uring_prep_writev(sqe, file, &write_iov[0], 1, 0);
		io_uring_sqe_set_data(sqe, (void*)((ptrdiff_t)this | Action::Write));
		io_uring_submit(&ring);
	}

	void post_read()
	{
		inbox.emplace_back();
		inbox.back().resize(size_to_read);
	    read_iov[0].iov_base = inbox.back().data();
	    read_iov[0].iov_len = inbox.back().size();

	    io_uring_sqe * sqe = io_uring_get_sqe(&ring);
	    io_uring_prep_readv(sqe, file, &read_iov[0], 1, 0);
	    io_uring_sqe_set_data(sqe, (void*)((ptrdiff_t)this | Action::Read));
	    io_uring_submit(&ring);
	}

	void post_accept()
	{
		accepted = new peer{ring, 0};

		io_uring_sqe * sqe = io_uring_get_sqe(&ring);
	    io_uring_prep_accept(sqe, file, (struct sockaddr*) &accepted->client_addr, &client_addr_len, 0);
	    io_uring_sqe_set_data(sqe, (void*)((ptrdiff_t)this | Action::Accept));
	    io_uring_submit(&ring);
	}

	virtual void   pack() {};
	virtual void unpack() {};
};

template<typename InMsg, typename OutMsg>
struct peer_srlz : peer
{
	utttil::ring_buffer<std::unique_ptr< InMsg>>  inbox_msg = utttil::ring_buffer<std::unique_ptr< InMsg>>(1024);
	utttil::ring_buffer<std::unique_ptr<OutMsg>> outbox_msg = utttil::ring_buffer<std::unique_ptr<OutMsg>>(1024);

	std::function<void (peer_srlz * p)> on_message;

	peer_srlz(io_uring & ring_, int file_)
		: peer(ring_, file_)
	{}

	void post_send(std::unique_ptr<OutMsg> && msg)
	{
		outbox_msg.push_back(std::move(msg));

	    io_uring_sqe * sqe = io_uring_get_sqe(&ring);
	    io_uring_prep_nop(sqe);
	    io_uring_sqe_set_data(sqe, (void*)((ptrdiff_t)this | Action::Pack));
	    io_uring_submit(&ring);	
	}
	void pack() override
	{
		if (outbox_msg.empty())
			return;
		std::vector<char> v;
		v.reserve(sizeof(typename decltype(outbox_msg)::value_type));
		auto s = utttil::srlz::to_binary(utttil::srlz::device::back_inserter(v));
		s << *outbox_msg.front();
		outbox_msg.pop_front();
		post_write(std::move(v));
	}
	void unpack() override
	{
	}
};

struct context
{
	io_uring ring;
	std::thread t;
	std::atomic_bool go_on = true;

	context()
	{
	    io_uring_params params;
	    memset(&params, 0, sizeof(params));
	    //params.flags |= IORING_SETUP_SQPOLL;
	    params.sq_thread_idle = 2000;

	    int queue_depth = 16384;
	    io_uring_queue_init_params(queue_depth, &ring, &params);
	}
	~context()
	{
		stop();
		io_uring_queue_exit(&ring);
	}

	void run()
	{
	    t = std::thread([&](){ this->loop(); });
	}
	void stop()
	{
		go_on = false;
		if (t.joinable())
			t.join();
	}

	int socket()
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

	int server_socket(int port)
	{
		int sock = this->socket();
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

	int client_socket(const std::string & addr, int port)
	{
		int sock = this->socket();
		if (sock == -1) {
			return -1;
		}

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

	std::unique_ptr<peer> bind(utttil::url url)
	{
		if (url.protocol != "tcp")
			return {};
		
		int sock = this->server_socket(std::stoull(url.port));
		if (sock == -1)
			return {};

	    std::unique_ptr<peer> peer_sptr(new peer{ring, sock});
	    peer_sptr->post_accept();
	    return peer_sptr;
	}

	std::unique_ptr<peer> connect(const utttil::url url)
	{
		if (url.protocol != "tcp")
			return {};

		int sock = this->client_socket(url.host, std::stoull(url.port));
		if (sock == -1) {
			return {};
		}

	    std::unique_ptr<peer> peer_sptr(new peer{ring, sock});
	    peer_sptr->post_read();
	    return peer_sptr;
	}

	template<typename InMsg, typename OutMsg>
	std::unique_ptr<peer_srlz<InMsg,OutMsg>> bind_srlz(const utttil::url url)
	{
		if (url.protocol != "tcp")
			return {};

		int sock = this->server_socket(std::stoull(url.port));
		if (sock == -1)
			return {};

	    std::unique_ptr<peer_srlz<InMsg,OutMsg>> peer_sptr(new peer_srlz<InMsg,OutMsg>{ring, sock});
	    //peer_sptr->on_data = [](peer * p){ ((peer_srlz<InMsg,OutMsg>*)p)->unpack(); };
	    peer_sptr->post_accept();
	    return peer_sptr;
	}

	template<typename InMsg, typename OutMsg>
	std::unique_ptr<peer_srlz<InMsg,OutMsg>> connect_srlz(const utttil::url url)
	{
		if (url.protocol != "tcp")
			return {};

		int sock = this->client_socket(url.host, std::stoull(url.port));
		if (sock == -1) {
			return {};
		}

	    std::unique_ptr<peer_srlz<InMsg,OutMsg>> peer_sptr(new peer_srlz<InMsg,OutMsg>{ring, sock});
	    //peer_sptr->on_data = [](peer * p){ ((peer_srlz<InMsg,OutMsg>*)p)->unpack(); };
	    peer_sptr->post_read();
	    return peer_sptr;
	}

	void loop()
	{
	    io_uring_cqe *cqe;
	    sockaddr_in client_addr;
	    socklen_t client_addr_len = sizeof(client_addr);

    	__kernel_timespec timeout;
    	timeout.tv_sec = 0;
    	timeout.tv_nsec = 100000000;

	    while (go_on)
	    {
	        int ret = io_uring_wait_cqe_timeout(&ring, &cqe, &timeout);
	        if (ret == -ETIME)
	        	continue;
	        if (ret < 0) {
	        	std::cerr << "io_uring_wait_cqe_timeout failed: " << errno << " " << strerror(-ret) << std::endl;
	        	continue;
	        }
	        Action action = static_cast<Action>(((ptrdiff_t)cqe->user_data) & 0xF);
	        peer * p = (peer*) (cqe->user_data ^ action); // zero bits 0, 1, 2, 3
	        if (cqe->res < 0) {
	            std::cerr << "Async request failed: " << strerror(-cqe->res) << std::endl;
	            if (p->on_close)
	            	p->on_close(p);
	            p->close();
	            return;
	        }
	        switch (action)
	        {
	            case Action::None:
	            	break;
	            case Action::Accept:
	            {
	            	peer * new_p = p->accepted;
	            	new_p->file = cqe->res;
	            	p->post_accept();
	            	if (p->on_accept)
	            		p->on_accept(new_p);
	            	if (p->on_data)
	            		new_p->on_data = p->on_data;
	            	if (p->on_close)
	            		new_p->on_close = p->on_close;
	                new_p->post_read();
	                break;
	            }
	            case Action::Read:
	            	if (cqe->res == 0)
	            	{
		            	if (p->on_close)
		            		p->on_close(p);
		            	break;
	            	}
	            	std::cout << "read " << cqe->res << std::endl;
	            	if (cqe->res == p->inbox.back().size() && p->size_to_read < 1024*1024) // buffer full
	            		p->size_to_read *= 2;
	            	else if (cqe->res < p->inbox.back().size() / 2 && p->size_to_read > 16) // buffer mostly empty
	            	{
	            		p->size_to_read /= 2;
	                	p->inbox.back().resize(cqe->res);
	                }
	                p->post_read();
	                if (p->on_data)
	                	p->on_data(p);
	                p->unpack();
	                break;
	            case Action::Write:
	            	p->outbox.pop_front();
	            	break;
	            case Action::Pack:
	            	p->pack();
	            	break;
	            default:
	            	std::cerr << "Invalid action" << std::endl;
	            	break;
	        }
	        /* Mark this request as processed */
	        io_uring_cqe_seen(&ring, cqe);
	    }
	}
};

}} // namespace
