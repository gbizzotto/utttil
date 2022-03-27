

#include <iostream>
#include <memory>
#include <string>
#include <atomic>
#include <chrono>

#include <utttil/io.hpp>
#include <utttil/assert.hpp>

#include "msg.hpp"

int main()
{
	utttil::io::context<Request> ctx;
	//ctx.run();
	ctx.start_accept();
	ctx.start_read();
	//ctx.start_write();	
	auto server_sptr = ctx.bind(utttil::url("tcp://127.0.0.1:1234"));
	if ( ! server_sptr)
	{
		std::cerr << "Couldnt bind" << std::endl;
		return 1;
	}
	std::cout << "server running" << std::endl;

	auto server_client_sptr = server_sptr->accept_inbox.front();
	std::cout << "Accepted" << std::endl;
	server_sptr->accept_inbox.pop_front();


	std::cout << "Waiting for 1st msg" << std::endl;
	server_client_sptr->inbox_msg.front();
	std::cout << "start timer" << std::endl;
	size_t msgs = 0;
	auto time_start = std::chrono::high_resolution_clock::now();
	for ( auto deadline = std::chrono::steady_clock::now()+std::chrono::seconds(12)
		; std::chrono::steady_clock::now() < deadline
		; )
	{
		if (server_client_sptr->inbox_msg.empty())
		{
			_mm_pause();
			continue;
		}
		auto & msg = server_client_sptr->inbox_msg.front();
		msgs++;
		//std::cout << "msg # " << msgs << std::endl;
		if (msg.type == Request::Type::End)
			break;
		server_client_sptr->inbox_msg.pop_front();

		// reply
		//std::vector<char> v(sent_by_server.begin(), sent_by_server.end());
		//server_client_sptr->async_write(std::move(v));
	}
	auto time_stop = std::chrono::high_resolution_clock::now();
	auto elapsed = time_stop - time_start;
	std::cout << msgs*1000000000 / elapsed.count() << " Request / s" << std::endl;

	ctx.stop();

	return 0;
}
