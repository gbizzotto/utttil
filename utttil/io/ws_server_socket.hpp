
#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

#include "interface.hpp"

namespace utttil {
namespace io {

template<typename CustomData=int>
struct ws_server_socket : interface<CustomData>
{
	using ConnectionData = CustomData;

	boost::beast::websocket::stream<boost::beast::tcp_stream> socket;
	utttil::synchronized<std::deque<std::vector<char>>> send_buffers;

	ws_server_socket(std::shared_ptr<boost::asio::io_context> & context)
		: interface<CustomData>(context)
		, socket(*interface<CustomData>::io_context)
	{}
	ws_server_socket(std::shared_ptr<boost::asio::io_context> && context)
		: interface<CustomData>(context)
		, socket(*interface<CustomData>::io_context)
	{}
	ws_server_socket()
		: socket(*this->io_context)
	{}
	void init()
	{
		this->recv_buffer.reserve(1500);

		auto this_sptr = std::static_pointer_cast<ws_server_socket>(this->shared_from_this());
		boost::asio::dispatch(socket.get_executor(),[this_sptr]()
			{
			        // Set suggested timeout settings for the websocket
			        this_sptr->socket.set_option(boost::beast::websocket::stream_base::timeout::suggested(boost::beast::role_type::server));
			        // Set a decorator to change the Server of the handshake
			        this_sptr->socket.set_option(boost::beast::websocket::stream_base::decorator(
			            [](boost::beast::websocket::response_type& res)
			            {
			                res.set(boost::beast::http::field::server, std::string(BOOST_BEAST_VERSION_STRING) + " ttt");
			            }));
			        this_sptr->async_accept();
			});
	}
	virtual bool is_null_io() const override { return false; }

	boost::asio::ip::tcp::socket & get_tcp_socket()
	{
		return socket.next_layer().socket();
	}

	void async_accept()
	{
		auto this_sptr = std::static_pointer_cast<ws_server_socket>(this->shared_from_this());
		socket.async_accept([this_sptr](boost::beast::error_code const & ec)
			{
				if(ec)
					std::cout << "server_socket error: " << ec.message() << std::endl;
				this_sptr->async_read();
			});
	}

	void async_read()
	{
		size_t available_size = 1500 - this->recv_buffer.size();
		char * ok = &this->recv_buffer[this->recv_buffer.size()];
		this->recv_buffer.resize(1500);
		socket.async_read_some(
				boost::asio::buffer(ok, available_size),
				boost::bind(&ws_server_socket::handle_read, std::static_pointer_cast<ws_server_socket>(this->shared_from_this()),
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred,
					available_size)
			);
	}
	void handle_read(boost::beast::error_code ec, std::size_t bytes_transferred, size_t expected)
	{
		this->recv_buffer.resize(1500 - expected + bytes_transferred);
		if (bytes_transferred)
			this->on_message(std::static_pointer_cast<ws_server_socket>(this->shared_from_this()));
		if(ec == boost::beast::websocket::error::closed)
		{
			this->on_close(std::static_pointer_cast<ws_server_socket>(this->shared_from_this()));
			return;
		}
		async_read();
	}
};

}} // namespace