
#pragma once

#include <memory>
#include <deque>

#include "io.hpp"
#include "srlz.hpp"

namespace utttil {

template<typename InMsg, typename OutMsg, typename CustomData=int>
struct msg_peer : std::enable_shared_from_this<msg_peer<InMsg,OutMsg,CustomData>>
{
	using ConnectionSPTR = typename utttil::io::interface<CustomData>::ConnectionSPTR;
	using CallbackOnConnect = std::function<void(ConnectionSPTR)>;
	using CallbackOnClose   = std::function<void(ConnectionSPTR)>;
	using CallbackOnMessage = std::function<void(ConnectionSPTR, std::unique_ptr<InMsg>)>;

	std::shared_ptr<utttil::io::interface<CustomData>> interface;
	utttil::synchronized<std::deque<std::pair<ConnectionSPTR,std::unique_ptr<InMsg>>>> inbox;
	
	CallbackOnConnect on_connect = [](ConnectionSPTR){};
	CallbackOnConnect on_close   = [](ConnectionSPTR){};
	CallbackOnMessage on_message;

	bool go_on = true;

	void close()
	{
		go_on = false;
		interface->close();
	}
	void async_run()
	{
		interface->async_run();
	}
	void join()
	{
		interface->join();
	}

	void decode_recvd(ConnectionSPTR & conn_sptr)
	{
		auto & data = conn_sptr->recv_buffer;
		if (data.size() < 2)
			return;
		size_t size = *data.begin() * 256 + *std::next(data.begin());
		if (data.size() != size+2)
			return;

		auto unpacker = utttil::srlz::from_binary(utttil::srlz::device::iterator_reader(std::next(data.begin(),2), data.end()));
		auto resp = std::make_unique<InMsg>();
		try {
			unpacker >> *resp;
		} catch (utttil::srlz::device::stream_end_exception & e) {
			return;
		} catch (std::overflow_error & e) {
			close();
			return;
		}
		if (on_message)
			on_message(conn_sptr, std::move(resp));
		else
		{
			inbox.lock()->emplace_back(conn_sptr, std::move(resp));
			inbox.notify_one();
		}
	}

	std::pair<ConnectionSPTR, std::unique_ptr<InMsg>> read()
	{
		auto inbox_proxy = inbox.wait_for_notification([this](auto & inbox){ return go_on == false || inbox.size()!=0; });
		if ( ! go_on)
			return {nullptr};
		auto p = std::move(inbox_proxy->front());
		inbox_proxy->pop_front();
		return p;
	}

	void async_send(OutMsg & req)
	{
		std::vector<char> msg;
		msg.reserve(64);
		auto packer = utttil::srlz::to_binary(utttil::srlz::device::back_pusher(msg));

		// size placeholder
		msg.push_back(0);
		msg.push_back(0);

		packer << req;

		//size
		size_t size = msg.size() - 2;
		msg.front() = (size/256) & 0xFF;
		*std::next(msg.begin()) = size & 0xFF;

		interface->async_write(std::move(msg));
	}
};


} // namespace