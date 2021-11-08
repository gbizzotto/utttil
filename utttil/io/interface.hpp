
#pragma once

#include "headers.hpp"

#include <functional>
#include <memory>
#include <vector>

#include <boost/asio.hpp>

#include "utttil/url.hpp"
#include "utttil/no_init.hpp"

namespace utttil {
namespace io {

template<typename CustomData=int>
struct interface : public std::enable_shared_from_this<interface<CustomData>>
{
	using ConnectionData = CustomData;
	using Connection = interface;
	using ConnectionSPTR = std::shared_ptr<interface>;
	using CallbackOnConnect = std::function<void(ConnectionSPTR)>;
	using CallbackOnClose   = std::function<void(ConnectionSPTR)>;
	using CallbackOnMessage = std::function<void(ConnectionSPTR)>;

	boost::asio::io_context & io_context;
	ConnectionData connection_data;
	std::vector<utttil::no_init<char>> recv_buffer;
	
	CallbackOnConnect on_connect = [](ConnectionSPTR){};
	CallbackOnConnect on_close   = [](ConnectionSPTR){};
	CallbackOnMessage on_message = [](ConnectionSPTR){};

	interface(boost::asio::io_context & ctx)
		: io_context(ctx)
	{}
	virtual ~interface()
	{
		close();
	}

	virtual void async_write(std::vector<char> && data) {}
	virtual void close() {}
};

}} // namespace