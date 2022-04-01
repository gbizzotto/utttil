
#include <atomic>
#include <thread>

#include <utttil/ring_buffer.hpp>
#include <utttil/assert.hpp>
#include <utttil/perf.hpp>

bool test_back()
{
	utttil::ring_buffer<int> rb(4);

	ASSERT_ACT(rb.capacity(), ==, 16ull, return false);
	ASSERT_ACT(rb.size()    , ==,  0ull, return false);
	ASSERT_ACT(rb.empty(), ==, true, return false);
	ASSERT_ACT(rb.full(), ==, false, return false);

	// push_back
	for (size_t i=0 ; i<rb.capacity() ; i++)
	{
		rb.push_back(i);
		ASSERT_MSG_ACT(rb.front   (), ==,   0, std::to_string(i), return false);
		ASSERT_MSG_ACT(rb.size    (), ==, i+1ull, std::to_string(i), return false);
		ASSERT_MSG_ACT(rb.capacity(), ==,  16ull, std::to_string(i), return false);
	}
	ASSERT_ACT(rb.full(), ==, true, return false);
	ASSERT_ACT(rb.empty(), ==, false, return false);

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
	C & operator=(const C & other)
	{
		value = other.value;
		return *this;
	}
};

bool test_object()
{
	utttil::ring_buffer<C> rb(5);


	// back
	rb.push_back(123);
	ASSERT_ACT(rb.front().value, ==, 123, return false);
	
	rb.emplace_back(321);
	ASSERT_ACT(rb.front().value, ==, 123, return false);

	{
		C c(111);
		rb.push_back(c);
	}
	ASSERT_ACT(rb.front().value, ==, 123, return false);

	{
		C c(222);
		rb.emplace_back(std::move(c));
	}
	ASSERT_ACT(rb.front().value, ==, 123, return false);

	return true;
}

bool test_stretches()
{
	utttil::ring_buffer<char> rb(10);

	auto t = rb.front_stretch();
	ASSERT_ACT(std::get<0>(t), ==, &rb.data[0], return false);
	ASSERT_ACT(std::get<1>(t), ==, 0ull, return false);
	t = rb.back_stretch();
	ASSERT_ACT(std::get<0>(t), ==, &rb.data[0], return false);
	ASSERT_ACT(std::get<1>(t), ==, rb.capacity(), return false);

	for (int i=0 ; i<100 ; i++)
		rb.push_back('a');

	ASSERT_ACT(rb.size(), ==, 100ull, return false);
	t = rb.front_stretch();
	ASSERT_ACT(std::get<0>(t), ==, &rb.data[0], return false);
	ASSERT_ACT(std::get<1>(t), ==, 100ull, return false);
	t = rb.back_stretch();
	ASSERT_ACT(std::get<0>(t), ==, &rb.data[100], return false);
	ASSERT_ACT(std::get<1>(t), ==, rb.capacity()-100, return false);

	rb.pop_front(50);

	ASSERT_ACT(rb.size(), ==, 50ull, return false);
	t = rb.front_stretch();
	ASSERT_ACT(std::get<0>(t), ==, &rb.data[50], return false);
	ASSERT_ACT(std::get<1>(t), ==, 50ull, return false);
	t = rb.back_stretch();
	ASSERT_ACT(std::get<0>(t), ==, &rb.data[100], return false);
	ASSERT_ACT(std::get<1>(t), ==, rb.capacity()-100, return false);

	rb.pop_front(50);

	ASSERT_ACT(rb.size(), ==, 0ull, return false);
	t = rb.front_stretch();
	ASSERT_ACT(std::get<0>(t), ==, &rb.data[100], return false);
	ASSERT_ACT(std::get<1>(t), ==, 0ull, return false);
	t = rb.back_stretch();
	ASSERT_ACT(std::get<0>(t), ==, &rb.data[100], return false);
	ASSERT_ACT(std::get<1>(t), ==, rb.capacity()-100, return false);

	for (int i=0 ; i<1024-50 ; i++)
		rb.push_back('a');

	ASSERT_ACT(rb.size(), ==, 1024ull-50, return false);
	t = rb.front_stretch();
	ASSERT_ACT(std::get<0>(t), ==, &rb.data[100], return false);
	ASSERT_ACT(std::get<1>(t), ==, rb.capacity()-100, return false);
	t = rb.back_stretch();
	ASSERT_ACT(std::get<0>(t), ==, &rb.data[50], return false);
	ASSERT_ACT(std::get<1>(t), ==, 50ull, return false);

	return true;
}

bool test_thread_safety_fuzz()
{
	std::atomic_bool go_on = true;
	const int total = 10000000;
	utttil::ring_buffer<int> rb(5);

	utttil::measurement_point mp("push_back + pop");
	utttil::measurement m(mp);

	std::thread twrite([&]()
		{
			for (int i=0 ; i<total && go_on ; i++)
				rb.push_back(i);
		});
	std::thread tread([&]()
		{
			for (int i=0 ; i<total; i++)
			{
				auto r = rb.front();
				rb.pop_front();
				if (r != i)
				{
					go_on = false;
					std::cout << "Unordered" << std::endl;
					return;
				}
				++mp.run_count;
			}
		});

	twrite.join();
	tread.join();

	ASSERT_ACT(mp.run_count, ==, (size_t)total, return 1);

	return true;
}

int main()
{
	bool success = true
		&& test_back()
		&& test_object()
		&& test_stretches()
		&& test_thread_safety_fuzz()
		;

	return success ? 0 : 1;
}