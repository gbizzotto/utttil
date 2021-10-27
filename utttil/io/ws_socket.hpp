#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

#include "interface.hpp"

namespace utttil {
namespace io {

template<typename CustomData=int>
struct ws_socket : interface<CustomData>
{
	using ConnectionData = CustomData;

    boost::beast::websocket::stream<boost::beast::tcp_stream> stream;

	utttil::synchronized<std::deque<std::vector<char>>> send_buffers;

	tcp::resolver resolver;
	
	ws_socket(std::shared_ptr<boost::asio::io_context> & context)
		: interface<CustomData>(context)
		, stream(*this->io_context)
		, resolver(*this->io_context)
	{
		this->recv_buffer.reserve(1500);
	}
	ws_socket(std::shared_ptr<boost::asio::io_context> && context)
		: interface<CustomData>(context)
		, stream(*this->io_context)
		, resolver(*this->io_context)
	{
		this->recv_buffer.reserve(1500);
	}
	ws_socket()
		: stream(*this->io_context)
		, resolver(*this->io_context)
	{
		this->recv_buffer.reserve(1500);
	}
	virtual bool is_null_io() const override { return false; }

	auto & get_socket()
	{
		return stream;
	}

	bool open(const char * host, const char * port, char const * target, int version)
	{
		boost::system::error_code ec;
		std::string host_ = host;
		std::string target_ = target;
    	auto this_sptr = std::static_pointer_cast<ws_socket<CustomData>>(this->shared_from_this());
		auto endpoints = resolver.resolve(host, port, ec);

        if(ec) {
            std::cout << "client couldnt resolve" << std::endl;
            return false;
        }

        // Set the timeout for the operation
        boost::beast::get_lowest_layer(this_sptr->stream).expires_after(std::chrono::seconds(30));

        // Make the connection on the IP address we get from a lookup
        boost::beast::get_lowest_layer(this_sptr->stream).connect(endpoints, ec);

        if(ec) {
            std::cout << "client couldnt connect" << std::endl;
            return false;
        }

        // Turn off the timeout on the tcp_stream, because
        // the websocket stream has its own timeout system.
        boost::beast::get_lowest_layer(this_sptr->stream).expires_never();

        // Set suggested timeout settings for the websocket
        this_sptr->stream.set_option(boost::beast::websocket::stream_base::timeout::suggested(boost::beast::role_type::client));

        // Set a decorator to change the User-Agent of the handshake
        this_sptr->stream.set_option(boost::beast::websocket::stream_base::decorator(
            [](boost::beast::websocket::request_type &req)
            {
                req.set(boost::beast::http::field::user_agent, std::string(BOOST_BEAST_VERSION_STRING) + " utttil");
            }));

        // Update the host_ string. This will provide the value of the
        // Host HTTP header during the WebSocket handshake.
        // See https://tools.ietf.org/html/rfc7230#section-5.4
        host_.append(":").append(port);

        // Perform the websocket handshake
        boost::beast::websocket::response_type res;
        this_sptr->stream.handshake(res, host_, target_, ec);
        if(ec) {
            std::cout << "client couldnt handshake because " << ec.message() << std::endl;
            return false;
        }
        this_sptr->async_read();
        return true;
	}

	void async_open(const char * host, const char * port, char const * target, int version)
	{
		std::string host_ = host;
		std::string target_ = target;
    	auto this_sptr = std::static_pointer_cast<ws_socket<CustomData>>(this->shared_from_this());
		resolver.async_resolve(host, port,
			[this_sptr,host_,target_](boost::beast::error_code ec, boost::asio::ip::tcp::resolver::results_type results) mutable
			{
		        if(ec)
		            std::cout << "client couldnt resolve" << std::endl;

		        // Set the timeout for the operation
		        boost::beast::get_lowest_layer(this_sptr->stream).expires_after(std::chrono::seconds(30));

		        // Make the connection on the IP address we get from a lookup
		        boost::beast::get_lowest_layer(this_sptr->stream).async_connect(results,
		        	[this_sptr,host_,target_](boost::beast::error_code ec, boost::asio::ip::tcp::resolver::results_type::endpoint_type ep) mutable
		        	{
				        if(ec)
				            std::cout << "client couldnt connect" << std::endl;

				        // Turn off the timeout on the tcp_stream, because
				        // the websocket stream has its own timeout system.
				        boost::beast::get_lowest_layer(this_sptr->stream).expires_never();

				        // Set suggested timeout settings for the websocket
				        this_sptr->stream.set_option(boost::beast::websocket::stream_base::timeout::suggested(boost::beast::role_type::client));

				        // Set a decorator to change the User-Agent of the handshake
				        this_sptr->stream.set_option(boost::beast::websocket::stream_base::decorator(
				            [](boost::beast::websocket::request_type &req)
				            {
				                req.set(boost::beast::http::field::user_agent, std::string(BOOST_BEAST_VERSION_STRING) + " utttil");
				            }));

				        // Update the host_ string. This will provide the value of the
				        // Host HTTP header during the WebSocket handshake.
				        // See https://tools.ietf.org/html/rfc7230#section-5.4
				        host_.append(":").append(std::to_string(ep.port()));

				        // Perform the websocket handshake
				        this_sptr->stream.async_handshake(host_, target_,
				        	[this_sptr](boost::beast::error_code ec) mutable
				        	{
						        if(ec)
						            std::cout << "client couldnt handshake because " << ec.message() << std::endl;
						        this_sptr->async_read();
				        	});
		        	});
			});
	}

	void close(std::string reason)
	{
		stream.close(boost::beast::websocket::close_reason(reason));
		this->on_close(std::static_pointer_cast<ws_socket>(this->shared_from_this()));
	}

	void async_read()
	{
		size_t available_size = 1500 - this->recv_buffer.size();
		char * ok = &this->recv_buffer[this->recv_buffer.size()];
		this->recv_buffer.resize(1500);
		stream.async_read_some(
				boost::asio::buffer(ok, available_size),
				boost::bind(&ws_socket::handle_read, std::static_pointer_cast<ws_socket>(this->shared_from_this()),
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred,
					available_size)
			);
	}
	void handle_read(const boost::system::error_code & error, size_t bytes_transferred, size_t expected)
	{
		if (error)
		{
			std::cout << "client err" << std::endl;
			close(error.message());
			return;
		}
		this->recv_buffer.resize(1500 - expected + bytes_transferred);
		if (bytes_transferred)
			this->on_message(std::static_pointer_cast<ws_socket>(this->shared_from_this()));
		async_read();
	}

	void async_write(std::vector<char> && data)
	{
		std::vector<char> * owned_buffer;
		{
			auto send_buffers_proxy = send_buffers.lock();
			send_buffers_proxy->emplace_back(std::move(data));
			owned_buffer = & send_buffers_proxy->back();
		}
		stream.async_write(
			boost::asio::buffer(owned_buffer->data(), owned_buffer->size()),
			boost::bind(&ws_socket::handle_write, std::static_pointer_cast<ws_socket>(this->shared_from_this()),
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));
	}
	void handle_write(const boost::beast::error_code & error, size_t bytes_transferred)
	{
		if (error)
		{
			close(error.message());
			return;
		}
		auto send_buffers_proxy = send_buffers.lock();
		std::vector<char> & owned_buffer = send_buffers_proxy->front();
		assert(bytes_transferred == owned_buffer.size());
		send_buffers_proxy->pop_front();
	}

};

template<typename CustomData=int>
std::shared_ptr<ws_socket<CustomData>> make_ws_socket()
{ return std::make_shared<ws_socket<CustomData>>(); }

template<typename CustomData=int>
std::shared_ptr<ws_socket<CustomData>> make_ws_socket(std::shared_ptr<boost::asio::io_context> & context)
{ return std::make_shared<ws_socket<CustomData>>(context); }

template<typename CustomData=int>
std::shared_ptr<ws_socket<CustomData>> make_ws_socket(std::shared_ptr<boost::asio::io_context> && context)
{ return std::make_shared<ws_socket<CustomData>>(context); }


}} // namespace
