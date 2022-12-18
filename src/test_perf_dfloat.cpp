
#include <utttil/perf.hpp>
#include <utttil/dfloat.hpp>

template <class T>
__attribute__((always_inline)) inline void DoNotOptimize(const T &value) {
  asm volatile("" : "+m"(const_cast<T &>(value)));
}

bool test_add_mul_32()
{
	utttil::measurement_point mp("add_w_mul_32");

	using df = utttil::dfloat<int, 4>;
	for ( auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(1)
	    ; std::chrono::steady_clock::now() < deadline
	    ; )
	{
		df a = df::rand();
		df b = df::rand();
		a.exponent = 5;
		b.exponent = 3;
		b.mantissa /= 100;
		{
			utttil::measurement m(mp);
			//DoNotOptimize(m);
			bool ok = a.add_loss(b);
			DoNotOptimize(ok);
		}
	}
	return true;
}

bool test_add_div_32()
{
	utttil::measurement_point mp("add_w_div_32");

	using df = utttil::dfloat<int, 4>;
	for ( auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(1)
	    ; std::chrono::steady_clock::now() < deadline
	    ; )
	{
		df a = df::rand();
		df b = df::rand();
		a.exponent = 5;
		a.mantissa /= 100;
		a.mantissa *= 100;
		b.exponent = 3;
		b.mantissa = 26843545+(rand()%10000);
		{
			utttil::measurement m(mp);
			//DoNotOptimize(m);
			bool ok = a.add_loss(b);
			DoNotOptimize(ok);
		}
	}
	return true;
}
bool test_sub_mul_32()
{
	utttil::measurement_point mp("sub_w_mul_32");

	using df = utttil::dfloat<int, 4>;
	for ( auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(1)
	    ; std::chrono::steady_clock::now() < deadline
	    ; )
	{
		df a = df::rand();
		df b = df::rand();
		a.exponent = 5;
		b.exponent = 3;
		b.mantissa /= 100;
		{
			utttil::measurement m(mp);
			//DoNotOptimize(m);
			bool ok = a.sub_loss(b);
			DoNotOptimize(ok);
		}
	}
	return true;
}

bool test_sub_div_32()
{
	utttil::measurement_point mp("sub_w_div_32");

	using df = utttil::dfloat<int, 4>;
	for ( auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(1)
	    ; std::chrono::steady_clock::now() < deadline
	    ; )
	{
		df a = df::rand();
		df b = df::rand();
		a.exponent = 5;
		a.mantissa /= 100;
		a.mantissa *= 100;
		b.exponent = 3;
		b.mantissa = 26843545+(rand()%10000);
		{
			utttil::measurement m(mp);
			//DoNotOptimize(m);
			bool ok = a.sub_loss(b);
			DoNotOptimize(ok);
		}
	}
	return true;
}

int main()
{
	bool success = true
		&& test_add_mul_32()
		&& test_add_div_32()
		&& test_sub_mul_32()
		&& test_sub_div_32()
		;

	if (success)
		std::cout << "Success" << std::endl;
	else
		std::cout << "Failure" << std::endl;

	return success ? 0 : 1;
}