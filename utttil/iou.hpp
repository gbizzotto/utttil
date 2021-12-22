
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
#include <iterator>

#include <liburing.h>

#include <utttil/url.hpp>
#include <utttil/ring_buffer.hpp>
#include <utttil/srlz.hpp>
#include <utttil/no_init.hpp>

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

	utttil::ring_buffer<std::vector<no_init<char>>>  inbox = utttil::ring_buffer<std::vector<no_init<char>>>(1024);
	utttil::ring_buffer<std::vector<        char >> outbox = utttil::ring_buffer<std::vector<        char >>(1024);
    iovec read_iov[1];
    iovec write_iov[1];
    size_t size_to_read = 16;
    std::vector<no_init<char>> recv_buffer;

	peer * accepted = nullptr;
	inline static socklen_t client_addr_len = sizeof(accepted->client_addr);

	std::function<void (peer * p)> on_accept;
	std::function<void (peer * p)> on_data;
	std::function<void (peer * p)> on_close;

	peer(io_uring & ring_, int file_)
		: ring(ring_)
		, file(file_)
	{}
	~peer()
	{
		std::cout <<"close"<< std::endl;
		close();
	}

	void close()
	{
		// TODO use io_uring_prep_close ?
		::shutdown(file, SHUT_RDWR);
	}

	void async_write(std::vector<char> && data)
	{
		//std::cout << "write " << data.size() << std::endl;
		//for (char & c : data)
		//	printf("%.02X ", c);
		//printf("\n");

		outbox.emplace_back(std::move(data));
	    write_iov[0].iov_base = outbox.back().data();
	    write_iov[0].iov_len = outbox.back().size();

		struct io_uring_sqe *sqe = io_uring_get_sqe(&ring);
		io_uring_prep_writev(sqe, file, &write_iov[0], 1, 0);
		io_uring_sqe_set_data(sqe, (void*)((ptrdiff_t)this | Action::Write));
		io_uring_submit(&ring);
	}

	void async_read()
	{
		recv_buffer.resize(size_to_read);
	    read_iov[0].iov_base = recv_buffer.data();
	    read_iov[0].iov_len = recv_buffer.size();

	    io_uring_sqe * sqe = io_uring_get_sqe(&ring);
	    io_uring_prep_readv(sqe, file, &read_iov[0], 1, 0);
	    io_uring_sqe_set_data(sqe, (void*)((ptrdiff_t)this | Action::Read));
	    io_uring_submit(&ring);
	}

	void async_accept()
	{
		accepted = make_peer();

		io_uring_sqe * sqe = io_uring_get_sqe(&ring);
	    io_uring_prep_accept(sqe, file, (struct sockaddr*) &accepted->client_addr, &client_addr_len, 0);
	    io_uring_sqe_set_data(sqe, (void*)((ptrdiff_t)this | Action::Accept));
	    io_uring_submit(&ring);
	}

	virtual peer * make_peer() { return new peer{ring, 0}; }
	virtual void   pack() {};
	virtual void unpack() {};
	virtual void set_events_from(peer * p)
	{
    	if (p->on_data)
    		this->on_data = p->on_data;
    	if (p->on_close)
    		this->on_close = p->on_close;
	}

	void sent(size_t count)
	{
		//std::cout << "sent " << count << std::endl;
		if (count == outbox.front().size())
			outbox.pop_front();
		else if (count < outbox.front().size())
			outbox.front().erase(outbox.front().begin(), outbox.front().begin()+count);
	}
	void rcvd(size_t count)
	{
    	if (count == recv_buffer.size() && size_to_read < 1024*1024) // buffer full
    		size_to_read *= 2;
    	else if (count < recv_buffer.size() / 2 && size_to_read > 16) // buffer mostly empty
    	{
    		size_to_read /= 2;
        	recv_buffer.resize(count);
        }
        inbox.push_back(std::move(recv_buffer));
        async_read();
        if (on_data)
        	on_data(this);
        unpack();
	}
};

struct inbox_reader
{
	peer & p;
	decltype(p.inbox)::iterator it_inbox;
	std::vector<no_init<char>>::iterator it_vec;
	inbox_reader(peer & p_)
		: p(p_)
		, it_inbox(p.inbox.begin())
		, it_vec(it_inbox->begin())
	{}
	const auto & operator()()
	{
		while (it_vec == it_inbox->end())
		{
			++it_inbox;
			if (it_inbox == p.inbox.end())
				throw utttil::srlz::device::stream_end_exception();
			it_vec = it_inbox->begin();
		}
		//printf("%.02X-\n", *it_vec);
		return *it_vec++;
	}
	void update_inbox()
	{
		if (it_inbox != p.inbox.end())
			it_inbox->erase(it_inbox->begin(), it_vec);
		p.inbox.erase(p.inbox.begin(), it_inbox);
	}
};

template<typename InMsg, typename OutMsg>
struct msg_peer : peer
{
	utttil::ring_buffer<std::unique_ptr< InMsg>>  inbox_msg;
	utttil::ring_buffer<std::unique_ptr<OutMsg>> outbox_msg;

	std::function<void (msg_peer * p)> on_message;

	msg_peer(io_uring & ring_, int file_)
		: peer(ring_, file_)
		,  inbox_msg(1024)
		, outbox_msg(1024)
	{}

	peer * make_peer() override { return new msg_peer<InMsg,OutMsg>{ring, 0}; }

	std::unique_ptr<InMsg> read()
	{
		while(inbox_msg.empty())
		{}
		auto result = std::move(inbox_msg.front());
		inbox_msg.pop_front();
		return result;
	}

	void async_send(std::unique_ptr<OutMsg> && msg)
	{
		outbox_msg.push_back(std::move(msg));

	    io_uring_sqe * sqe = io_uring_get_sqe(&ring);
	    io_uring_prep_nop(sqe);
	    io_uring_sqe_set_data(sqe, (void*)((ptrdiff_t)this | Action::Pack));
	    io_uring_submit(&ring);	
	}
	void pack() override
	{
		if (outbox_msg.empty() || outbox.full())
			return;
		std::vector<char> v;
		v.reserve(sizeof(typename decltype(outbox_msg)::value_type));
		auto s = utttil::srlz::to_binary(utttil::srlz::device::back_inserter(v));
		s << *outbox_msg.front();
		outbox_msg.pop_front();
		async_write(std::move(v));
	}
	void unpack() override
	{
		if (inbox.empty() || inbox_msg.full())
			return;

		bool has_messages = false;

		auto deserializer = utttil::srlz::from_binary(inbox_reader(*this));
		for (;;)
		{
			auto msg_uptr = std::make_unique<InMsg>();
			auto checkpoint = deserializer.read;
			try {
				deserializer >> *msg_uptr;
				inbox_msg.push_back(std::move(msg_uptr));
				has_messages = true;
			} catch (utttil::srlz::device::stream_end_exception &) {
				checkpoint.update_inbox();
				break;
			}
		}
		if (has_messages && on_message)
			on_message(this);
	}
	void set_events_from(peer * p) override
	{
    	if (p->on_data)
    		this->on_data = p->on_data;
    	if (p->on_close)
    		this->on_close = p->on_close;
    	if (((msg_peer<InMsg,OutMsg>*)p)->on_message)
    		this->on_message = ((msg_peer<InMsg,OutMsg>*)p)->on_message;
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

	std::unique_ptr<peer> bind(utttil::url url, std::function<void(peer*)> on_accept_=nullptr, std::function<void(peer*)> on_data_=nullptr, std::function<void(peer*)> on_close_=nullptr)
	{
		if (url.protocol != "tcp")
			return {};

		int sock = this->server_socket(std::stoull(url.port));
		if (sock == -1)
			return {};

	    std::unique_ptr<peer> peer_sptr(new peer{ring, sock});
	    if (on_accept_)
	    	peer_sptr->on_accept = on_accept_;
	    if (on_data_)
	    	peer_sptr->on_data = on_data_;
	    if (on_close_)
	    	peer_sptr->on_close = on_close_;
	    peer_sptr->async_accept();
	    return peer_sptr;
	}

	std::unique_ptr<peer> connect(const utttil::url url, std::function<void(peer*)> on_data_=nullptr, std::function<void(peer*)> on_close_=nullptr)
	{
		if (url.protocol != "tcp")
			return {};

		int sock = this->client_socket(url.host, std::stoull(url.port));
		if (sock == -1) {
			return {};
		}

	    std::unique_ptr<peer> peer_sptr(new peer{ring, sock});
	    if (on_data_)
	    	peer_sptr->on_data = on_data_;
	    if (on_close_)
	    	peer_sptr->on_close = on_close_;
	    peer_sptr->async_read();
	    return peer_sptr;
	}

	template<typename InMsg, typename OutMsg>
	std::unique_ptr<msg_peer<InMsg,OutMsg>> bind_srlz(const utttil::url url, std::function<void(peer*)> on_accept_=nullptr, std::function<void(msg_peer<InMsg,OutMsg>*)> on_msg_=nullptr, std::function<void(peer*)> on_close_=nullptr)
	{
		if (url.protocol != "tcp")
			return {};

		int sock = this->server_socket(std::stoull(url.port));
		if (sock == -1)
			return {};

	    std::unique_ptr<msg_peer<InMsg,OutMsg>> peer_sptr(new msg_peer<InMsg,OutMsg>{ring, sock});
	    if (on_accept_)
	    	peer_sptr->on_accept = on_accept_;
	    if (on_msg_)
	    	peer_sptr->on_message = on_msg_;
	    if (on_close_)
	    	peer_sptr->on_close = on_close_;
	    peer_sptr->async_accept();
	    return peer_sptr;
	}

	template<typename InMsg, typename OutMsg>
	std::unique_ptr<msg_peer<InMsg,OutMsg>> connect_srlz(const utttil::url url, std::function<void(msg_peer<InMsg,OutMsg>*)> on_msg_=nullptr, std::function<void(peer*)> on_close_=nullptr)
	{
		if (url.protocol != "tcp")
			return {};

		int sock = this->client_socket(url.host, std::stoull(url.port));
		if (sock == -1) {
			return {};
		}

	    std::unique_ptr<msg_peer<InMsg,OutMsg>> peer_sptr(new msg_peer<InMsg,OutMsg>{ring, sock});
	    if (on_msg_)
	    	peer_sptr->on_message = on_msg_;
	    if (on_close_)
	    	peer_sptr->on_close = on_close_;
	    peer_sptr->async_read();
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
	            	p->async_accept();
	            	if (p->on_accept)
	            		p->on_accept(new_p);
	            	new_p->set_events_from(p);
	                new_p->async_read();
	                break;
	            }
	            case Action::Read:
	            	if (cqe->res == 0)
	            	{
		            	if (p->on_close)
		            		p->on_close(p);
		            	break;
	            	}
	            	//std::cout << "read " << cqe->res << std::endl;
	            	p->rcvd(cqe->res);
	                break;
	            case Action::Write:
	            	p->sent(cqe->res);
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
