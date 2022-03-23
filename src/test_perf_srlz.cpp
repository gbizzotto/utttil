
#include <chrono>

#include <utttil/srlz.hpp>
#include <utttil/perf.hpp>

#include "msg.hpp"

utttil::measurement_point mps("srlz srlz");
utttil::measurement_point mpss("srlz operator<<");

bool test()
{
	Request req;
	req.type = Request::Type::NewOrder;
	req.seq = utttil::max<decltype(req.seq)>();
	req.account_id = utttil::max<decltype(req.account_id)>();
	req.req_id = utttil::max<decltype(req.req_id)>();
	NewOrder &new_order = req.new_order;
	new_order.instrument_id = utttil::max<decltype(new_order.instrument_id)>();
	new_order.is_sell                   = false;
	new_order.is_limit                  = false;
	new_order.is_stop                   = false;
	new_order.participate_dont_initiate = false;
	new_order.time_in_force = TimeInForce::GTD;
	new_order.lot_count      = utttil::max<decltype(new_order.lot_count     )>();
	new_order.pic_count      = utttil::max<decltype(new_order.pic_count     )>();
	new_order.stop_pic_count = utttil::max<decltype(new_order.stop_pic_count)>();

	for ( auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3)
		; std::chrono::steady_clock::now() < deadline
		; )
	{
		utttil::measurement ms(mps);
		char buf[1024];
		auto s = utttil::srlz::to_binary(utttil::srlz::device::ptr_writer(buf+2));
		{
			utttil::measurement mss(mpss);
			s << req;
		}
		size_t size = s.write.size() - 2;
		*buf = (size/256) & 0xFF;
		*(buf+1) = size & 0xFF;
	}
	return true;
}

int main()
{
	bool result = true
		&& test()
		;

	return result ? 0 : 1;
}