
#pragma once

#include "utttil/io/ws_server_socket.hpp"
#include "utttil/io/interface.hpp"

namespace utttil {
namespace io {

template<typename CustomData=int>
struct ws_server : interface<CustomData>
{
	using ConnectionData = CustomData;
	using Connection     = ws_server_socket<CustomData>;
	using ConnectionSPTR = typename Connection::ConnectionSPTR;

	tcp::acceptor acceptor;

	ws_server(boost::asio::io_context & context, int port)
		: interface<CustomData>(context)
		, acceptor(context, tcp::endpoint(tcp::v4(), port))
	{
		acceptor.set_option(boost::asio::socket_base::reuse_address(true));
	}
	
	virtual void close() override
	{
		boost::beast::error_code ec;
		this->acceptor.close(ec);
		this->on_close(this->shared_from_this());
	}
	
	void listen()
	{
		this->acceptor.listen(boost::asio::socket_base::max_listen_connections);
		this->async_accept();
	}

	void async_accept()
	{
		auto new_connection_sptr = std::make_shared<Connection>(this->io_context);
		acceptor.async_accept(new_connection_sptr->get_tcp_socket(), [new_connection_sptr,this](const boost::system::error_code & error)
			{
				if (error)
					return;

				new_connection_sptr->on_connect = this->on_connect;
				new_connection_sptr->on_close   = this->on_close  ;
				new_connection_sptr->on_message = this->on_message;
				this->on_connect(new_connection_sptr);
				new_connection_sptr->init();
				async_accept();
			});
	}
};

template<typename CustomData=int>
std::shared_ptr<ws_server<CustomData>> make_ws_server(boost::asio::io_context & context, int port)
{
	return std::make_shared<ws_server<CustomData>>(context, port);
}

}} // namespace
