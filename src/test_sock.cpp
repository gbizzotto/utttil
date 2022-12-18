
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <poll.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>

#include <thread>
#include <chrono>
#include <atomic>

#include <utttil/assert.hpp>
#include <utttil/log.hpp>
#include "utttil/io.hpp"

enum ConnStatus
{
	Unknown = 0,
	Connecting = 1,
	Connected = 2,
	Disconnected = 3,
};

int main()
{
	utttil::LogWithPrefix log("ok");
	log.add(std::cout);

	// socket
	int fd = ::socket(PF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (fd == -1) {
		std::cerr << "socket() " << ::strerror(errno) << std::endl;
		return -1;
	}
	int enable = 1;
	if (::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
		std::cerr << "setsockopt(SO_REUSEADDR) " << ::strerror(errno) << std::endl;
		return -1;
	}
	log << utttil::LogLevel::MUST_HAVE << "fd: " << fd << std::endl;

	std::atomic_bool go_on = true;

	utttil::url url("tcp://127.0.0.1:1234");
	utttil::url url2("tcp://123.132.213.231:1234");	

	// run local tcp server
	utttil::io::context ctx;
	ctx.run();

	auto server = ctx.bind_raw(url);
	std::thread ts([&]()
		{
			while(go_on)
			{
				if (server->get_accept_inbox()->empty())
					continue;
				auto server_client_sptr = server->get_accept_inbox()->front();
				log << utttil::LogLevel::MUST_HAVE << "Accepted" << std::endl;
				server->get_accept_inbox()->pop_front();
				server_client_sptr->get_outbox()->back() = 255;
				server_client_sptr->get_outbox()->advance_back();
			}
		});

	auto client1 = ctx.connect_raw<ConnStatus>(url);
	std::thread tc1([&]()
		{
			client1->data = Connecting;
			log << utttil::LogLevel::MUST_HAVE << "C1 Connecting" << std::endl;
			for ( auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10)
				; std::chrono::steady_clock::now() < deadline
				; std::this_thread::sleep_for(std::chrono::seconds(1))
				)
			{
				if ( ! client1->get_inbox()->empty())
				{
					log << utttil::LogLevel::MUST_HAVE << "C1 ConnectED" << std::endl;
					client1->data = Connected;
					return;
				}
			}
			log << utttil::LogLevel::MUST_HAVE << "C1 DISConnectED" << std::endl;
			client1->data = Disconnected;
		});

	auto client2 = ctx.connect_raw<ConnStatus>(url2);
	std::thread tc2([&]()
		{
			client2->data = Connecting;
			log << utttil::LogLevel::MUST_HAVE << "C2 Connecting" << std::endl;
			for ( auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10)
				; std::chrono::steady_clock::now() < deadline
				; std::this_thread::sleep_for(std::chrono::seconds(1))
				)
			{
				if ( ! client2->get_inbox()->empty())
				{
					log << utttil::LogLevel::MUST_HAVE << "C2 ConnectED" << std::endl;
					client2->data = Connected;
					return;
				}
			}
			log << utttil::LogLevel::MUST_HAVE << "C2 DISConnectED" << std::endl;
			client2->data = Disconnected;
		});

	tc1.join();
	tc2.join();
	go_on = false;
	ts.join();


/*
	int nfds = 1;
	int num_open_fds = 1;
	pollfd *pfds = (pollfd *)calloc(nfds, sizeof(struct pollfd));
	pfds[0].fd = fd;
	pfds[0].events = POLLIN;


	std::thread tc([&]()
		{
			std::this_thread::sleep_for(std::chrono::seconds(2));
			for ( auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(200)
				; std::chrono::steady_clock::now() < deadline
				; //std::this_thread::sleep_for(std::chrono::milliseconds(1000))
				)
			{
//				int err;
//				socklen_t len = sizeof(err);
//				int r = getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len);
//				auto rno = errno;
//				log << utttil::LogLevel::INFO << "getsockopt() " << r << ", err: " << err << " " << strerror(err) << std::endl;
//
//				char ok[1];
//				r = ::read(fd, ok, 1);
//				rno = errno;
//				log << utttil::LogLevel::INFO << "read(1): " << r << ", err: " << rno << " " << strerror(rno) << std::endl;
//
//				r = ::write(fd, ok, 1);
//				rno = errno;
//				log << utttil::LogLevel::INFO << "write(1): " << r << ", err: " << rno << " " << strerror(rno) << std::endl;

				log << utttil::LogLevel::MUST_HAVE << "Before poll" << std::endl;
				int ready = poll(pfds, nfds, 1);
				log << utttil::LogLevel::MUST_HAVE << "After poll" << std::endl;
				if (ready == -1)
				{
					log << utttil::LogLevel::MUST_HAVE << "poll err: " << ready << std::endl;
					return;
				}
				log << utttil::LogLevel::MUST_HAVE << "Ready: " << ready << std::endl;

                   if (pfds[0].revents != 0)
                   {
                       printf("  fd=%d; events: %s%s%s\n", pfds[0].fd,
                               (pfds[0].revents & POLLIN)  ? "POLLIN "  : "",
                               (pfds[0].revents & POLLHUP) ? "POLLHUP " : "",
                               (pfds[0].revents & POLLERR) ? "POLLERR " : "");

                       if (pfds[0].revents & POLLIN) {
                           char buf[10];
                           ssize_t s = read(pfds[0].fd, buf, sizeof(buf));
                           if (s == -1)
                           {
                           	log << utttil::LogLevel::MUST_HAVE << "read ok" << std::endl;
                           }
                           else
                           {
                           	log << utttil::LogLevel::MUST_HAVE << "read error" << std::endl;
                           }
                       } else {
                       	log << utttil::LogLevel::MUST_HAVE << "error or hup" << std::endl;
                       }
                       return;
                   }
			}
		});

	std::this_thread::sleep_for(std::chrono::milliseconds(2000));

	// connect
	sockaddr_in srv_addr;
	::memset(&srv_addr, 0, sizeof(srv_addr));
	srv_addr.sin_family = AF_INET;
	srv_addr.sin_port = ::htons(1234);
	srv_addr.sin_addr.s_addr = ::inet_addr("127.0.0.1");
	//srv_addr.sin_addr.s_addr = ::inet_addr("127.0.0.1");
	log << utttil::LogLevel::MUST_HAVE << "connect" << std::endl;
	auto ret = ::connect(fd, (const sockaddr *)&srv_addr, sizeof(srv_addr));
	log << utttil::LogLevel::MUST_HAVE << "connect() returned " << ret << ", errno: " << errno << " " << strerror(errno) << std::endl;
	if (ret == 0)
		return fd;

	go_on = false;

	tc.join();
	ts.join();
*/

	return 0;
}