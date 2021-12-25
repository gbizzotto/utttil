
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

	peer * accepted_peer = nullptr;
	inline static socklen_t client_addr_len = sizeof(accepted_peer->client_addr);

	std::function<void (peer * p)> on_accept;
	std::function<void (peer * p)> on_data;
	std::function<void (peer * p)> on_close;

	peer(io_uring & ring_, int file_)
		: ring(ring_)
		, file(file_)
	{}
	~peer()
	{
		std::cout <<"iou ~peer close"<< std::endl;
		close();
		//std::cout <<"~peer done"<< std::endl;
	}

	void close()
	{
		// TODO use io_uring_prep_close ?
		::shutdown(file, SHUT_RDWR);
	}

	size_t inbox_size()
	{
		size_t result = 0;
		for (auto it=inbox.begin() ; it!=inbox.end() ; ++it)
			result += it->size();
		return result;
	}

	void async_write(std::vector<char> && data)
	{
		//std::cout << "iou write " << data.size() << std::endl;
		//for (char & c : data)
		//	printf("%.02X ", c);
		//printf("\n");

		while(outbox.full())
		{}
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
		//std::cout << "iou == async_read" << std::endl;
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
		accepted_peer = make_peer();

		io_uring_sqe * sqe = io_uring_get_sqe(&ring);
		io_uring_prep_accept(sqe, file, (struct sockaddr*) &accepted_peer->client_addr, &client_addr_len, 0);
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
		if (count == 0)
		{
			if (on_close)
				on_close(this);
			return;
		}

		if (count == recv_buffer.size() && size_to_read < 2048) // buffer full
			size_to_read *= 2;
		else if (count < recv_buffer.size() / 2 && size_to_read > 32) // buffer mostly empty
			size_to_read /= 2;
		recv_buffer.resize(count);
		while(inbox.full())
		{}
		inbox.push_back(std::move(recv_buffer));
		async_read();
		if (on_data)
			on_data(this);
		unpack();
	}
	void accepted(int fd)
	{
		peer * new_p = accepted_peer;
		new_p->file = fd;
		async_accept();
		if (on_accept)
			on_accept(new_p);
		new_p->set_events_from(this);
		new_p->async_read();
	}
};

struct inbox_reader
{
	peer * p;
	typename utttil::ring_buffer<std::vector<no_init<char>>>::iterator it_inbox;
	std::vector<no_init<char>>::iterator it_vec;
	inbox_reader(peer & p_)
		: p(&p_)
		, it_inbox(p->inbox.begin())
		, it_vec(it_inbox->begin())
	{}
	const auto & operator()()
	{
		while (it_vec == it_inbox->end())
		{
			++it_inbox;
			if (it_inbox == p->inbox.end())
				throw utttil::srlz::device::stream_end_exception();
			it_vec = it_inbox->begin();
		}
		//printf("%.02X-\n", *it_vec);
		return *it_vec++;
	}
	size_t inbox_size_left()
	{
		auto it = it_inbox;
		size_t result = std::distance(it_vec, it_inbox->end());
		for (++it ; it!=p->inbox.end() ; ++it)
			result += it->size();
		return result;
	}
	void update_inbox()
	{
		while (it_vec == it_inbox->end())
		{
			++it_inbox;
			if (it_inbox == p->inbox.end())
				break;
			it_vec = it_inbox->begin();
		}		//std::cout << "update_inbox" << std::endl;
		if (it_inbox != p->inbox.end())
			it_inbox->erase(it_inbox->begin(), it_vec);
		p->inbox.erase(p->inbox.begin(), it_inbox);
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
		//	std::this_thread::sleep_for(std::chrono::microseconds(1));
		auto result = std::move(inbox_msg.front());
		inbox_msg.pop_front();
		return result;
	}

	void async_send(std::unique_ptr<OutMsg> && msg)
	{
		if ( ! msg)
			throw std::exception();
		while(outbox_msg.full())
		{}
		outbox_msg.push_back(std::move(msg));

		io_uring_sqe * sqe = io_uring_get_sqe(&ring);
		io_uring_prep_nop(sqe);
		io_uring_sqe_set_data(sqe, (void*)((ptrdiff_t)this | Action::Pack));
		io_uring_submit(&ring);	
	}
	void send(const OutMsg & msg)
	{
		std::vector<char> v;
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

	void pack() override
	{
		if (outbox_msg.empty() || outbox.full())
			return;
		send(*outbox_msg.front());
		outbox_msg.pop_front();
	}
	void unpack() override
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

		bool has_messages = false;
		auto deserializer = utttil::srlz::from_binary(inbox_reader(*this));
		auto checkpoint = deserializer.read;
		for (;;)
		{
			//std::cout << "loop init inbox_size: " << inbox_size() << ", inbox: " << inbox << std::endl;
			//std::cout << "loop init inbox_msg: " << inbox_msg << std::endl;
			//std::cout << "loop init deserializer: it_inbox: " << deserializer.read.it_inbox.pos << std::endl;
			//std::cout << "new checkpoint: it_inbox: " << checkpoint.it_inbox.pos << std::endl;
			try {
				if (deserializer.read.inbox_size_left() < 2) {
					//std::cout << "deserializer.read.inbox_size_left() < 2" << std::endl;
					//std::cout << "-1 checkpoint: it_inbox: " << checkpoint.it_inbox.pos << std::endl;
					break;
				}
				int msg_size = (deserializer.read() << 8) + deserializer.read();
				//std::cout << "msg_size: " << msg_size << std::endl;
				if (msg_size == 0) {
					std::cout << "iou msg size is zero, this can't be good..." << std::endl;
					break;
				}
				if (msg_size > deserializer.read.inbox_size_left()) {
					std::cout << "iou inbox does not contain a full msg" << std::endl;
					//std::cout << "-2 checkpoint: it_inbox: " << checkpoint.it_inbox.pos << std::endl;
					break;
				}
				auto msg_uptr = std::make_unique<InMsg>();
				//std::cout << "about de deserialize a message. deserializer: it_inbox: " << deserializer.read.it_inbox.pos << std::endl;
				deserializer >> *msg_uptr;
				//std::cout << "msg deserialized. deserializer: it_inbox: " << deserializer.read.it_inbox.pos << std::endl;

				while(inbox_msg.full())
				{}
				inbox_msg.push_back(std::move(msg_uptr));

				//std::cout << "2 inbox_size: " << inbox_size() << ", inbox: " << inbox << std::endl;
				//std::cout << "2 inbox_msg: " << inbox_msg << std::endl;

				has_messages = true;
			} catch (utttil::srlz::device::stream_end_exception &) {
				//std::cout << "utttil::srlz::device::stream_end_exception" << std::endl;
				//std::cout << "-3 checkpoint: it_inbox: " << checkpoint.it_inbox.pos << std::endl;
				break;
			}
			checkpoint = deserializer.read;
		}
		checkpoint.update_inbox();
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
		params.sq_thread_idle = 1000;


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
			if (ret == -ETIME) {
				continue;
			}
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
			//std::cout << "action: " << action << std::endl;
			switch (action)
			{
				case Action::None:
					break;
				case Action::Accept:
					p->accepted(cqe->res);
					break;
				case Action::Read:
					//std::cout << "iou read " << cqe->res << std::endl;
					p->rcvd(cqe->res);
					break;
				case Action::Write:
					p->sent(cqe->res);
					break;
				case Action::Pack:
					p->pack();
					break;
				default:
					std::cerr << "iou Invalid action" << std::endl;
					break;
			}
			/* Mark this request as processed */
			io_uring_cqe_seen(&ring, cqe);
		}
	}
};

}} // namespace
