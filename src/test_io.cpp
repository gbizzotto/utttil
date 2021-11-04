
#include "headers.hpp"

#include <boost/asio.hpp>
#include <thread>
#include <iostream>
#include <memory>

#include "utttil/io.hpp"

std::string sent_str = "32jk1hkjh1k3j4h62kj345h6345kljh345kj7h";
std::string recv_str;

auto server(utttil::url url)
{
	auto server_sptr = utttil::io::bind(url);
	server_sptr->on_message = [](typename decltype(server_sptr)::element_type::ConnectionSPTR socket)
		{
			auto & data = socket->recv_buffer;
			std::cout << "on message " << std::string(&data[0], &data[data.size()]) << std::endl;
			recv_str = std::string(&data[0], &data[data.size()]);
		};
	server_sptr->async_run();
	return server_sptr;
}

auto client(utttil::url url)
{
	auto client_sptr = utttil::io::connect(url);
	client_sptr->async_run();

	std::vector<char> v(sent_str.begin(), sent_str.end());
	client_sptr->async_write(std::move(v));
	return client_sptr;
}

bool test(std::string url)
{
	auto server_sptr = server(url);
	std::this_thread::sleep_for(std::chrono::milliseconds(1));

	auto client_sptr = client(url);
	std::this_thread::sleep_for(std::chrono::milliseconds(1));

	server_sptr->stop();
	client_sptr->stop();

	server_sptr->join();
	client_sptr->join();

	std::cout << recv_str << std::endl;
	return  recv_str == sent_str;
}

int main()
{
	bool success = test("ws://127.0.0.1:1234/")
		&& test("tcp://127.0.0.1:1234/");

	return success?0:1;
}