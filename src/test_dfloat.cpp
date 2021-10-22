
#include "headers.hpp"

#include <iostream>
#include <thread>

#include "utttil/dfloat.hpp"
#include "utttil/assert.hpp"
#include "utttil/unique_int.hpp"

volatile char go_on;

bool test_add_int32()
{
	using df = utttil::dfloat<int, 4>;
	for (int i=0 ; i<10 ; i++)
	{
		df a = df::rand();
		df b = df::rand();
		df c = a;
		bool loss = c.add_loss(b);
		if ( ! loss)
		{
			bool success = true;
			ASSERT_MSG_ACT(c.sub_loss(b), ==, false, "Overflow error", success = false);
			ASSERT_MSG_ACT(a, ==, c, std::string("add/sub error, i=").append(std::to_string(i)), success = false);
		}
	}
	return true;
}

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

	ASSERT_ACT(lot_count_t(1234)*round_lot_t(4321), ==, quantity_t(5332114), success = false);
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

	// dfloat<int, 4> x(0, 0);
	// dfloat<int, 4> y(192833, 11);
	// std::cout << x << " " << y << std::endl;
	// bool err = x.add_loss(y);
	// std::cout << x << " " << err << std::endl;


	go_on = 1;
	std::thread t([&](){ success &= test_add_int32(); });
	for (auto timeout = std::chrono::high_resolution_clock::now() + std::chrono::seconds(2) ; std::chrono::high_resolution_clock::now() < timeout ; )
		;
	go_on = false;
	t.join();

	return !success;
}
