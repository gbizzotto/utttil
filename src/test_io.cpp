
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
	ctx.start_all();
	std::cout << "Context running" << std::endl;

	// server
	auto server_sptr = ctx.bind_raw(url);
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	ASSERT_ACT(server_sptr, !=, nullptr, return false);
	ASSERT_ACT(server_sptr->good(), ==, true, return false);
	std::cout << "Bind done" << std::endl;

	std::cout << "Sizeof(*server_sptr): " << sizeof(*server_sptr) << std::endl;

	// client
	auto client_sptr = ctx.connect_raw(url);
	ASSERT_ACT(client_sptr, !=, nullptr, return false);
	ASSERT_ACT(client_sptr->good(), ==, true, return false);
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	std::cout << "Connect done" << std::endl;

	std::cout << "Sizeof(*client_sptr): " << sizeof(*client_sptr) << std::endl;

	while (server_sptr->get_accept_inbox()->empty())
		_mm_pause();
	std::cout << "Accepted" << std::endl;
	auto server_client_sptr = server_sptr->get_accept_inbox()->front();
	server_sptr->get_accept_inbox()->pop_front();
	
	std::cout << "Server_client_sptr->async_write " << sent_by_server.size() << std::endl;
	server_client_sptr->async_write(sent_by_server.data(), sent_by_server.size());
	std::cout << "Msg sent by server" << std::endl;

	while (client_sptr->get_inbox()->empty())
		_mm_pause();	
	std::cout << "Msg rcvd by client with count bytes: " << client_sptr->get_inbox()->size() << std::endl;
	auto stretch_recv = client_sptr->get_inbox()->front_stretch();
	recv_by_client = std::string(std::get<0>(stretch_recv), std::get<1>(stretch_recv));
	client_sptr->get_inbox()->advance_front(std::get<1>(stretch_recv));
	stretch_recv = client_sptr->get_inbox()->front_stretch();
	recv_by_client.append(std::string(std::get<0>(stretch_recv), std::get<1>(stretch_recv)));
	client_sptr->get_inbox()->advance_front(std::get<1>(stretch_recv));
	
	client_sptr->async_write(sent_by_client.data(), sent_by_client.size());
	std::cout << "Reply sent by client" << std::endl;

	while (server_client_sptr->get_inbox()->empty())
		_mm_pause();
	std::cout << "Msg rcvd by server with count byte: " << server_client_sptr->get_inbox()->size() << std::endl;
	auto stretch = server_client_sptr->get_inbox()->front_stretch();
	recv_by_server = std::string(std::get<0>(stretch), std::get<1>(stretch));
	server_client_sptr->get_inbox()->advance_front(std::get<1>(stretch));
	stretch = server_client_sptr->get_inbox()->front_stretch();
	recv_by_server.append(std::string(std::get<0>(stretch), std::get<1>(stretch)));
	server_client_sptr->get_inbox()->advance_front(std::get<1>(stretch));

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
	auto server_sptr = ctx.bind_msg<std::string,std::string>(url);
	ASSERT_ACT(server_sptr, !=, nullptr, return false);

	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	std::cout << "bind done" << std::endl;

	// client
	auto client_sptr = ctx.connect_msg<std::string,std::string>(url);
	ASSERT_ACT(client_sptr, !=, nullptr, return false);
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	std::cout << "connect done" << std::endl;

	std::cout << "server_sptr->async_send " << sent_by_server.size() << std::endl;
	server_sptr->async_send(sent_by_server);
	std::cout << "msg sent by server" << std::endl;

	while (client_sptr->get_inbox_msg()->empty())
		_mm_pause();	
	std::cout << "msg rcvd by client" << std::endl;
	recv_by_client = client_sptr->get_inbox_msg()->front();
	
	ASSERT_ACT(recv_by_client, ==, sent_by_server, return false);
	
	return true;
}

bool test_srv_2_cli_udpmr(std::string url, std::string url_replay)
{
	Request sent_by_server;
	sent_by_server.type = Request::Type::NewOrder;
	sent_by_server.seq = 0;
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

	utttil::io::context ctx;
	ctx.run();
	std::cout << "context running" << std::endl;

	// server
	auto server_sptr = ctx.bind_msg<Request,Request>(url, url_replay);
	ASSERT_ACT(server_sptr, !=, nullptr, return false);
	std::cout << "bind done" << std::endl;

	// set up a gap
	server_sptr->get_outbox_msg()->back() = sent_by_server;
	server_sptr->get_outbox_msg()->advance_back();
	sent_by_server.seq = 1;

	// client
	std::this_thread::sleep_for(std::chrono::milliseconds(100)); // let's miss the 1st msg
	auto client_sptr = ctx.connect_msg<Request,Request>(url, url_replay);
	ASSERT_ACT(client_sptr, !=, nullptr, return false);
	std::cout << "connect done" << std::endl;

	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	server_sptr->get_outbox_msg()->back() = sent_by_server;
	server_sptr->get_outbox_msg()->advance_back();
	std::cout << __func__ << " msg sent by server" << std::endl;

	while (client_sptr->get_inbox_msg()->empty())
		_mm_pause();	
	recv_by_client = client_sptr->get_inbox_msg()->front();
	std::cout << "msg rcvd by client #" << recv_by_client.get_seq() << std::endl;
	client_sptr->get_inbox_msg()->pop_front();
	while (client_sptr->get_inbox_msg()->empty())
		_mm_pause();	
	recv_by_client = client_sptr->get_inbox_msg()->front();
	std::cout << "msg rcvd by client #" << recv_by_client.get_seq() << std::endl;
	client_sptr->get_inbox_msg()->pop_front();
	
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

	utttil::io::context ctx;
	ctx.start_all();
	
	std::cout << "context running" << std::endl;

	// server
	auto server_sptr = ctx.bind_msg<Request,Request>(url);
	if ( !server_sptr)
	{
		std::cerr << "bind failed" << std::endl;
		return false;
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	std::cout << "bind done" << std::endl;

	// client
	auto client_sptr = ctx.connect_msg<Request,Request>(url);
	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	std::cout << "connect done" << std::endl;

	std::shared_ptr<utttil::io::peer_msg<Request,Request>> server_client_sptr;

	while(server_sptr->good())
	{
		if ( ! server_sptr->get_accept_inbox()->empty())
		{
			std::cout << "Accepted" << std::endl;
			server_client_sptr = server_sptr->get_accept_inbox()->front();
			server_sptr->get_accept_inbox()->pop_front();
			break;
		}
	}

	client_sptr->async_send(sent_by_client);
	std::cout << "msg sent by client" << std::endl;

	while(server_client_sptr->good())
	{
		if ( ! server_client_sptr->get_inbox_msg()->empty())
		{
			std::cout << "msg rcvd by server" << std::endl;
			Request & data = server_client_sptr->get_inbox_msg()->front();
			recv_by_server = data;
			server_client_sptr->get_inbox_msg()->pop_front();

			server_client_sptr->async_send(sent_by_server);
			break;
		}
	}

	std::cout << "reply sent by client" << std::endl;


	while(client_sptr->good())
	{
		if ( ! client_sptr->get_inbox_msg()->empty())
		{
			std::cout << "msg rcvd by client" << std::endl;
			Request & data = client_sptr->get_inbox_msg()->front();
			recv_by_client = data;
			client_sptr->get_inbox_msg()->pop_front();
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
		&& test_srv_2_cli_udpm("udpm://226.1.1.1:2000/")
		&& test_srv_2_cli_udpmr("udpmr://226.1.1.1:2004/", "tcp://127.0.0.1:2005")
		&& test_2_ways    ( "tcp://127.0.0.1:2002/")
		&& test_2_ways_msg( "tcp://127.0.0.1:2003/")
		;

	return success?0:1;
}