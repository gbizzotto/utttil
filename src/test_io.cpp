
#include "headers.hpp"

#include <iostream>
#include <memory>
#include <string>
#include <atomic>

#include <utttil/assert.hpp>

#include "utttil/io.hpp"

#include "msg.hpp"

bool test_2_ways(std::string url)
{
	std::string sent_by_client = "32jk1hkjh1k3j4h62kj345h6345kljh345kj7h";
	std::string sent_by_server = "4322431423412412412341243213";
	std::string recv_by_client;
	std::string recv_by_server;

	utttil::io::context ctx;
	ctx.run();
	std::cout << "Context running" << std::endl;

	std::cout << "Sizeof(ctx): " << sizeof(ctx) << std::endl;

	// server
	auto server_sptr = ctx.bind(url);
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	std::cout << "Bind done" << std::endl;

	std::cout << "Sizeof(*server_sptr): " << sizeof(*server_sptr) << std::endl;

	// client
	auto client_sptr = ctx.connect(url);
	ASSERT_ACT(client_sptr, !=, nullptr, return false);
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	std::cout << "Connect done" << std::endl;

	std::cout << "Sizeof(*client_sptr): " << sizeof(*client_sptr) << std::endl;

	while (server_sptr->accept_inbox.empty())
		_mm_pause();
	std::cout << "Accepted" << std::endl;
	auto server_client_sptr = server_sptr->accept_inbox.front();
	server_sptr->accept_inbox.pop_front();
	
	std::cout << "Server_client_sptr->async_write " << sent_by_server.size() << std::endl;
	server_client_sptr->async_write(sent_by_server.data(), sent_by_server.size());
	std::cout << "Msg sent by server" << std::endl;

	while (client_sptr->inbox.empty())
		_mm_pause();	
	std::cout << "Msg rcvd by client with count bytes: " << client_sptr->inbox.size() << std::endl;
	auto stretch_recv = client_sptr->inbox.front_stretch();
	recv_by_client = std::string(std::get<0>(stretch_recv), std::get<1>(stretch_recv));
	client_sptr->inbox.advance_front(std::get<1>(stretch_recv));
	stretch_recv = client_sptr->inbox.front_stretch();
	recv_by_client.append(std::string(std::get<0>(stretch_recv), std::get<1>(stretch_recv)));
	client_sptr->inbox.advance_front(std::get<1>(stretch_recv));
	
	client_sptr->async_write(sent_by_client.data(), sent_by_client.size());
	std::cout << "Reply sent by client" << std::endl;

	while (server_client_sptr->inbox.empty())
		_mm_pause();
	std::cout << "Msg rcvd by server with count byte: " << server_client_sptr->inbox.size() << std::endl;
	auto stretch = server_client_sptr->inbox.front_stretch();
	recv_by_server = std::string(std::get<0>(stretch), std::get<1>(stretch));
	server_client_sptr->inbox.advance_front(std::get<1>(stretch));
	stretch = server_client_sptr->inbox.front_stretch();
	recv_by_server.append(std::string(std::get<0>(stretch), std::get<1>(stretch)));
	server_client_sptr->inbox.advance_front(std::get<1>(stretch));


	ASSERT_ACT(recv_by_server, ==, sent_by_client, return false);
	ASSERT_ACT(recv_by_client, ==, sent_by_server, return false);

	return true;
}

bool test_srv_2_cli_udpm(std::string url)
{
	std::string sent_by_server = "4322431423412412412341243213";
	std::string recv_by_client;

	utttil::io::context ctx;
	ctx.run();
	std::cout << "context running" << std::endl;

	// server
	auto server_sptr = ctx.bind(url);
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	std::cout << "bind done" << std::endl;

	// client
	auto client_sptr = ctx.connect(url);
	ASSERT_ACT(client_sptr, !=, nullptr, return false);
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	std::cout << "connect done" << std::endl;

	std::cout << "server_sptr->async_write " << sent_by_server.size() << std::endl;
	server_sptr->async_write(sent_by_server.data(), sent_by_server.size());
	std::cout << "msg sent by server" << std::endl;

	while (client_sptr->inbox.empty())
		_mm_pause();	
	std::cout << "msg rcvd by client with count bytes: " << client_sptr->inbox.size() << std::endl;
	auto stretch_recv = client_sptr->inbox.front_stretch();
	recv_by_client = std::string(std::get<0>(stretch_recv), std::get<1>(stretch_recv));
	client_sptr->inbox.advance_front(std::get<1>(stretch_recv));
	stretch_recv = client_sptr->inbox.front_stretch();
	recv_by_client.append(std::string(std::get<0>(stretch_recv), std::get<1>(stretch_recv)));
	client_sptr->inbox.advance_front(std::get<1>(stretch_recv));
	
	ASSERT_ACT(recv_by_client, ==, sent_by_server, return false);
	
	return true;
}

bool test_2_ways_msg(std::string url)
{
	Request sent_by_client;
	sent_by_client.type = Request::Type::NewOrder;
	sent_by_client.seq = utttil::max<decltype(sent_by_client.seq)>();
	sent_by_client.account_id = utttil::max<decltype(sent_by_client.account_id)>();
	sent_by_client.req_id = utttil::max<decltype(sent_by_client.req_id)>();
	NewOrder &new_order = sent_by_client.new_order;
	new_order.instrument_id = utttil::max<decltype(new_order.instrument_id)>();
	new_order.is_sell                   = false;
	new_order.is_limit                  = false;
	new_order.is_stop                   = false;
	new_order.participate_dont_initiate = false;
	new_order.time_in_force = TimeInForce::GTD;
	new_order.lot_count      = utttil::max<decltype(new_order.lot_count     )>();
	new_order.pic_count      = utttil::max<decltype(new_order.pic_count     )>();
	new_order.stop_pic_count = utttil::max<decltype(new_order.stop_pic_count)>();

	Request sent_by_server;
	sent_by_server.type = Request::Type::NewOrder;
	sent_by_server.seq = utttil::max<decltype(sent_by_server.seq)>();
	sent_by_server.account_id = utttil::max<decltype(sent_by_server.account_id)>();
	sent_by_server.req_id = utttil::max<decltype(sent_by_server.req_id)>();
	NewOrder &new_order2 = sent_by_server.new_order;
	new_order2.instrument_id = utttil::max<decltype(new_order2.instrument_id)>();
	new_order2.is_sell                   = false;
	new_order2.is_limit                  = false;
	new_order2.is_stop                   = false;
	new_order2.participate_dont_initiate = false;
	new_order2.time_in_force = TimeInForce::GTD;
	new_order2.lot_count      = utttil::max<decltype(new_order2.lot_count     )>();
	new_order2.pic_count      = utttil::max<decltype(new_order2.pic_count     )>();
	new_order2.stop_pic_count = utttil::max<decltype(new_order2.stop_pic_count)>();

	Request recv_by_client;
	Request recv_by_server;

	utttil::io::context<Request> ctx;
	ctx.run();
	
	std::cout << "context running" << std::endl;

	// server
	auto server_sptr = ctx.bind(url);
	if ( !server_sptr)
	{
		std::cerr << "bind failed" << std::endl;
		return false;
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	std::cout << "bind done" << std::endl;

	// client
	auto client_sptr = ctx.connect(url);
	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	std::cout << "connect done" << std::endl;

	std::shared_ptr<utttil::io::peer<Request>> server_client_sptr;

	for (;;)
	{
		if ( ! server_sptr->accept_inbox.empty())
		{
			std::cout << "Accepted" << std::endl;
			server_client_sptr = server_sptr->accept_inbox.front();
			server_sptr->accept_inbox.pop_front();
			break;
		}
	}

	client_sptr->async_send(sent_by_client);
	std::cout << "msg sent by client" << std::endl;

	for (;;)
	{
		if ( ! server_client_sptr->inbox_msg.empty())
		{
			std::cout << "msg rcvd by server" << std::endl;
			Request & data = server_client_sptr->inbox_msg.front();
			recv_by_server = data;
			server_client_sptr->inbox_msg.pop_front();

			server_client_sptr->async_send(sent_by_server);
			break;
		}
	}

	std::cout << "reply sent by client" << std::endl;


	for (;;)
	{
		if ( ! client_sptr->inbox_msg.empty())
		{
			std::cout << "msg rcvd by client" << std::endl;
			Request & data = client_sptr->inbox_msg.front();
			recv_by_client = data;
			client_sptr->inbox_msg.pop_front();
			break;
		}
		_mm_pause();
	}

	ASSERT_ACT(recv_by_server, ==, sent_by_client, return false);
	ASSERT_ACT(recv_by_client, ==, sent_by_server, return false);
	
	return true;
}

int main()
{
	bool success = true
		//&& test("ws://127.0.0.1:1234/")
		&& test_srv_2_cli_udpm ("udpm://226.1.1.1:1000/")
		&& test_2_ways    ( "tcp://127.0.0.1:1002/")
		&& test_2_ways_msg( "tcp://127.0.0.1:1003/")
		;

	return success?0:1;
}