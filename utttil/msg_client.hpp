
#pragma once

#include "io.hpp"
#include "srlz.hpp"
#include "msg_peer.hpp"

namespace utttil {

template<typename InMsg, typename OutMsg, typename CustomData=int>
struct msg_client : msg_peer<InMsg,OutMsg,CustomData>
{
	using ConnectionSPTR = typename msg_peer<InMsg,OutMsg,CustomData>::ConnectionSPTR;

	void connect(const utttil::url url, std::shared_ptr<boost::asio::io_context> io_context = nullptr)
	{
		this->go_on = true;

		this->interface = utttil::io::connect(url, io_context);

		auto this_sptr = this->shared_from_this();
		this->interface->on_message = [this_sptr](typename utttil::io::interface<CustomData>::ConnectionSPTR conn_sptr)
			{
				this_sptr->decode_recvd(conn_sptr);
			};
	}
};

template<typename InMsg, typename OutMsg, typename CustomData=int>
auto make_msg_client(const utttil::url url, std::shared_ptr<boost::asio::io_context> io_context = nullptr)
{
	auto client = std::make_shared<msg_client<InMsg,OutMsg,CustomData>>();
	client->connect(url, io_context);
	return client;
}

} // namespace