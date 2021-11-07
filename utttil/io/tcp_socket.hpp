
#pragma once

#include "headers.hpp"

#include <deque>
#include <vector>
#include <functional>

#include <boost/asio.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/bind/bind.hpp>

#include "utttil/synchronized.hpp"

#include "interface.hpp"

using boost::asio::ip::tcp;

namespace utttil {
namespace io {

template<typename T>
struct async_write_callback_tcp_t
{
	std::vector<char> data;
	T t;
	async_write_callback_tcp_t(std::vector<char> && d, T t_)
		: data(std::move(d))
		, t(t_)
	{}
	void operator()(const boost::system::error_code & error, size_t bytes_transferred)
	{
		if (error)
		{
			if (error != boost::asio::error::eof)
				t->close();
			else
				t->on_close(t);
			return;
		}
	};
};

template<typename CustomData=int>
struct tcp_socket : interface<CustomData>
{
	using ConnectionData = CustomData;

	tcp::socket socket;
	
	tcp_socket(std::shared_ptr<boost::asio::io_context> context = nullptr)
		: interface<CustomData>(context)
		, socket(*this->io_context)
	{
		this->recv_buffer.reserve(1500);
	}
	virtual bool is_null_io() const override { return false; }

	auto & get_socket()
	{
		return socket;
	}

	void open(const char * host, const char * port)
	{
		tcp::resolver resolver(*this->io_context);
		auto endpoints = resolver.resolve(host, port);
		boost::asio::connect(socket, endpoints);
		this->on_connect(std::static_pointer_cast<tcp_socket<CustomData>>(this->shared_from_this()));
		async_read();
	}
	virtual void close() override
	{
		if (socket.is_open())
		{
			boost::system::error_code ec;
			socket.close(ec);
			this->on_close(std::static_pointer_cast<tcp_socket>(this->shared_from_this()));
		}
	}

	void async_read()
	{
		size_t available_size = this->recv_buffer.capacity() - this->recv_buffer.size();
		char * ok = &this->recv_buffer[this->recv_buffer.size()];
		this->recv_buffer.resize(this->recv_buffer.capacity());
		socket.async_read_some(
				boost::asio::buffer(ok, available_size),
				boost::bind(&tcp_socket::handle_read, std::static_pointer_cast<tcp_socket>(this->shared_from_this()),
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred,
					available_size)
			);
	}
	void handle_read(const boost::system::error_code & error, size_t bytes_transferred, size_t expected)
	{
		if (error)
		{
			if (error != boost::asio::error::eof)
				close();
			else
				this->on_close(std::static_pointer_cast<tcp_socket>(this->shared_from_this()));
			return;
		}
		this->recv_buffer.resize(this->recv_buffer.capacity() - expected + bytes_transferred);
		if (bytes_transferred)
			this->on_message(std::static_pointer_cast<tcp_socket>(this->shared_from_this()));
		async_read();
	}

	void async_write(std::vector<char> && data)
	{
		auto this_sptr = std::static_pointer_cast<tcp_socket>(this->shared_from_this());
		async_write_callback_tcp_t callback(std::move(data), this_sptr);
		boost::asio::async_write(socket, boost::asio::buffer(callback.data.data(), callback.data.size()), callback);
	}
};

template<typename CustomData=int>
std::shared_ptr<tcp_socket<CustomData>> make_tcp_socket(std::shared_ptr<boost::asio::io_context> context = nullptr)
{
	return std::make_shared<tcp_socket<CustomData>>(context);
}

}} // namespaces
