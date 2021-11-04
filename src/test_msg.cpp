
#include "headers.hpp"

#include <memory>
#include <string>
#include "utttil/msg_server.hpp"
#include "utttil/msg_client.hpp"

std::string sent_str = "lkjlkjlkj32lkj42l3kj4l2k3j";
std::string recv_str;

auto server(utttil::url url)
{
	auto server_sptr = utttil::make_msg_server<std::string,std::string>(url);
	server_sptr->on_message = [](typename decltype(server_sptr)::element_type::ConnectionSPTR conn_sptr, std::unique_ptr<std::string> msg)
		{
			std::cout << "on message " << *msg << std::endl;
			recv_str = *msg;
		};
	server_sptr->async_run();
	return server_sptr;
}

auto client(utttil::url url)
{
	auto client_sptr = utttil::make_msg_client<std::string,std::string>(url);
	client_sptr->async_run();
	std::this_thread::sleep_for(std::chrono::milliseconds(10));
	client_sptr->async_send(sent_str);
	return client_sptr;
}

bool test(std::string url)
{
	auto server_sptr = server(url);
	std::this_thread::sleep_for(std::chrono::milliseconds(10));

	auto client_sptr = client(url);
	std::this_thread::sleep_for(std::chrono::milliseconds(10));

	client_sptr->close();
	server_sptr->close();

	server_sptr->join();
	client_sptr->join();

	std::cout << recv_str << std::endl;
	return  recv_str == sent_str;
}

int main()
{
	bool success = test("ws://127.0.0.1:1234/")
		//&& test("tcp://127.0.0.1:1234/")
		;

	return success?0:1;
}