
#include "headers.hpp"

#include <iostream>
#include <memory>
#include <string>
#include <atomic>

#include "utttil/iou.hpp"


bool test(std::string url)
{
	std::string sent_by_client = "32jk1hkjh1k3j4h62kj345h6345kljh345kj7h";
	std::string sent_by_server = "4322431423412412412341243213";
	std::string recv_by_client;
	std::string recv_by_server;

	std::atomic_bool done = false;

	utttil::iou::context ctx;
	ctx.run();
	
	std::cout << "context running" << std::endl;

	// server
	auto server_sptr = ctx.bind(url);
	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	std::cout << "bind done" << std::endl;

	// client
	auto client_sptr = ctx.connect(url);
	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	std::cout << "connect done" << std::endl;

	std::shared_ptr<utttil::iou::peer> server_client_sptr;

	for (;;)
	{
		if ( ! server_sptr->accepted.empty())
		{
			std::cout << "Accepted" << std::endl;
			server_client_sptr = server_sptr->accepted.front();
			server_sptr->accepted.pop_front();
			break;
		}
	}

	{
		std::vector<char> v(sent_by_client.begin(), sent_by_client.end());
		client_sptr->async_write(std::move(v));
	}

	std::cout << "msg sent by client" << std::endl;

	for (;;)
	{
		if ( ! server_client_sptr->inbox.empty())
		{
			std::cout << "msg rcvd by server" << std::endl;
			auto & data = server_client_sptr->inbox.front();
			recv_by_server = std::string(&data[0], &data[data.size()]);
			server_client_sptr->inbox.pop_front();

			std::vector<char> v(sent_by_server.begin(), sent_by_server.end());
			server_client_sptr->async_write(std::move(v));
			break;
		}
	}

	std::cout << "reply sent by client" << std::endl;


	for (;;)
	{
		if ( ! client_sptr->inbox.empty())
		{
			std::cout << "msg rcvd by client" << std::endl;
			auto & data = client_sptr->inbox.front();
			recv_by_client = std::string(&data[0], &data[data.size()]);
			client_sptr->inbox.pop_front();
			break;
		}
	}

	return recv_by_server == sent_by_client
		&& recv_by_client == sent_by_server
		;
}

int main()
{
	bool success = true
		//&& test("ws://127.0.0.1:1234/")
		&& test("tcp://127.0.0.1:1234/")
		;

	return success?0:1;
}