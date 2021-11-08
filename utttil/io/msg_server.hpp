
#pragma once

#include <memory>

#include "utttil/srlz.hpp"
#include "utttil/io/msg_peer.hpp"
#include "utttil/io/context.hpp"

namespace utttil {
namespace io {

template<typename InMsg, typename OutMsg, typename CustomData=int>
struct msg_server : msg_peer<InMsg,OutMsg,CustomData>
{
	using ConnectionSPTR = typename msg_peer<InMsg,OutMsg,CustomData>::ThisSPTR;

	void bind(context & ctx, const utttil::url url)
	{
		this->go_on = true;

		auto this_wptr = std::weak_ptr(this->shared_from_this());
		auto on_connect = [this_wptr](auto interface_sptr)
			{
				auto this_sptr = this_wptr.lock();
				if ( ! this_sptr)
					return;
				auto peer_sptr = std::make_shared<msg_peer<InMsg,OutMsg,CustomData>>();
				peer_sptr->interface = interface_sptr;
				peer_sptr->on_message = this_sptr->on_message;
				interface_sptr->on_message = [peer_sptr](auto)
					{
						peer_sptr->decode_recvd();
					};
			};
		auto on_close = [this_wptr](auto)
			{
				auto this_sptr = this_wptr.lock();
				if (this_sptr)
					this_sptr->on_close(this_sptr);
			};

		this->interface = ctx.bind(url, on_connect, nullptr, on_close);
	}
};

template<typename InMsg, typename OutMsg, typename CustomData=int>
auto make_msg_server(context & ctx, const utttil::url url
	,typename msg_peer<InMsg,OutMsg,CustomData>::CallbackOnConnect on_connect_
	,typename msg_peer<InMsg,OutMsg,CustomData>::CallbackOnMessage on_message_
	,typename msg_peer<InMsg,OutMsg,CustomData>::CallbackOnClose   on_close_  
	)
{
	auto msg_server = std::make_shared<utttil::io::msg_server<InMsg,OutMsg,CustomData>>();
	if (on_connect_ != nullptr)
		msg_server->on_connect = on_connect_;
	if (on_message_ != nullptr)
		msg_server->on_message = on_message_;
	if (on_close_ != nullptr)
		msg_server->on_close   = on_close_;
	msg_server->bind(ctx, url);
	return msg_server;
}

}} // namespace