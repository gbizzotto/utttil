#pragma once

#include "io.hpp"
#include "srlz.hpp"
#include "msg_peer.hpp"

namespace utttil {

template<typename InMsg, typename OutMsg, typename CustomData=int>
struct msg_server : msg_peer<InMsg,OutMsg,CustomData>
{
	using ConnectionSPTR = typename msg_peer<InMsg,OutMsg,CustomData>::ThisSPTR;
	using UnderlyingSPTR = typename msg_peer<InMsg,OutMsg,CustomData>::UnderlyingSPTR;

	void bind(const utttil::url url, std::shared_ptr<boost::asio::io_context> io_context = nullptr)
	{
		auto this_sptr = this->shared_from_this();

		this->go_on = true;
		this->interface = utttil::io::bind(url, io_context);
		this->interface->on_connect = [this_sptr](UnderlyingSPTR interface_sptr)
			{
				std::cout << "on_connect" << std::endl;
				auto peer_sptr = std::make_shared<msg_peer<InMsg,OutMsg,CustomData>>();
				peer_sptr->interface = interface_sptr;
				peer_sptr->on_message = this_sptr->on_message;
				interface_sptr->on_message = [peer_sptr](auto)
					{
						std::cout << "server on_msg" << std::endl;
						peer_sptr->decode_recvd();
					};
			};
	}
};

template<typename InMsg, typename OutMsg, typename CustomData=int>
auto make_msg_server(const utttil::url url, std::shared_ptr<boost::asio::io_context> io_context = nullptr)
{
	auto msg_server = std::make_shared< utttil::msg_server<InMsg,OutMsg,CustomData> >();
	msg_server->bind(url, io_context);
	return msg_server;
}

} // namespace