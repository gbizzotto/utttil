
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
	utttil::io::context ctx;
	ctx.run();

	auto client_sptr = ctx.connect_msg<Request,Request>(utttil::url("udpmr://226.1.1.1:1234"), utttil::url("tcp://127.0.0.1:2010"));
	if ( ! client_sptr)
	{
		std::cerr << "Couldnt connect" << std::endl;
		return 1;
	}
	std::cout << "client connected" << std::endl;

	// wait for 1st msg
	while (client_sptr->get_inbox_msg()->empty())
		_mm_pause();

	seq_t next_seq(0);
	size_t msgs = 0;
	auto time_start = std::chrono::high_resolution_clock::now();
	//for ( auto deadline = std::chrono::steady_clock::now()+std::chrono::seconds(10)
	//	; std::chrono::steady_clock::now() < deadline
	//	; )
	for (;;)
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

		if (req.type == Request::Type::End)
		{
			std::cout << "Last Msg: #" << req.seq << std::endl;
			break;
		}
	}
	std::cout << "Last Msg: #" << next_seq << std::endl;
	auto time_stop = std::chrono::high_resolution_clock::now();
	auto elapsed = time_stop - time_start;
	std::cout << msgs*1000000000 / elapsed.count() << " Request / s" << std::endl;

	ctx.stop();

	return 0;
}
