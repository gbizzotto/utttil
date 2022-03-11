
#include <atomic>
#include <thread>

#include <utttil/ring_buffer.hpp>
#include <utttil/assert.hpp>
#include <utttil/perf.hpp>

bool test_back()
{
	utttil::ring_buffer<int> rb(16);

	ASSERT_ACT(rb.capacity(), ==, 16ull, return false);
	ASSERT_ACT(rb.size()    , ==,  0ull, return false);
	ASSERT_ACT(rb.empty(), ==, true, return false);
	ASSERT_ACT(rb.full(), ==, false, return false);

	// push_back
	for (size_t i=0 ; i<rb.capacity() ; i++)
	{
		rb.push_back(i);
		ASSERT_MSG_ACT(rb.front   (), ==,   0, std::to_string(i), return false);
		ASSERT_MSG_ACT(rb.back    (), ==, (int)i  , std::to_string(i), return false);
		ASSERT_MSG_ACT(rb.size    (), ==, i+1ull, std::to_string(i), return false);
		ASSERT_MSG_ACT(rb.capacity(), ==,  16ull, std::to_string(i), return false);
	}
	ASSERT_ACT(rb.full(), ==, true, return false);
	ASSERT_ACT(rb.empty(), ==, false, return false);

	// pop_back
	for (size_t i=rb.capacity() ; i>0 ; i--)
	{
		ASSERT_MSG_ACT(rb.front   (), ==,   0, std::to_string(i), return false);
		ASSERT_MSG_ACT(rb.back    (), ==, (int)i-1, std::to_string(i), return false);
		ASSERT_MSG_ACT(rb.size    (), ==, i  , std::to_string(i), return false);
		ASSERT_MSG_ACT(rb.capacity(), ==,  16ull, std::to_string(i), return false);
		rb.pop_back();
	}
	ASSERT_ACT(rb.size(), ==, 0ull, return false);
	ASSERT_ACT(rb.full(), ==, false, return false);
	ASSERT_ACT(rb.empty(), ==, true, return false);

	return true;
}

bool test_front()
{
	utttil::ring_buffer<int> rb(16);

	ASSERT_ACT(rb.capacity(), ==, 16ull, return false);
	ASSERT_ACT(rb.size()    , ==,  0ull, return false);

	// push_back
	for (size_t i=0 ; i<rb.capacity() ; i++)
	{
		rb.push_front(i);
		ASSERT_MSG_ACT(rb.back    (), ==,   0, std::to_string(i), return false);
		ASSERT_MSG_ACT(rb.front   (), ==, (int)i  , std::to_string(i), return false);
		ASSERT_MSG_ACT(rb.size    (), ==, i+1, std::to_string(i), return false);
		ASSERT_MSG_ACT(rb.capacity(), ==,  16ull, std::to_string(i), return false);
	}
		ASSERT_ACT(rb.back(), ==, 0, return false);

	// pop_back
	for (size_t i=rb.capacity() ; i>0 ; i--)
	{
		ASSERT_MSG_ACT(rb.back    (), ==,   0, std::to_string(i), return false);
		ASSERT_MSG_ACT(rb.front   (), ==, (int)i-1, std::to_string(i), return false);
		ASSERT_MSG_ACT(rb.size    (), ==, i  , std::to_string(i), return false);
		ASSERT_MSG_ACT(rb.capacity(), ==,  16ull, std::to_string(i), return false);
		rb.pop_front();
	}
	ASSERT_ACT(rb.size(), ==, 0ull, return false);
	return true;
}

class C
{
public:
	int value;
	inline static size_t counter = 0;
	C()
		: value(0)
	{
		++counter;
	}
	C(int v)
		: value(v)
	{
		++counter;
	}
	C(const C & other)
		: value(other.value)
	{
		++counter;
	}
	C(C && other)
		: value(other.value)
	{
		++counter;
	}
	~C()
	{
		value = 0;
		--counter;
	}
};

bool test_object()
{
	utttil::ring_buffer<C> rb(23);

	ASSERT_ACT(0ull, ==, C::counter, return false);

	// back
	rb.push_back(123);
	ASSERT_ACT(rb.front().value, ==, 123, return false);
	ASSERT_ACT(rb.back().value, ==, 123, return false);
	ASSERT_ACT(rb.size(), ==, C::counter, return false);
	
	rb.emplace_back(321);
	ASSERT_ACT(rb.front().value, ==, 123, return false);
	ASSERT_ACT(rb.back().value, ==, 321, return false);
	ASSERT_ACT(rb.size(), ==, C::counter, return false);

	{
		C c(111);
		rb.push_back(c);
	}
	ASSERT_ACT(rb.front().value, ==, 123, return false);
	ASSERT_ACT(rb.back().value, ==, 111, return false);
	ASSERT_ACT(rb.size(), ==, C::counter, return false);

	{
		C c(222);
		rb.emplace_back(std::move(c));
	}
	ASSERT_ACT(rb.front().value, ==, 123, return false);
	ASSERT_ACT(rb.back().value, ==, 222, return false);
	ASSERT_ACT(rb.size(), ==, C::counter, return false);

	while(rb.size())
	{
		rb.pop_back();
		ASSERT_ACT(rb.size(), ==, C::counter, return false);
	}


	// front
	rb.push_front(123);
	ASSERT_ACT(rb.back().value, ==, 123, return false);
	ASSERT_ACT(rb.front().value, ==, 123, return false);
	ASSERT_ACT(rb.size(), ==, C::counter, return false);
	
	rb.emplace_front(321);
	ASSERT_ACT(rb.back().value, ==, 123, return false);
	ASSERT_ACT(rb.front().value, ==, 321, return false);
	ASSERT_ACT(rb.size(), ==, C::counter, return false);

	{
		C c(111);
		rb.push_front(c);
	}
	ASSERT_ACT(rb.back().value, ==, 123, return false);
	ASSERT_ACT(rb.front().value, ==, 111, return false);
	ASSERT_ACT(rb.size(), ==, C::counter, return false);

	{
		C c(222);
		rb.emplace_front(std::move(c));
	}
	ASSERT_ACT(rb.back().value, ==, 123, return false);
	ASSERT_ACT(rb.front().value, ==, 222, return false);
	ASSERT_ACT(rb.size(), ==, C::counter, return false);

	while(rb.size())
	{
		rb.pop_front();
		ASSERT_ACT(rb.size(), ==, C::counter, return false);
	}

	return true;
}

bool test_thread_safety_fuzz()
{
	std::atomic_bool go_on = true;
	const int total = 10000000;
	utttil::ring_buffer<size_t> rb(23);

	utttil::measurement_point mp("pup_back + pop");
	utttil::measurement m(mp);

	std::thread twrite([&]()
		{
			for (int i=0 ; i<total && go_on ; i++)
			{
				try {
					while(rb.full() && go_on)
					{}
				} catch (std::exception &) {
					std::cout << "exception in full()" << std::endl;
				}
				try {
					rb.push_back(i);
				} catch (std::exception &) {
					std::cout << "exception in push_back()" << std::endl;
				}
			}
		});
	int res = -1;
	std::thread tread([&]()
		{
			for (int i=0 ; i<total; i++)
			{
				try {
					while(rb.empty())
					{}
				} catch (std::exception &) {
					std::cout << "exception in empty()" << std::endl;
				}
				int r = -1;
				try {
					r = rb.front();
				} catch (std::exception &) {
					std::cout << "exception in front()" << std::endl;
				}
				try {
					rb.pop_front();
				} catch (std::exception &) {
					std::cout << "exception in pop_front()" << std::endl;
				}
				if (r != res+1)
				{
					go_on = false;
					std::cout << "Unordered" << std::endl;
					return;
				}
				res = r;
			}
		});

	twrite.join();
	tread.join();

	ASSERT_ACT(res, ==, total-1, return 1);
	mp.run_count = total;

	return true;
}

int main()
{
	bool success = true
		&& test_back()
		&& test_front()
		&& test_object()
		&& test_thread_safety_fuzz()
		;

	return success ? 0 : 1;
}