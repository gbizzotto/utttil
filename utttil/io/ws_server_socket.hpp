
#pragma once

#include "headers.hpp"

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

#include "ws_socket.hpp"

namespace utttil {
namespace io {

template<typename CustomData=int>
struct ws_server_socket : ws_socket<CustomData>
{
	using ConnectionData = CustomData;

	ws_server_socket(std::shared_ptr<boost::asio::io_context> io_context = nullptr)
		: ws_socket<CustomData>(io_context)
	{}
	void init()
	{
		auto this_sptr = std::static_pointer_cast<ws_server_socket>(this->shared_from_this());
		boost::asio::dispatch(this->stream.get_executor(),[this_sptr]()
			{
			        // Set suggested timeout settings for the websocket
			        this_sptr->stream.set_option(boost::beast::websocket::stream_base::timeout::suggested(boost::beast::role_type::server));
			        // Set a decorator to change the Server of the handshake
			        this_sptr->stream.set_option(boost::beast::websocket::stream_base::decorator(
			            [](boost::beast::websocket::response_type& res)
			            {
			                res.set(boost::beast::http::field::server, std::string(BOOST_BEAST_VERSION_STRING) + " ttt");
			            }));
			        this_sptr->async_accept();
			});
	}

	void async_accept()
	{
		auto this_sptr = std::static_pointer_cast<ws_server_socket>(this->shared_from_this());
		this->stream.async_accept([this_sptr](boost::beast::error_code const & ec)
			{
				//if(ec)
				//	std::cout << "server_socket error: " << ec.message() << std::endl;
				if (ec != boost::beast::websocket::error::closed)
					this_sptr->async_read();
			});
	}
};

}} // namespace