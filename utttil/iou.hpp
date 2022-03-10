
#pragma once

#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <immintrin.h>

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
#include <map>

#include <liburing.h>

#include <utttil/url.hpp>
#include <utttil/ring_buffer.hpp>
#include <utttil/srlz.hpp>
#include <utttil/no_init.hpp>
#include <utttil/on_scope_exit.hpp>

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
	size_t id;
	int fd;
	io_uring & ring;


	utttil::ring_buffer<std::shared_ptr<peer>> accepted;


	inline const static size_t default_output_ring_buffer_size = 16;

	iovec write_iov[1];
	utttil::ring_buffer<buffer> outbox;


	inline const static size_t default_accepted_ring_buffer_size_ = 32;
	inline const static size_t default_input_ring_buffer_size = 16;
	inline const static size_t min_input_chunk_size = 2048;
	inline const static size_t max_input_chunk_size = 2048;

	iovec read_iov[1];
	utttil::ring_buffer<buffer> inbox;

	size_t input_chunk_size;
	std::vector<no_init<char>> recv_buffer;

	sockaddr_in accept_client_addr;
	socklen_t client_addr_len = sizeof(sockaddr_in);

	inline peer( io_uring & ring_
	           , int fd_
	           , size_t accepted_ring_buffer_size_= default_accepted_ring_buffer_size_
	           , size_t output_ring_buffer_size = default_output_ring_buffer_size
	           , size_t input_ring_buffer_size = default_input_ring_buffer_size
	           , size_t input_chunk_size_ = min_input_chunk_size )
		: ring(ring_)
		, accepted(accepted_ring_buffer_size_)
		, fd(fd_)
		, outbox(output_ring_buffer_size)
		, inbox(input_ring_buffer_size)
		, input_chunk_size(input_chunk_size_)
	{}

	inline size_t outbox_size()
	{
		size_t result = 0;
		for (auto it=outbox.begin() ; it!=outbox.end() ; ++it)
			result += it->size();
		return result;
	}
	inline size_t inbox_size()
	{
		size_t result = 0;
		for (auto it=inbox.begin() ; it!=inbox.end() ; ++it)
			result += it->size();
		return result;
	}

	inline void async_accept()
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

	inline void async_write(buffer && data)
	{
		//std::cout << "iou write " << data.size() << std::endl;
		//for (char & c : data)
		//	printf("%.02X ", c);
		//printf("\n");

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

		//std::cout << "async_write with ptr: " << std::hex << this << std::dec << std::endl;
		//std::cout << "outbox_size: " << outbox_size() << std::endl;
	}

	inline void async_read()
	{
		//std::cout << "iou == async_read" << std::endl;
		recv_buffer.resize(input_chunk_size);
		read_iov[0].iov_base = recv_buffer.data();
		read_iov[0].iov_len = recv_buffer.size();

		io_uring_sqe * sqe = io_uring_get_sqe(&ring);
		io_uring_prep_readv(sqe, fd, &read_iov[0], 1, 0);
		size_t user_data = (id << 3) | Action::Read;
		io_uring_sqe_set_data(sqe, (void*)user_data);
		io_uring_submit(&ring);
	}

	inline void sent(size_t count)
	{
		//std::cout << "sent " << count << std::endl;
		//std::cout << "outbox_size: " << outbox_size() << std::endl;
		while (count >= 0 && outbox.size() > 0 && count >= outbox.front().size())
		{
			count -= outbox.front().size();
			outbox.pop_front();
		}
		if (count > 0 && count < outbox.front().size())
			outbox.front().erase(outbox.front().begin(), outbox.front().begin()+count);
	}

	inline void rcvd(size_t count)
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
		//unpack();
	}
};

struct context
{
	io_uring ring;
	std::thread t;
	std::atomic_bool go_on = true;
	std::map<size_t, std::weak_ptr<peer>> peers;
	size_t next_id = 1;

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


	std::shared_ptr<peer> bind(const utttil::url url)
	{
		if (url.protocol != "tcp")
			return nullptr;

		int fd = server_socket(std::stoull(url.port));
		if (fd == -1) {
			std::cout << "server_socket() failed" << std::endl;
			return nullptr;
		}
		auto peer_sptr = std::make_shared<peer>(ring, fd);
		if ( ! peer_sptr)
			return nullptr;

		peer_sptr->id = next_id++;
		peers[peer_sptr->id] = peer_sptr;
		peer_sptr->async_accept();
		return peer_sptr;
	}

	std::shared_ptr<peer> connect(const utttil::url url)
	{
		if (url.protocol != "tcp")
			return nullptr;

		int fd = client_socket(url.host.c_str(), std::stoull(url.port));
		if (fd == -1) {
			std::cout << "client_socket() failed" << std::endl;
			return nullptr;
		}
		std::shared_ptr<peer> peer_sptr = std::make_shared<peer>(ring, fd);
		if ( ! peer_sptr)
			return nullptr;
		
		peer_sptr->id = next_id++;
		peers[peer_sptr->id] = peer_sptr;
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
			Action action = static_cast<Action>(((ptrdiff_t)cqe->user_data) & 0x7);
			//std::cout << "action happened: " << action << std::endl;
			auto id = cqe->user_data >> 3;
			//std::cout << "id: " << id << std::endl;
			auto peer_it = peers.find(id);
			if (peer_it == peers.end())
				continue;
			std::shared_ptr<peer> peer_sptr(peers[id]);
			if ( ! peer_sptr)
			{
				std::cout << "! peer_sptr" << std::endl;
				peers.erase(peer_it);
				continue;
			}
			if (cqe->res < 0) {
				std::cerr << "Async request failed: " << strerror(-cqe->res) << std::endl;
				peer_sptr->fd = -1;
				return;
			}
			//std::cout << "action: " << action << std::endl;
			switch (action)
			{
				case Action::None:
					break;
				case Action::Accept:
				{
					int fd = cqe->res;
					std::shared_ptr<peer> new_peer_sptr = std::make_shared<peer>(ring, fd);					
					if ( ! new_peer_sptr)
					{
						::close(fd);
						break;
					}
					new_peer_sptr->id = next_id++;
					peers[new_peer_sptr->id] = new_peer_sptr;
					new_peer_sptr->async_read();
					peer_sptr->accepted.push_back(new_peer_sptr);
					peer_sptr->async_accept();
					break;
				}
				case Action::Read:
				{
					//std::cout << "iou read " << cqe->res << std::endl;

					if (cqe->res < 0) {
						std::cout << "closing" << std::endl;
						peer_sptr->fd = -1;
						peers.erase(peer_it);
					} else {
						peer_sptr->rcvd(cqe->res);
					}
					break;
				}
				case Action::Write:
				{
					//std::cout << "iou write " << cqe->res << std::endl;

					peer_sptr->sent(cqe->res);
					break;
				}
				//case Action::Pack:
				//{
				//	auto ok = utttil::small_shared_ptr<peer_msg_writing_base>::copy_persisted(cqe->user_data);
				//	ok->pack();
				//	break;
				//}
				default:
					std::cerr << "iou Invalid action " << (int)action << std::endl;
					break;
			}
			/* Mark this request as processed */
			io_uring_cqe_seen(&ring, cqe);
		}
	}
};

}} // namespace
