
#include <iostream>
#include <memory>
#include <string>
#include <atomic>
#include <chrono>

#include <utttil/io.hpp>
#include <utttil/assert.hpp>

#include "msg.hpp"

void prepare_request(Request & sent_by_server)
{
	static decltype(sent_by_server.seq) next_seq(0);

	sent_by_server.type = Request::Type::NewOrder;
	sent_by_server.seq        = next_seq++;
	sent_by_server.account_id = utttil::max<decltype(sent_by_server.account_id)>();
	sent_by_server.req_id     = utttil::max<decltype(sent_by_server.req_id)>();
	NewOrder &new_order = sent_by_server.new_order;
	new_order.instrument_id   = utttil::max<decltype(new_order.instrument_id)>();
	new_order.is_sell                   = false;
	new_order.is_limit                  = true;
	new_order.is_stop                   = true;
	new_order.participate_dont_initiate = false;
	new_order.time_in_force = TimeInForce::GTD;
	new_order.lot_count      = utttil::max<decltype(new_order.lot_count     )>();
	new_order.pic_count      = utttil::max<decltype(new_order.pic_count     )>();
	new_order.stop_pic_count = utttil::max<decltype(new_order.stop_pic_count)>();
}

int main()
{
	utttil::io::context<Request,Request> ctx;
	ctx.run();

	auto server_sptr = ctx.bind(utttil::url("udpm://226.1.1.1:1234"));
	if ( ! server_sptr)
	{
		std::cerr << "Couldnt bind" << std::endl;
		return 1;
	}
	std::cout << "server running" << std::endl;

	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	auto time_start = std::chrono::high_resolution_clock::now();
	size_t msg_count = 0;
	for ( auto deadline = std::chrono::steady_clock::now()+std::chrono::seconds(12)
		; std::chrono::steady_clock::now() < deadline
		; )
	{
		Request & req = server_sptr->get_outbox_msg()->back();
		prepare_request(req);
		server_sptr->get_outbox_msg()->advance_back(1);
		msg_count++;
	}

	auto time_stop = std::chrono::high_resolution_clock::now();
	auto elapsed = time_stop - time_start;
	std::cout << msg_count*1000000000 / elapsed.count() << " Request / s" << std::endl;


	return 0;
}
