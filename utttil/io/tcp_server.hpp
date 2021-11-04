
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

	tcp_server(int port, std::shared_ptr<boost::asio::io_context> context = nullptr)
		: interface<CustomData>(context)
		, acceptor(*this->io_context, tcp::endpoint(tcp::v4(), port))
	{}
	virtual bool is_null_io() const override { return false; }
	
	virtual void close() override
	{
		if (acceptor.is_open())
			acceptor.close();
	}

	void async_accept()
	{
		auto new_connection_sptr = std::make_shared<Connection>(this->io_context);
		acceptor.async_accept(new_connection_sptr->get_socket(), [new_connection_sptr,this](const boost::system::error_code & error)
			{
				if ( ! error)
				{
					new_connection_sptr->on_connect = this->on_connect;
					new_connection_sptr->on_close   = this->on_close  ;
					new_connection_sptr->on_message = this->on_message;
					this->on_connect(new_connection_sptr);
					new_connection_sptr->async_read();
					async_accept();
				}
			});
	}
};

template<typename CustomData=int>
std::shared_ptr<tcp_server<CustomData>> make_tcp_server(int port, std::shared_ptr<boost::asio::io_context> context = nullptr)
{
	return std::make_shared<tcp_server<CustomData>>(port, context);
}

}} // namespace
