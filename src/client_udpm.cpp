
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
	utttil::io::context<Request,Request> ctx;
	ctx.run();

	auto client_sptr = ctx.connect(utttil::url("udpm://226.1.1.1:1234"));
	if ( ! client_sptr)
	{
		std::cerr << "Couldnt connect" << std::endl;
		return 1;
	}
	std::cout << "client connected" << std::endl;

	seq_t next_seq(0);
	size_t msgs = 0;
	auto time_start = std::chrono::high_resolution_clock::now();
	for ( auto deadline = std::chrono::steady_clock::now()+std::chrono::seconds(12)
		; std::chrono::steady_clock::now() < deadline
		; )
	{
		if (client_sptr->get_inbox_msg()->empty())
		{
			_mm_pause();
			continue;
		}
		Request & req = client_sptr->get_inbox_msg()->front();
		if (next_seq < req.seq) {
			std::cout << "Gap " << next_seq << " - " << req.seq << std::endl;
			next_seq = req.seq;
			++next_seq;
		} else if (req.seq < next_seq) {
			std::cout << "Old " << next_seq << " - " << req.seq << std::endl;
		} else {
			next_seq = req.seq;
			++next_seq;
		}
		client_sptr->get_inbox_msg()->pop_front();
		msgs++;
	}
	auto time_stop = std::chrono::high_resolution_clock::now();
	auto elapsed = time_stop - time_start;
	std::cout << msgs*1000000000 / elapsed.count() << " Request / s" << std::endl;

	ctx.stop();

	return 0;
}
