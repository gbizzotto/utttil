
#include "headers.hpp"

#include <iostream>
#include <random>
#include <memory>

#include "utttil/srlz.hpp"
#include "utttil/math.hpp"
#include "utttil/assert.hpp"
#include "utttil/unique_int.hpp"

#include "msg.hpp"

namespace srlz = utttil::srlz;


bool test_order()
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

	{
		std::vector<char> v;
		auto s = utttil::srlz::to_binary(utttil::srlz::device::back_inserter(v));
		s << req;

		auto ds = utttil::srlz::from_binary(utttil::srlz::device::iterator_reader(v.begin(), v.end()));
		Request req2;
		ds >> req2;

		ASSERT_MSG_ACT(req, ==, req2, "Error: (De)Serialization of NewOrder.", return false);
	}
	{
		const Request & r = req;
		char buf[1024];
		std::unique_ptr<srlz::to_binary<srlz::device::mmap_writer>> packer;
		packer.reset(new srlz::to_binary(srlz::device::mmap_writer((char *) buf)));
		(*packer) << r;
	}
	return true;
}

int main()
{
	return (
		   test_order()
		)?0:1;
}