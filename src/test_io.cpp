
#include "headers.hpp"

#include <iostream>
#include <memory>
#include <string>
#include <atomic>

#include "utttil/io.hpp"


bool test(std::string url)
{
	std::string sent_by_client = "32jk1hkjh1k3j4h62kj345h6345kljh345kj7h";
	std::string sent_by_server = "4322431423412412412341243213";
	std::string recv_by_client;
	std::string recv_by_server;

	std::atomic_bool done = false;

	utttil::io::context ctx;
	ctx.async_run();
	
	// server
	auto srv_on_message = [&](auto conn_sptr)
		{
			auto & data = conn_sptr->recv_buffer;
			recv_by_server = std::string(&data[0], &data[data.size()]);
			std::vector<char> v(sent_by_server.begin(), sent_by_server.end());
			conn_sptr->async_write(std::move(v));
		};
	auto server_sptr = ctx.bind(url, nullptr, srv_on_message);

	// client
	auto cli_on_connect = [&](auto conn_sptr)
		{
			std::vector<char> v(sent_by_client.begin(), sent_by_client.end());
			conn_sptr->async_write(std::move(v));
		};
	auto cli_on_message = [&](auto conn_sptr)
		{
			auto & data = conn_sptr->recv_buffer;
			recv_by_client = std::string(&data[0], &data[data.size()]);
			done = true;
		};
	auto client_sptr = ctx.connect(url, cli_on_connect, cli_on_message);

	while( ! done)
	{}

	return recv_by_server == sent_by_client
		&& recv_by_client == sent_by_server
		;
}

int main()
{
	bool success = true
		&& test("ws://127.0.0.1:1234/")
		&& test("tcp://127.0.0.1:1234/")
		;

	return success?0:1;
}