
#pragma once

#include <functional>
#include <memory>
#include <vector>

#include <boost/asio.hpp>

#include <utttil/url.hpp>

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

	std::shared_ptr<boost::asio::io_context> io_context;
	ConnectionData connection_data;
	std::vector<char> recv_buffer;
	
	CallbackOnConnect on_connect = [](ConnectionSPTR){};
	CallbackOnConnect on_close   = [](ConnectionSPTR){};
	CallbackOnMessage on_message = [](ConnectionSPTR){};

	std::thread t;

	interface(std::shared_ptr<boost::asio::io_context> & context)
		: io_context(context)
	{}
	interface(std::shared_ptr<boost::asio::io_context> && context)
		: io_context(context)
	{}
	interface()
		: io_context(std::make_shared<boost::asio::io_context>())
	{}

	void run()
	{
		io_context->run();
	}
	void async_run()
	{
		auto this_sptr = this->shared_from_this();
		t = std::thread([this_sptr](){ this_sptr->run(); });
	}
	void stop()
	{
		io_context->stop();
	}
	void join()
	{
		t.join();
	}

	virtual void async_write(std::vector<char> && data) {};
	virtual bool is_null_io() const { return true; }
};

std::shared_ptr<interface<int>> make_null_io() { return std::make_shared<interface<int>>(); }
std::shared_ptr<interface<int>> make_null_io(std::shared_ptr<boost::asio::io_context>  & context) { return std::make_shared<interface<int>>(context); }
std::shared_ptr<interface<int>> make_null_io(std::shared_ptr<boost::asio::io_context> && context) { return std::make_shared<interface<int>>(context); }

}} // namespace