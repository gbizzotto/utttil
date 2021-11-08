
#pragma once

#include "headers.hpp"

#include <functional>
#include <memory>
#include <deque>
#include <vector>

#include <boost/asio.hpp>

#include "interface.hpp"
#include "tcp_socket.hpp"

using boost::asio::ip::tcp;

namespace utttil{
namespace io {

template<typename CustomData=int>
struct tcp_server : interface<CustomData>
{
	using ConnectionData = CustomData;
	using Connection     = tcp_socket<CustomData>;
	using ConnectionSPTR = typename Connection::ConnectionSPTR;

	tcp::acceptor acceptor;

	tcp_server(boost::asio::io_context & context, int port)
		: interface<CustomData>(context)
		, acceptor(context, tcp::endpoint(tcp::v4(), port))
	{
		this->async_accept();
	}
	
	virtual void close() override
	{
		boost::system::error_code ec;
		acceptor.close(ec);
	}

	void async_accept()
	{
		auto new_connection_sptr = std::make_shared<Connection>(this->io_context);
		acceptor.async_accept(new_connection_sptr->get_socket(), [new_connection_sptr,this](const boost::system::error_code & error)
			{
				if (error)
					return;

				new_connection_sptr->on_connect = this->on_connect;
				new_connection_sptr->on_close   = this->on_close  ;
				new_connection_sptr->on_message = this->on_message;
				this->on_connect(new_connection_sptr);
				new_connection_sptr->async_read();
				async_accept();
			});
	}
};

template<typename CustomData=int>
std::shared_ptr<tcp_server<CustomData>> make_tcp_server(boost::asio::io_context & context, int port)
{
	return std::make_shared<tcp_server<CustomData>>(context, port);
}

}} // namespace
