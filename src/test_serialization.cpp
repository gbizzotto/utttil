
#include "headers.hpp"

#include <iostream>
#include <random>
#include <memory>

#include "utttil/srlz.hpp"
#include "utttil/math.hpp"
#include "utttil/assert.hpp"
#include "utttil/unique_int.hpp"

namespace srlz = utttil::srlz;

struct      account_id_tag{}; using      account_id_t = utttil::unique_int<uint32_t,      account_id_tag>;
struct   instrument_id_tag{}; using   instrument_id_t = utttil::unique_int<uint32_t,   instrument_id_tag>;
struct       lot_count_tag{}; using       lot_count_t = utttil::unique_int<uint32_t,       lot_count_tag>;
struct       pic_count_tag{}; using       pic_count_t = utttil::unique_int< int32_t,       pic_count_tag>;
struct      pic_volume_tag{}; using      pic_volume_t = utttil::unique_int< int64_t,      pic_volume_tag>;
using time_in_force_t = uint8_t;
enum class TimeInForce : time_in_force_t
{
	GTC = 0, // Good Till Cancel
	IOC = 1, // Immediate or Cancel
	FOK = 2, // Fill or Kill
	GTD = 3, // Good till Date
};

struct NewOrder
{
	account_id_t     account_id;
	instrument_id_t  instrument_id;
	bool           is_sell;
	bool           is_limit;
	bool           is_stop;
	bool           participate_dont_initiate;
	TimeInForce    time_in_force;
	lot_count_t lot_count;
	pic_count_t pic_count;      // absent if (!is_limit)
	pic_count_t stop_pic_count; // absent if (!is_stop)

	inline NewOrder() {};

	template<typename Serializer>
	void serialize(Serializer && s) const
	{
		char flags = is_sell
			| (is_limit << 1)
			| (is_stop  << 2)
			| (participate_dont_initiate << 3)
			;
		s << account_id
		  << instrument_id
		  << (time_in_force_t)time_in_force
		  << lot_count
		  << flags
		  ;
		if (is_limit)
			s << pic_count;
		if (is_stop)
			s << stop_pic_count;
	}
	template<typename Deserializer>
	void deserialize(Deserializer && s)
	{
		char flags;
		s >> account_id
		  >> instrument_id
		  >> (time_in_force_t&)time_in_force
		  >> lot_count
		  >> flags
		  ;
		is_sell                   = bool(flags & 0b000000001);
		is_limit                  = bool(flags & 0b000000010);
		is_stop                   = bool(flags & 0b000000100);
		participate_dont_initiate = bool(flags & 0b000001000);
		if (is_limit)
			s >> pic_count;
		if (is_stop)
			s >> stop_pic_count;
	}
};
inline std::ostream & operator<<(std::ostream & out, const  NewOrder & new_order)
{
	return out << "NewOrder";
}
inline bool operator==(const NewOrder & left, const NewOrder & right)
{
	return left.account_id == right.account_id
		&& left.instrument_id == right.instrument_id
	    && left.is_sell == right.is_sell
	    && left.is_limit == right.is_limit
	    && left.is_stop == right.is_stop
	    && left.participate_dont_initiate == right.participate_dont_initiate
	    && left.time_in_force == right.time_in_force
	    && left.lot_count == right.lot_count
	    && left.is_limit?(left.pic_count == right.pic_count):true
	    && left.is_stop?(left.stop_pic_count == right.stop_pic_count):true
	    ;
}
inline bool operator!=(const NewOrder & left, const NewOrder & right)
{
	return ! (left == right);
}

struct Msg
{
	char type;
	int64_t seq;
	unsigned int account_id;
	int req_id;
	NewOrder new_order;
	template<typename Serializer  > void   serialize(  Serializer && s) const { s << type << seq << account_id << req_id << new_order; }
	template<typename Deserializer> void deserialize(Deserializer && s)       { s >> type >> seq >> account_id >> req_id >> new_order; }
};
inline std::ostream & operator<<(std::ostream & out, const  Msg & msg)
{
	return out << "Msg";
}
inline bool operator==(const Msg & left, const Msg & right)
{
	return left.type == right.type
	    && left.seq == right.seq
		&& left.account_id == right.account_id
		&& left.req_id == right.req_id
		&& left.new_order == right.new_order
		;
}
inline bool operator!=(const Msg & left, const Msg & right)
{
	return ! (left == right);
}

bool test_order()
{
	Msg req;
	req.type = 1;
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
		Msg req2;
		ds >> req2;

		ASSERT_MSG_ACT(req, ==, req2, "Error: (De)Serialization of NewOrder.", return false);
	}
	{
		const Msg & r = req;
		char buf[1024];
		std::unique_ptr<srlz::to_binary<srlz::device::mmap_writer>> packer;
		packer.reset(new srlz::to_binary(srlz::device::mmap_writer((volatile char *) buf)));
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