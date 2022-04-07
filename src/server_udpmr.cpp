
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
	utttil::io::context ctx;
	ctx.run();

	auto server_sptr = ctx.bind_msg<Request,Request>(utttil::url("udpmr://226.1.1.1:1234"), utttil::url("tcp://127.0.0.1:2010"));
	if ( ! server_sptr)
	{
		std::cerr << "Couldnt bind" << std::endl;
		return 1;
	}
	std::cout << "server running" << std::endl;

	std::this_thread::sleep_for(std::chrono::milliseconds(2000));

	Request req;
	auto time_start = std::chrono::high_resolution_clock::now();
	size_t msg_count = 0;
	for ( auto deadline = std::chrono::steady_clock::now()+std::chrono::seconds(10)
		; std::chrono::steady_clock::now() < deadline
		; )
	{
		prepare_request(req);
		server_sptr->async_send(req);
		msg_count++;
	}
	prepare_request(req);
	req.type = Request::Type::End;
	server_sptr->async_send(req);
	msg_count++;

	std::cout << "Last msg: #" << req.seq << std::endl;

	auto time_stop = std::chrono::high_resolution_clock::now();
	auto elapsed = time_stop - time_start;
	std::cout << msg_count*1000000000 / elapsed.count() << " Request / s" << std::endl;

	// ensure client gets the termination message
	for ( auto deadline = std::chrono::steady_clock::now()+std::chrono::seconds(5)
		; std::chrono::steady_clock::now() < deadline
		; )
	{
		sleep(1);
		prepare_request(req);
		req.type = Request::Type::End;
		server_sptr->async_send(req);
		msg_count++;
	}

	return 0;
}
