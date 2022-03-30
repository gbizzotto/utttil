
#include <iostream>
#include <memory>
#include <string>
#include <atomic>

#include <utttil/io.hpp>
#include <utttil/perf.hpp>

#include "msg.hpp"

int main()
{
	Request sent_by_client;
	sent_by_client.type = Request::Type::NewOrder;
	sent_by_client.seq        = utttil::max<decltype(sent_by_client.seq)>();
	sent_by_client.account_id = utttil::max<decltype(sent_by_client.account_id)>();
	sent_by_client.req_id     = utttil::max<decltype(sent_by_client.req_id)>();
	NewOrder &new_order = sent_by_client.new_order;
	new_order.instrument_id   = utttil::max<decltype(new_order.instrument_id)>();
	new_order.is_sell                   = false;
	new_order.is_limit                  = true;
	new_order.is_stop                   = true;
	new_order.participate_dont_initiate = false;
	new_order.time_in_force = TimeInForce::GTD;
	new_order.lot_count      = utttil::max<decltype(new_order.lot_count     )>();
	new_order.pic_count      = utttil::max<decltype(new_order.pic_count     )>();
	new_order.stop_pic_count = utttil::max<decltype(new_order.stop_pic_count)>();

	utttil::io::context<Request,Request> ctx;
	ctx.run();
	
	std::cout << "context running" << std::endl;

	// client
	auto client_sptr = ctx.connect(utttil::url("tcp://127.0.0.1:1234"));
	if ( ! client_sptr)
	{
		std::cerr << "Couldnt connect" << std::endl;
		return 1;
	}
	std::cout << "connect done" << std::endl;

	//std::this_thread::sleep_for(std::chrono::milliseconds(100));

	auto time_start = std::chrono::high_resolution_clock::now();
	size_t msg_count = 0;
	for ( auto deadline = std::chrono::steady_clock::now()+std::chrono::seconds(10)
		; std::chrono::steady_clock::now() < deadline
		; )
	{
		if (client_sptr->get_outbox_msg()->full())
			continue;
		//std::cout << "msg # " << msg_count << std::endl;
		client_sptr->async_send(sent_by_client);
		msg_count++;
	}
	sent_by_client.type = Request::Type::End;
	client_sptr->async_send(sent_by_client);
	std::this_thread::sleep_for(std::chrono::milliseconds(10));

	auto time_stop = std::chrono::high_resolution_clock::now();
	auto elapsed = time_stop - time_start;
	std::cout << msg_count*1000000000 / elapsed.count() << " Request / s" << std::endl;

	ctx.stop();

	return 0;
}
