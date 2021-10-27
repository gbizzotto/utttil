#include "io/interface.hpp"
#include "io/tcp_socket.hpp"
#include "io/tcp_server.hpp"
#include "io/ws_server.hpp"
#include "io/ws_socket.hpp"

namespace utttil {
namespace io {

template<typename CustomData=int>
std::shared_ptr<interface<CustomData>> connect(const utttil::url url)
{
	if (url.protocol == "tcp")
	{
		auto socket = make_tcp_socket<CustomData>();
		socket->open(url.host.c_str(), url.port.c_str());
		return socket;
	}
	else if (url.protocol == "ws")
	{
		auto socket = make_ws_socket<CustomData>();
		socket->open(url.host.c_str(), url.port.c_str(), url.location.c_str(), 1);
		return socket;
	}
	return make_null_io();
}

template<typename CustomData=int>
std::shared_ptr<interface<CustomData>> bind(const utttil::url url)
{
	if (url.protocol == "tcp")
	{
		auto server = make_tcp_server<CustomData>(std::stoi(url.port));
		server->async_accept();
		return server;
	}
	else if (url.protocol == "ws")
	{
		auto server = make_ws_server<CustomData>(std::stoi(url.port));
		server->listen();
		return server;
	}
	return make_null_io();
}

}} // namespace
