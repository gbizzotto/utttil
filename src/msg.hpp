
#include <cstdint>

#include <utttil/dfloat.hpp>
#include <utttil/unique_int.hpp>
#include <utttil/fixed_string.hpp>

struct     lot_count_tag{}; using     lot_count_t = utttil::unique_int<uint32_t,     lot_count_tag>;
struct     pic_count_tag{}; using     pic_count_t = utttil::unique_int< int32_t,     pic_count_tag>;
struct    pic_volume_tag{}; using    pic_volume_t = utttil::unique_int< int64_t,    pic_volume_tag>;
struct           seq_tag{}; using           seq_t = utttil::unique_int<uint64_t,           seq_tag>;
struct        req_id_tag{}; using        req_id_t = utttil::unique_int<uint64_t,        req_id_tag>;
struct           oid_tag{}; using           oid_t = utttil::unique_int<uint64_t,           oid_tag>;
struct          gwid_tag{}; using          gwid_t = utttil::unique_int<uint32_t,          gwid_tag>;
struct          meid_tag{}; using          meid_t = utttil::unique_int<uint32_t,          meid_tag>;
struct    account_id_tag{}; using    account_id_t = utttil::unique_int<uint32_t,    account_id_tag>;
struct      asset_id_tag{}; using      asset_id_t = utttil::unique_int<uint32_t,      asset_id_tag>;
using cloid_t = req_id_t;
struct instrument_id_tag{}; using instrument_id_t = utttil::unique_int<uint32_t, instrument_id_tag>;
struct     timestamp_tag{}; using     timestamp_t = utttil::unique_int<uint64_t,     timestamp_tag>;
struct cl_session_id_tag{}; using cl_session_id_t = utttil::unique_int<uint64_t, cl_session_id_tag>;

struct       rl_tag{}; using       round_lot_t = utttil::dfloat<  uint32_t, 4,       rl_tag>;
struct      pic_tag{}; using price_increment_t = utttil::dfloat<   int32_t, 4,      pic_tag>;
struct quantity_tag{}; using        quantity_t = utttil::dfloat<  uint64_t, 4, quantity_tag>;
struct   volume_tag{}; using          volume_t = utttil::dfloat<__int128_t, 5,   volume_tag>;
struct  balance_tag{}; using         balance_t = utttil::dfloat<__int128_t, 5,  balance_tag>;

struct username_tag{}; using        username_t = utttil::fixed_string<32, username_tag>;
struct password_tag{}; using        password_t = utttil::fixed_string<32, password_tag>;

static_assert(sizeof(      round_lot_t) == sizeof(      round_lot_t::mantissa_t));
static_assert(sizeof(price_increment_t) == sizeof(price_increment_t::mantissa_t));
static_assert(sizeof(       quantity_t) == sizeof(       quantity_t::mantissa_t));
static_assert(sizeof(         volume_t) == sizeof(         volume_t::mantissa_t));
static_assert(sizeof(        balance_t) == sizeof(        balance_t::mantissa_t));

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
		s << instrument_id
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
		s >> instrument_id
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

inline std::ostream & operator<<(std::ostream & out, const  NewOrder &)
{
	return out << "NewOrder";
}
inline bool operator==(const NewOrder & left, const NewOrder & right)
{
	return left.instrument_id == right.instrument_id
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


struct Request
{
	using request_type_t = std::uint32_t;
	enum class Type : request_type_t
	{
		// 0 is RFU
		NewOrder = 6,

		End = 0xFFFFFFFF
	};

	Type         type;
	seq_t        seq; // starts at 1
	account_id_t account_id; // the author of the request. Could be an internal system or a user.
	req_id_t     req_id; // user-provided, echoed into response/publications, is also used for cl_order_id

	union // value depends on field 'type'
	{
		NewOrder         new_order;
	};

	inline Request() {}
	Request(const Request & other)
		: type(other.type)
		, seq(other.seq)
		, account_id(other.account_id)
		, req_id(other.req_id)
	{
		switch(other.type)
		{
			case Request::Type::NewOrder        : new_order         = other.new_order        ; break;
			case Request::Type::End             :                                              break;
		}
	}

	Request & operator=(const Request & other)
	{
		type       = other.type      ;
		seq        = other.seq       ;
		account_id = other.account_id;
		req_id     = other.req_id    ;
		new_order  = other.new_order ;
		return *this;
	}

	template<typename Serializer>
	void serialize(Serializer && s) const
	{
		s << (request_type_t)type
		  << seq
		  << account_id
		  << req_id
		  ;
		switch(type)
		{
			case Request::Type::NewOrder        : s << new_order        ; break;
			case Request::Type::End             :                         break;
		}
		s << std::flush;
	}
	template<typename Deserializer>
	void deserialize(Deserializer && s)
	{
		s >> (request_type_t&)type
		  >> seq
		  >> account_id
		  >> req_id
		  ;
		switch(type)
		{
			case Request::Type::NewOrder        : s >> new_order        ; break;
			case Request::Type::End             :                         break;
		}
	}

	using seq_type = decltype(seq);
	seq_t get_seq() const { return seq; }
};

inline std::ostream & operator<<(std::ostream & out, const Request::Type & type)
{
	return out << (typename Request::request_type_t) type;
}
inline std::ostream & operator<<(std::ostream & out, const Request & req)
{
	return out << "Req #" << req.seq << " T" << req.type;
}
inline bool operator==(const Request & left, const Request & right)
{
	if (   left.type       != right.type
	    || left.account_id != right.account_id
	    || left.seq        != right.seq  )
	{
		return false;
	}
	switch(left.type)
	{
		case Request::Type::NewOrder        : return left.new_order         == right.new_order        ;
		case Request::Type::End             : return true;
	}
	return true;
}
inline bool operator!=(const Request & left, const Request & right)
{
	return ! (left == right);
}