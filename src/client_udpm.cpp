
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
	ctx.run();

	auto clinet_sptr = ctx.connect(utttil::url("udpm://226.1.1.1:1234"));
	if ( ! clinet_sptr)
	{
		std::cerr << "Couldnt connect" << std::endl;
		return 1;
	}
	std::cout << "client connected" << std::endl;

	size_t msgs = 0;
	auto time_start = std::chrono::high_resolution_clock::now();
	for ( auto deadline = std::chrono::steady_clock::now()+std::chrono::seconds(12)
		; std::chrono::steady_clock::now() < deadline
		; )
	{
		if (clinet_sptr->inbox_msg.empty())
		{
			_mm_pause();
			continue;
		}
		clinet_sptr->inbox_msg.front();
		clinet_sptr->inbox_msg.pop_front();
		msgs++;
	}
	auto time_stop = std::chrono::high_resolution_clock::now();
	auto elapsed = time_stop - time_start;
	std::cout << msgs*1000000000 / elapsed.count() << " Request / s" << std::endl;

	ctx.stop();

	return 0;
}
