
#pragma once

#include "headers.hpp"

#include <iostream>
#include <thread>
#include <boost/asio.hpp>

#include "utttil/io/tcp_socket.hpp"
#include "utttil/io/tcp_server.hpp"
#include "utttil/io/ws_server.hpp"
#include "utttil/io/ws_socket.hpp"
#include "utttil/io/interface.hpp"

#include "utttil/io/msg_peer.hpp"

namespace utttil {
namespace io {

// forward declarations

struct context;

template<typename InMsg, typename OutMsg, typename CustomData=int>
auto make_msg_client(context & ctx, const utttil::url url
	,typename msg_peer<InMsg,OutMsg,CustomData>::CallbackOnConnect on_connect_ = nullptr
	,typename msg_peer<InMsg,OutMsg,CustomData>::CallbackOnMessage on_message_ = nullptr
	,typename msg_peer<InMsg,OutMsg,CustomData>::CallbackOnClose   on_close_   = nullptr
	);
template<typename InMsg, typename OutMsg, typename CustomData=int>
auto make_msg_server(context & ctx, const utttil::url url
	,typename msg_peer<InMsg,OutMsg,CustomData>::CallbackOnConnect on_connect_ = nullptr
	,typename msg_peer<InMsg,OutMsg,CustomData>::CallbackOnMessage on_message_ = nullptr
	,typename msg_peer<InMsg,OutMsg,CustomData>::CallbackOnClose   on_close_   = nullptr
	);


// Owns the io_context and thread
// stops those if it dies so we have graceful shutdown
struct context
{
	boost::asio::io_context io_context;
	boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work;
	std::thread t;

	inline context()
		: work(boost::asio::make_work_guard(io_context))
	{}
	inline ~context()
	{
		stop();
		join();
	}

	inline void run()
	{
		io_context.run();
	}
	inline void async_run()
	{
		t = std::thread([&](){ io_context.run(); });
	}
	inline void stop()
	{
		io_context.stop();
	}
	inline void join()
	{
		if (t.joinable())
			t.join();
	}

	template<typename CustomData=int>
	std::shared_ptr<interface<CustomData>> connect(const utttil::url url
		,typename interface<CustomData>::CallbackOnConnect on_connect_ = [](typename interface<CustomData>::ConnectionSPTR){}
		,typename interface<CustomData>::CallbackOnMessage on_message_ = [](typename interface<CustomData>::ConnectionSPTR){}
		,typename interface<CustomData>::CallbackOnClose   on_close_   = [](typename interface<CustomData>::ConnectionSPTR){}
		)
	{
		if (url.protocol == "tcp")
		{
			auto socket = make_tcp_socket<CustomData>(this->io_context);
			if ( ! socket)
				return nullptr;
			if (on_connect_ != nullptr)
				socket->on_connect = on_connect_;
			if (on_message_ != nullptr)
				socket->on_message = on_message_;
			if (on_close_ != nullptr)
				socket->on_close   = on_close_;
			socket->open(url.host.c_str(), url.port.c_str());
			return socket;
		}
		else if (url.protocol == "ws")
		{
			auto socket = make_ws_socket<CustomData>(io_context);
			if ( ! socket)
				return nullptr;
			if (on_connect_ != nullptr)
				socket->on_connect = on_connect_;
			if (on_message_ != nullptr)
				socket->on_message = on_message_;
			if (on_close_ != nullptr)
				socket->on_close   = on_close_;
			socket->async_open(url.host.c_str(), url.port.c_str(), url.location.c_str(), 1);
			return socket;
		}
		return nullptr;
	}

	template<typename CustomData=int>
	std::shared_ptr<interface<CustomData>> bind(const utttil::url url
		,typename interface<CustomData>::CallbackOnConnect on_connect_ = [](typename interface<CustomData>::ConnectionSPTR){}
		,typename interface<CustomData>::CallbackOnMessage on_message_ = [](typename interface<CustomData>::ConnectionSPTR){}
		,typename interface<CustomData>::CallbackOnClose   on_close_   = [](typename interface<CustomData>::ConnectionSPTR){}
		)
	{
		if (url.protocol == "tcp")
		{
			auto server = make_tcp_server<CustomData>(this->io_context, std::stoi(url.port));
			if ( ! server)
				return nullptr;
			if (on_connect_ != nullptr)
				server->on_connect = on_connect_;
			if (on_message_ != nullptr)
				server->on_message = on_message_;
			if (on_close_ != nullptr)
				server->on_close   = on_close_;
			return server;
		}
		else if (url.protocol == "ws")
		{
			auto server = make_ws_server<CustomData>(io_context, std::stoi(url.port));
			if ( ! server)
				return nullptr;
			if (on_connect_ != nullptr)
				server->on_connect = on_connect_;
			if (on_message_ != nullptr)
				server->on_message = on_message_;
			if (on_close_ != nullptr)
				server->on_close   = on_close_;
			server->listen();
			return server;
		}
		return nullptr;
	}

	template<typename InMsg, typename OutMsg, typename CustomData=int>
	std::shared_ptr<msg_peer<InMsg,OutMsg,CustomData>> connect_srlz(const utttil::url url
		,typename msg_peer<InMsg,OutMsg,CustomData>::CallbackOnConnect on_connect_ = [](auto){}
		,typename msg_peer<InMsg,OutMsg,CustomData>::CallbackOnMessage on_message_ = nullptr // when null, messages go to msg_peer::inbox
		,typename msg_peer<InMsg,OutMsg,CustomData>::CallbackOnClose   on_close_   = [](auto){}
		)
	{
		return make_msg_client<InMsg,OutMsg,CustomData>(*this, url, on_connect_, on_message_, on_close_);
	}
	template<typename InMsg, typename OutMsg, typename CustomData=int>
	std::shared_ptr<msg_peer<InMsg,OutMsg,CustomData>> bind_srlz(const utttil::url url
		,typename msg_peer<InMsg,OutMsg,CustomData>::CallbackOnConnect on_connect_ = [](auto){}
		,typename msg_peer<InMsg,OutMsg,CustomData>::CallbackOnMessage on_message_ = nullptr // when null, messages go to msg_peer::inbox
		,typename msg_peer<InMsg,OutMsg,CustomData>::CallbackOnClose   on_close_   = [](auto){}
		)
	{
		return make_msg_server<InMsg,OutMsg,CustomData>(*this, url, on_connect_, on_message_, on_close_);
	}
};

}} // namespace
