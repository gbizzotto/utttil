
#include "headers.hpp"

#include <iostream>
#include <thread>

#include "utttil/dfloat.hpp"
#include "utttil/assert.hpp"
#include "utttil/unique_int.hpp"

bool go_on;

bool test_add_sub_int32()
{
	bool success = true;
	using df = utttil::dfloat<int, 4>;
	while(go_on)
	{
		df a = df::rand();
		df b = df::rand();
		df c = a;
		bool loss = c.add_loss(b);
		if ( ! loss)
		{
			ASSERT_MSG_ACT(c.sub_loss(b), ==, false, "Overflow error", success = false);
			ASSERT_MSG_ACT(a, ==, c, a.to_string().append(" + ").append(b.to_string()), success = false);
		}
	}
	return success;
}

//bool test_mul_div_int32()
//{
//	bool success = true;
//	using df4 = utttil::dfloat<int32_t, 4>;
//	using df8 = utttil::dfloat<int64_t, 5>;
//	while(go_on)
//	{
//		df4 a = df4::rand();
//		df4 b = df4::rand();
//		df8 c = a * b;
//		df8 d = c / a;
//		df8 e = c / b;
//		ASSERT_MSG_ACT(d, ==, b, a.to_string().append(" * ").append(b.to_string()).append(" / ").append(a.to_string()), success = false);
//		ASSERT_MSG_ACT(e, ==, a, a.to_string().append(" * ").append(b.to_string()).append(" / ").append(b.to_string()), success = false);
//	}
//	return success;
//}

bool test_comp()
{
	using df = utttil::dfloat<int,4>;
	bool success = true;
	{
		df a(10000,4);
		df b(10,1);
		ASSERT_ACT(a, ==, b, success = false);
		ASSERTNOT_ACT(a, <, b, success = false);
		ASSERTNOT_ACT(b, <, a, success = false);
		ASSERT_ACT(a, <=, b, success = false);
		ASSERT_ACT(b, <=, a, success = false);
	}
	{
		df a(10,1);
		df b(10000,4);
		ASSERT_ACT(a, ==, b, success = false);
		ASSERTNOT_ACT(a, <, b, success = false);
		ASSERTNOT_ACT(b, <, a, success = false);
		ASSERT_ACT(a, <=, b, success = false);
		ASSERT_ACT(b, <=, a, success = false);
	}
	{
		df a(11000,4); // 1.1
		df b(10,1);    // 1
		ASSERTNOT_ACT(a, ==, b, success = false);
		ASSERTNOT_ACT(a, <, b, success = false);
		ASSERT_ACT(b, <, a, success = false);
		ASSERTNOT_ACT(a, <=, b, success = false);
		ASSERT_ACT(b, <=, a, success = false);
	}
	{
		df a(11,1);    // 1.1
		df b(10000,4); // 1
		ASSERTNOT_ACT(a, ==, b, success = false);
		ASSERTNOT_ACT(a, <, b, success = false);
		ASSERT_ACT(b, <, a, success = false);
		ASSERTNOT_ACT(a, <=, b, success = false);
		ASSERT_ACT(b, <=, a, success = false);
	}
	{
		df a(10000,4); // 1
		df b(11,1);    // 1.1
		ASSERTNOT_ACT(a, ==, b, success = false);
		ASSERT_ACT(a, <, b, success = false);
		ASSERTNOT_ACT(b, <, a, success = false);
		ASSERT_ACT(a, <=, b, success = false);
		ASSERTNOT_ACT(b, <=, a, success = false);
	}
	{
		df a(10,1);    // 1
		df b(11000,4); // 1.1
		ASSERTNOT_ACT(a, ==, b, success = false);
		ASSERT_ACT(a, <, b, success = false);
		ASSERTNOT_ACT(b, <, a, success = false);
		ASSERT_ACT(a, <=, b, success = false);
		ASSERTNOT_ACT(b, <=, a, success = false);
	}
	return success;
}

struct lot_count_tag{}; using       lot_count_t = utttil::unique_int<  uint32_t,    lot_count_tag>;
struct        rl_tag{}; using       round_lot_t = utttil::    dfloat<  uint32_t, 4,        rl_tag>;
struct  quantity_tag{}; using        quantity_t = utttil::    dfloat<  uint64_t, 4,  quantity_tag>;
struct    volume_tag{}; using          volume_t = utttil::    dfloat<__int128_t, 5,    volume_tag>;
inline quantity_t operator*(const lot_count_t & left, const round_lot_t & right)
{
	return quantity_t(quantity_t::mantissa_t(left.value()) * right.mantissa
	                 ,quantity_t::exponent_t(right.exponent));
}

int main()
{
	bool success = true;

	ASSERT_ACT(lot_count_t(1234u)*round_lot_t(4321u), ==, quantity_t(5332114u), success = false);
	ASSERT_ACT(quantity_t(10,1), ==, quantity_t(1,0), success = false);
	ASSERT_ACT(volume_t(123'456'789'000'000'000,10), ==, quantity_t(123'456'789,1), success = false);

	volume_t v(123, 2); // 12300
	ASSERT_MSG_ACT(v.add_loss(volume_t(10)), ==, false, "Unexpected overflow", success = false);

	using dfp = utttil::dfloat<int, 4>;
	dfp a(50'000'000,8);
	ASSERT_MSG_ACT(a.add_loss(a), ==, false, "Unexpected overflow", success = false);
	dfp b(100'000'000,8);
	ASSERT_MSG_ACT(b.add_loss(b), ==, true, "Missing overflow", success = false);

	success &= test_comp();
/*
	utttil::dfloat<int, 4> x(21754190, 3);
	utttil::dfloat<int, 4> y(6992, 4);
	auto s = x;
	bool err = x.add_loss(y);
	std::cout << s << " + " << y << " = " << x << " " << err << std::endl;
	std::cout << x << " - " << y << " = ";
	err &= x.sub_loss(y);
	std::cout << x << " " << err << std::endl;
	if (err)
		return 1;
*/
	go_on = 1;
	std::thread t1([&](){ success &= test_add_sub_int32(); });
	std::thread t2([&](){ success &= test_add_sub_int32(); });
	std::this_thread::sleep_for(std::chrono::seconds(2));
	go_on = false;
	t1.join();
	t2.join();

	return !success;
}
