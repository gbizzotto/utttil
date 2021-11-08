
#include "headers.hpp"

#include <iostream>
#include <memory>
#include <string>
#include <atomic>

#include "utttil/io.hpp"

struct MyData
{
	int value;
	std::string str;

	template<typename Serializer>
	void serialize(Serializer && s) const
	{
		s << value
		  << str
		  ;
	}
	template<typename Deserializer>
	void deserialize(Deserializer && s)
	{
		s >> value
		  >> str
		  ;
	}
};
inline bool operator==(const MyData & left, const MyData & right)
{
	return left.value == right.value
		&& left.str   == right.str
		;
}
inline std::ostream & operator<<(std::ostream & out, const MyData & d)
{
	return out << d.value << ", " << d.str;
}


bool test(std::string url)
{
	MyData sent_by_client = {1234, "lkjlkjlkj32lkj42l3kj4l2k3j"};
	MyData sent_by_server = {9876, "fdsa32431241234124"};
	MyData recv_by_client;
	MyData recv_by_server;

	std::atomic_bool done = false;

	utttil::io::context ctx;
	ctx.async_run();

	// server
	auto srv_on_message = [&](auto conn_sptr, std::unique_ptr<MyData> msg)
		{
			recv_by_server = *msg;
			conn_sptr->async_send(sent_by_server);
		};
	auto server_sptr = ctx.bind_srlz<MyData,MyData>(url, nullptr, srv_on_message);

	// client
	auto cli_on_connect = [&](auto conn_sptr)
		{
			conn_sptr->async_send(sent_by_client);
		};
	auto cli_on_message = [&](auto conn_sptr, std::unique_ptr<MyData> msg)
		{
			recv_by_client = *msg;
			done = true;
		};
	auto client_sptr = ctx.connect_srlz<MyData,MyData>(url, cli_on_connect, cli_on_message);

	while( ! done)
	{}

	// check
	return recv_by_server == sent_by_client
	    && recv_by_client == sent_by_server;
}

int main()
{
	bool success = true
		&& test("ws://127.0.0.1:1234/")
		&& test("tcp://127.0.0.1:1234/")
		;

	return success?0:1;
}