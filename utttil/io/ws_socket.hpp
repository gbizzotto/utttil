
#pragma once

#include "headers.hpp"

#include <iostream>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

#include "interface.hpp"

namespace utttil {
namespace io {

template<typename T>
struct async_write_callback_ws_t
{
	std::vector<char> data;
	T t;
	async_write_callback_ws_t(std::vector<char> && d, T t_)
		: data(std::move(d))
		, t(t_)
	{}
	void operator()(const boost::system::error_code & error, size_t bytes_transferred)
	{
		if (error)
		{
			if (error != boost::beast::websocket::error::closed)
				t->close();
			else
				t->on_close(t);
			return;
		}
	};
};


template<typename CustomData=int>
struct ws_socket : interface<CustomData>
{
	using ConnectionData = CustomData;

	boost::beast::websocket::stream<boost::beast::tcp_stream> stream;
	
	ws_socket(boost::asio::io_context & context)
		: interface<CustomData>(context)
		, stream(context)
	{
		this->recv_buffer.reserve(1500);
	}

	virtual void close() override
	{
		boost::beast::error_code ec;
		this->stream.close(boost::beast::websocket::close_reason(), ec);
		this->on_close(this->shared_from_this());
	}

	boost::asio::ip::tcp::socket & get_tcp_socket()
	{
		return this->stream.next_layer().socket();
	}

	bool open(const char * host, const char * port, char const * target, int version)
	{
		boost::system::error_code ec;
		std::string host_ = host;
		std::string target_ = target;

		tcp::resolver resolver(this->io_context);
		auto endpoints = resolver.resolve(host, port, ec);

		if(ec) {
			//std::cout << "client couldnt resolve" << std::endl;
			return false;
		}

		// Set the timeout for the operation
		boost::beast::get_lowest_layer(this->stream).expires_after(std::chrono::seconds(30));

		// Make the connection on the IP address we get from a lookup
		boost::beast::get_lowest_layer(this->stream).connect(endpoints, ec);

		if(ec) {
			//std::cout << "client couldnt connect" << std::endl;
			return false;
		}

		// Turn off the timeout on the tcp_stream, because
		// the websocket stream has its own timeout system.
		boost::beast::get_lowest_layer(this->stream).expires_never();

		// Set suggested timeout settings for the websocket
		this->stream.set_option(boost::beast::websocket::stream_base::timeout::suggested(boost::beast::role_type::client));

		// Set a decorator to change the User-Agent of the handshake
		this->stream.set_option(boost::beast::websocket::stream_base::decorator(
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
		this->stream.handshake(res, host_, target_, ec);
		if(ec) {
			//std::cout << "client couldnt handshake because " << ec.message() << std::endl;
			return false;
		}

		this->on_connect(std::static_pointer_cast<ws_socket>(this->shared_from_this()));
		this->async_read();
		return true;
	}

	void async_open(const char * host, const char * port, char const * target, int version)
	{
		std::string host_ = host;
		std::string target_ = target;
		auto this_sptr = std::static_pointer_cast<ws_socket<CustomData>>(this->shared_from_this());

		auto resolver_sptr = std::make_shared<tcp::resolver>(this->io_context);
		resolver_sptr->async_resolve(host, port,
			[resolver_sptr,this_sptr,host_,target_](boost::beast::error_code ec, boost::asio::ip::tcp::resolver::results_type results) mutable
			{
				if(ec) {
					std::cout << "client couldnt resolve because " << ec.message() << std::endl;
					return;
				}

				// Set the timeout for the operation
				boost::beast::get_lowest_layer(this_sptr->stream).expires_after(std::chrono::seconds(30));

				// Make the connection on the IP address we get from a lookup
				boost::beast::get_lowest_layer(this_sptr->stream).async_connect(results,
					[this_sptr,host_,target_](boost::beast::error_code ec, boost::asio::ip::tcp::resolver::results_type::endpoint_type ep) mutable
					{
						if(ec) {
							std::cout << "client couldnt connect because " << ec.message() << std::endl;
							return;
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
						host_.append(":").append(std::to_string(ep.port()));

						// Perform the websocket handshake
						this_sptr->stream.async_handshake(host_, target_,
							[this_sptr](boost::beast::error_code ec) mutable
							{
								if(ec) {
									std::cout << "client couldnt handshake because " << ec.message() << std::endl;
									return;
								}

								this_sptr->on_connect(this_sptr);
								this_sptr->async_read();
							});
					});
			});
	}

	void async_read()
	{
		size_t available_size = this->recv_buffer.capacity() - this->recv_buffer.size();
		char * ok = (char*) &this->recv_buffer[this->recv_buffer.size()];
		this->recv_buffer.resize(this->recv_buffer.capacity());
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
		this->recv_buffer.resize(this->recv_buffer.capacity() - expected + bytes_transferred);
		if (bytes_transferred)
		{
			// transform 4-bits-per-byte data into 8-bits-per-byte data
			auto itw = std::next(this->recv_buffer.begin(),this->recv_buffer.size()-(bytes_transferred));
			for (auto it=std::next(this->recv_buffer.begin(),this->recv_buffer.size()-(bytes_transferred))
				    ,end=std::next(this->recv_buffer.begin(),this->recv_buffer.size())
				; it!=end
				; ++itw )
			{
				*itw = ((*it)&0xF) << 4;
				++it;
				*itw += (*it)&0xF; 
				++it;
			}
			this->recv_buffer.resize(this->recv_buffer.capacity() - expected + bytes_transferred/2);
			this->recv_buffer.erase(itw, this->recv_buffer.end());

			this->on_message(std::static_pointer_cast<ws_socket>(this->shared_from_this()));
		}
		if (error)
		{
			if (error != boost::beast::websocket::error::closed)
				close();
			else
				this->on_close(std::static_pointer_cast<ws_socket>(this->shared_from_this()));
			return;
		}
		async_read();
	}

	void async_write(std::vector<char> && data)
	{
		// transform 8-bits-per-byte data into 4-bits-per-byte data
		std::vector<char> hex_data;
		hex_data.reserve(data.size()*2);
		for(char c : data)
		{
			hex_data.push_back((c>>4) + 0b01000000);
			hex_data.push_back((c&0xF) + 0b01000000);
		}
		auto this_sptr = std::static_pointer_cast<ws_socket>(this->shared_from_this());
		async_write_callback_ws_t callback(std::move(hex_data), this_sptr);
		stream.async_write(boost::asio::buffer(callback.data.data(), callback.data.size()), callback);
	}

};

template<typename CustomData=int>
std::shared_ptr<ws_socket<CustomData>> make_ws_socket(boost::asio::io_context & context)
{
	return std::make_shared<ws_socket<CustomData>>(context);
}

}} // namespace
