
#pragma once

#include "utttil/srlz.hpp"
#include "utttil/io/msg_peer.hpp"
#include "utttil/io/context.hpp"

namespace utttil {
namespace io {

template<typename InMsg, typename OutMsg, typename CustomData=int>
struct msg_client : msg_peer<InMsg,OutMsg,CustomData>
{
	using ConnectionSPTR = typename msg_peer<InMsg,OutMsg,CustomData>::ThisSPTR;

	void connect(context & ctx, const utttil::url url)
	{
		this->go_on = true;

		// using weak_ptr here to allow the destructor to be called (and thus close())
		// when whomever build this thing lets it go out of scope and die
		auto this_wptr = std::weak_ptr(this->shared_from_this());
		auto on_connect_ = [this_wptr](auto)
			{
				auto this_sptr = this_wptr.lock();
				if (this_sptr)
				{
					// if ctx.connect() called below is synchronous, this callback
					// will be called before this->interface is assigned
					// if this happens, we have to call it manually after ctx.connect() returns
					if ( ! this_sptr->interface_)
						return;
					this_sptr->on_connect(this_sptr);
					this_sptr->on_connect = nullptr;
				}
			};
		auto on_message_ = [this_wptr](auto)
			{
				auto this_sptr = this_wptr.lock();
				if (this_sptr)
					this_sptr->decode_recvd();
			};
		auto on_close_ = [this_wptr](auto)
			{
				auto this_sptr = this_wptr.lock();
				if (this_sptr)
					this_sptr->on_close(this_sptr);
			};
		this->interface_ = ctx.connect<CustomData>(url, on_connect_, on_message_, on_close_);
		if (this->on_connect) // see comment inside on_connect above
			this->on_connect(this->shared_from_this());
	}
};

template<typename InMsg, typename OutMsg, typename CustomData=int>
auto make_msg_client(context & ctx, const utttil::url url
	,typename msg_peer<InMsg,OutMsg,CustomData>::CallbackOnConnect on_connect_
	,typename msg_peer<InMsg,OutMsg,CustomData>::CallbackOnMessage on_message_
	,typename msg_peer<InMsg,OutMsg,CustomData>::CallbackOnClose   on_close_  
	)
{
	auto client = std::make_shared<msg_client<InMsg,OutMsg,CustomData>>();
	if (on_connect_ != nullptr)
		client->on_connect = on_connect_;
	if (on_message_ != nullptr)
		client->on_message = on_message_;
	if (on_close_ != nullptr)
		client->on_close   = on_close_;
	client->connect(ctx, url);
	return client;
}

}} // namespace