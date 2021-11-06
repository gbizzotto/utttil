
#include "headers.hpp"

#include <memory>
#include <string>
#include "utttil/msg_server.hpp"
#include "utttil/msg_client.hpp"

bool test(std::string url)
{
	std::string sent_by_client = "lkjlkjlkj32lkj42l3kj4l2k3j";
	std::string sent_by_server = "fdsa32431241234124";
	std::string recv_by_client;
	std::string recv_by_server;

	// server
	auto server_sptr = utttil::make_msg_server<std::string,std::string>(url);
	server_sptr->on_message = [&](auto conn_sptr, std::unique_ptr<std::string> msg)
		{
			std::cout << "on message server" << *msg << std::endl;
			recv_by_server = *msg;
			conn_sptr->async_send(sent_by_server);
		};
	server_sptr->async_run();
	std::this_thread::sleep_for(std::chrono::milliseconds(10));

	// client
	auto client_sptr = utttil::make_msg_client<std::string,std::string>(url);
	client_sptr->on_message = [&](auto, std::unique_ptr<std::string> msg)
		{
			std::cout << "on message client " << *msg << std::endl;
			recv_by_client = *msg;
		};
	client_sptr->async_run();
	client_sptr->async_send(sent_by_client);
	std::this_thread::sleep_for(std::chrono::milliseconds(10));

	// shutdown
	client_sptr->close();
	server_sptr->close();

	server_sptr->join();
	client_sptr->join();

	// check
	return recv_by_server == sent_by_client
	    && recv_by_client == sent_by_server;
}

int main()
{
	bool success = test("ws://127.0.0.1:1234/")
		&& test("tcp://127.0.0.1:1234/")
		;

	return success?0:1;
}