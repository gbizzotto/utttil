

#include <iostream>
#include <memory>
#include <string>
#include <atomic>
#include <chrono>

#include <utttil/io.hpp>
#include <utttil/assert.hpp>
#include <utttil/perf.hpp>

template<typename D, typename F>
void do_for(D d, F f)
{
	for ( auto deadline = std::chrono::steady_clock::now()+d
		; std::chrono::steady_clock::now() < deadline
		; )
	{
		if ( ! f()) break;
	}	
}

int ok()
{
	int fd;
	fd = ::socket(PF_INET, SOCK_STREAM, 0);
	if (fd == -1) {
		std::cerr << "socket() " << ::strerror(errno) << std::endl;
		return -1;
	}
	int enable = 1;
	if (::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
		std::cerr << "setsockopt(SO_REUSEADDR) " << ::strerror(errno) << std::endl;
		return -1;
	}
	sockaddr_in srv_addr;
	::memset(&srv_addr, 0, sizeof(srv_addr));
	srv_addr.sin_family = AF_INET;
	srv_addr.sin_port = ::htons(1234);
	srv_addr.sin_addr.s_addr = ::htonl(INADDR_ANY);
	if (::bind(fd, (const sockaddr *)&srv_addr, sizeof(srv_addr)) < 0) {
		std::cerr << "bind() " << strerror(errno) << std::endl;;
		return -1;
	}
	if (::listen(fd, 32) < 0) {
		std::cerr << "listen() " << strerror(errno) << std::endl;;
		return -1;
	}

	sockaddr_in accept_client_addr;
	socklen_t client_addr_len = sizeof(sockaddr_in);
	int client_fd = ::accept(fd, (sockaddr*) &accept_client_addr, &client_addr_len);

	auto time_start = std::chrono::high_resolution_clock::now();
	utttil::measurement_point mp("recv()");
	size_t count_bytes = 0;
	char buffer[65536];
	do_for(std::chrono::seconds(10), [&]()
	{
		utttil::measurement m(mp);
		int count = ::recv(client_fd, buffer, 1440, 0);
		if (count == -1)
			return false;
		count_bytes += count;
		return true;
	});
	auto time_stop = std::chrono::high_resolution_clock::now();
	auto elapsed = time_stop - time_start;
	std::cout << count_bytes*1000000000 / elapsed.count() << " B / s" << std::endl;
	return 0;
}


int main()
{
	ok();
	return 0;



	utttil::io::context<> ctx;
	auto server_sptr = ctx.bind(utttil::url("tcp://127.0.0.1:1234"));
	ctx.start_accept();
	ctx.start_read();
	ctx.start_write();
	if ( ! server_sptr)
	{
		std::cerr << "Couldnt bind" << std::endl;
		return 1;
	}
	std::cout << "server running" << std::endl;

	auto server_client_sptr = server_sptr->accept_inbox.front();


	//auto server_client_sptr = server_sptr->accept();
	if ( ! server_client_sptr)
	{
		std::cerr << "accept failed" << std::endl;
		return 1;
	}
	std::cout << "Accepted" << std::endl;
	//server_sptr->accept_inbox.pop_front();

	size_t bytes = 0;

	std::cout << "read" << std::endl;
	while (server_client_sptr->inbox.empty())
		_mm_pause();
	std::cout << "read 1" << std::endl;
	auto front_stretch = server_client_sptr->inbox.front_stretch();
	std::cout << "Rcvd: " << std::string(std::get<0>(front_stretch), std::get<1>(front_stretch)) << std::endl;
	auto time_start = std::chrono::high_resolution_clock::now();
	bytes += std::get<1>(front_stretch);
	server_client_sptr->inbox.advance_front(std::get<1>(front_stretch));
	for ( auto deadline = std::chrono::steady_clock::now()+std::chrono::seconds(10)
		; std::chrono::steady_clock::now() < deadline
		; )
	{
		front_stretch = server_client_sptr->inbox.front_stretch();
		bytes += std::get<1>(front_stretch);
		server_client_sptr->inbox.advance_front(std::get<1>(front_stretch));
	}
	auto time_stop = std::chrono::high_resolution_clock::now();
	auto elapsed = time_stop - time_start;
	std::cout << bytes*1000000000 / elapsed.count() << " B / s" << std::endl;

	ctx.stop();

	return 0;
}
