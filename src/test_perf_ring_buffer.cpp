
#include <typeinfo>
#include <string>
#include <chrono>

#include <utttil/no_init.hpp>
#include <utttil/perf.hpp>

#include <utttil/ring_buffer.hpp>

template<typename T>
bool test(T default_value)
{
	utttil::measurement_point mpf(std::string("pop_front ").append(typeid(T).name()));
	utttil::measurement_point mpb(std::string("push_back ").append(typeid(T).name()));

	utttil::ring_buffer<T, 10> rb;

	for(size_t i=0 ; i<rb.capacity()/2 ; i++)
		rb.push_back(default_value);

	for ( auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(1)
		; std::chrono::steady_clock::now() < deadline
		; )
	{
		{
			utttil::measurement m(mpb);
			rb.push_back(default_value);
		}
		{
			utttil::measurement m(mpf);
			rb.pop_front();
		}
	}
	return true;
}

int main()
{
	bool success = true
		&& test<char>('a')
		&& test<utttil::no_init<char>>('b')
		&& test<int>(12345)
		&& test<std::string>("abcdefghij")
		;

	return success ? 0 : 1;
}