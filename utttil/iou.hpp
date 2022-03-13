
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

#include <utttil/iou/peer.hpp>

namespace utttil {
namespace iou {

template<typename MsgT=no_msg_t>
struct context
{
	io_uring ring;
	std::thread t;
	std::atomic_bool go_on = true;
	std::map<size_t, std::weak_ptr<peer<MsgT>>> peers;
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

	std::shared_ptr<peer<MsgT>> bind(const utttil::url url)
	{
		if (url.protocol != "tcp")
			return nullptr;

		std::shared_ptr<peer<MsgT>> peer_sptr = peer<MsgT>::bind(next_id++, ring, url);
		if ( ! peer_sptr)
			return nullptr;

		peers[peer_sptr->id] = peer_sptr;
		peer_sptr->async_accept();
		return peer_sptr;
	}

	std::shared_ptr<peer<MsgT>> connect(const utttil::url url)
	{
		if (url.protocol != "tcp")
			return nullptr;

		std::shared_ptr<peer<MsgT>> peer_sptr = peer<MsgT>::connect(next_id++, ring, url);
		if ( ! peer_sptr)
			return nullptr;

		peers[peer_sptr->id] = peer_sptr;
		peer_sptr->async_read();
		return peer_sptr;
	}

	void loop()
	{
		io_uring_cqe *cqe;

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
			std::shared_ptr<peer<MsgT>> peer_sptr(peers[id]);
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
					std::shared_ptr<peer<MsgT>> new_peer_sptr = peer_sptr->accepted(next_id++, fd);
					if ( ! new_peer_sptr)
					{
						::close(fd);
						break;
					}
					peers[new_peer_sptr->id] = new_peer_sptr;
					new_peer_sptr->async_read();
					peer_sptr->accept_inbox.push_back(new_peer_sptr);
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
				case Action::Pack:
				{
					peer_sptr->pack();
					break;
				}
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
