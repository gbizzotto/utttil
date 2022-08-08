
#include <stdlib.h>
#include <vector>
#include <chrono>
#include <set>

#include <utttil/pool.hpp>
#include <utttil/assert.hpp>

bool test_basics()
{
	utttil::fixed_pool<int> pool(1024);

	ASSERT_ACT(pool.size(), ==, 0ull, return false);
	ASSERT_ACT(pool.empty(), ==, true, return false);
	ASSERT_ACT(pool.full(), ==, false, return false);

	int * a = pool.alloc();

	ASSERT_ACT(a, !=, nullptr, return false);
	ASSERT_ACT(pool.size(), ==, 1ull, return false);
	ASSERT_ACT(pool.empty(), ==, false, return false);
	ASSERT_ACT(pool.full(), ==, false, return false);

	*a = 1234;

	pool.free(a);

	ASSERT_ACT(pool.size(), ==, 0ull, return false);
	ASSERT_ACT(pool.empty(), ==, true, return false);
	ASSERT_ACT(pool.full(), ==, false, return false);

	return true;
}

bool test_full()
{
	utttil::fixed_pool<int> pool(1024);

	int *ptrs[1024];

	for (int i=0 ; i<1024 ; i++)
	{
		ptrs[i] = pool.alloc();
		*ptrs[i] = 1000000 + i;
	}

	ASSERT_ACT(pool.size(), ==, 1024ull, return false);
	ASSERT_ACT(pool.empty(), ==, false, return false);
	ASSERT_ACT(pool.full(), ==, true,  return false);

	for (int i=0 ; i<1024 ; i++)
	{
		ASSERT_MSG_ACT(*ptrs[i], ==, 1000000 + i, std::to_string(i), return false);
		pool.free(ptrs[i]);
	}

	ASSERT_ACT(pool.size(), ==, 0ull, return false);
	ASSERT_ACT(pool.empty(), ==, true, return false);
	ASSERT_ACT(pool.full(), ==, false,  return false);

	return true;
}

bool test_holes()
{
	utttil::fixed_pool<int> pool(1024);

	std::vector<int*> ptrs;
	std::vector<int> values;

	for (int i=0 ; i<1024 ; i++)
	{
		ptrs.push_back(pool.alloc());
		*ptrs.back() = 1000000 + i;
		values.push_back(1000000 + i);
	}

	ASSERT_ACT(pool.size(), ==, 1024ull, return false);
	ASSERT_ACT(pool.empty(), ==, false, return false);
	ASSERT_ACT(pool.full(), ==, true,  return false);
	for (size_t i=0 ; i<ptrs.size() ; i++)
		ASSERT_MSG_ACT(*ptrs[i], ==, values[i], std::to_string(i), return false);

	for (int i=0 ; i<128 ; i++)
	{
		int idx = rand() % ptrs.size();
		int * ptr = ptrs[idx];
		ptrs.erase(ptrs.begin()+idx);
		values.erase(values.begin()+idx);
		pool.free(ptr);
	}

	ASSERT_ACT(pool.size(), ==, 1024ull-128ull, return false);
	ASSERT_ACT(pool.empty(), ==, false, return false);
	ASSERT_ACT(pool.full(), ==, false,  return false);
	for (size_t i=0 ; i<ptrs.size() ; i++)
		ASSERT_MSG_ACT(*ptrs[i], ==, values[i], std::to_string(i), return false);

	for (int i=0 ; i<128 ; i++)
	{
		ptrs.push_back(pool.alloc());
		*ptrs.back() = 1000000 + i;
		values.push_back(1000000 + i);
	}

	ASSERT_ACT(pool.size(), ==, 1024ull, return false);
	ASSERT_ACT(pool.empty(), ==, false, return false);
	ASSERT_ACT(pool.full(), ==, true,  return false);
	for (size_t i=0 ; i<ptrs.size() ; i++)
		ASSERT_MSG_ACT(*ptrs[i], ==, values[i], std::to_string(i), return false);

	return true;
}

bool test_contains()
{
	utttil::fixed_pool<int> pool1(16);
	utttil::fixed_pool<int> pool2(1024);

	int * p1_i1 = pool1.alloc();
	int * p1_i2 = pool1.alloc();

	int * p2_i1 = pool2.alloc();
	int * p2_i2 = pool2.alloc();

	ASSERT_ACT(pool1.contains(p1_i1), ==, true, return false);
	ASSERT_ACT(pool1.contains(p1_i2), ==, true, return false);
	ASSERT_ACT(pool1.contains(p2_i1), ==, false, return false);
	ASSERT_ACT(pool1.contains(p2_i2), ==, false, return false);

	ASSERT_ACT(pool2.contains(p2_i1), ==, true, return false);
	ASSERT_ACT(pool2.contains(p2_i2), ==, true, return false);
	ASSERT_ACT(pool2.contains(p1_i1), ==, false, return false);
	ASSERT_ACT(pool2.contains(p1_i2), ==, false, return false);

	return true;
}

bool test_index_of()
{
	utttil::fixed_pool<int> pool1(16);
	utttil::fixed_pool<int> pool2(1024);

	int * p1_i1 = pool1.alloc();
	int * p1_i2 = pool1.alloc();

	int * p2_i1 = pool2.alloc();
	int * p2_i2 = pool2.alloc();

	ASSERT_ACT(pool1.index_of(p1_i1), ==, 0u, return false);
	ASSERT_ACT(pool1.index_of(p1_i2), ==, 1u, return false);
	ASSERT_ACT(&pool1.element_at(0), ==, p1_i1, return false);
	ASSERT_ACT(&pool1.element_at(1), ==, p1_i2, return false);

	ASSERT_ACT(pool2.index_of(p2_i1), ==, 0u, return false);
	ASSERT_ACT(pool2.index_of(p2_i2), ==, 1u, return false);
	ASSERT_ACT(&pool2.element_at(0), ==, p2_i1, return false);
	ASSERT_ACT(&pool2.element_at(1), ==, p2_i2, return false);

	pool1.free(p1_i1);
	int * p1_i3 = pool1.alloc();
	ASSERT_ACT(pool1.index_of(p1_i3), ==, 0u, return false);
	ASSERT_ACT(&pool1.element_at(0), ==, p1_i3, return false);
	ASSERT_ACT(&pool1.element_at(1), ==, p1_i2, return false);

	pool2.free(p2_i1);
	int * p2_i3 = pool2.alloc();
	ASSERT_ACT(pool2.index_of(p2_i3), ==, 0u, return false);
	ASSERT_ACT(&pool2.element_at(0), ==, p2_i3, return false);
	ASSERT_ACT(&pool2.element_at(1), ==, p2_i2, return false);

	return true;
}

bool test_fuzz()
{
	auto x = std::make_shared<int>(123);
	{
		using T = decltype(x);
		utttil::fixed_pool<T> pool(1024*1024);
		std::set<T*> ptrs;

		for (int i=0 ; i<1024 ; i++)
		{
			T * ptr = pool.alloc(x);
			ASSERT_ACT(ptrs.find(ptr) == ptrs.end(), ==, true, return false);
			ptrs.insert(ptr);
		}

		size_t insert_count = 0;
		size_t delete_count = 0;

		for ( auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3)
			; std::chrono::steady_clock::now() < deadline
			; )
		{
			int n = rand() % ptrs.size();
			delete_count += n;
			for (int i=0 ; i<n ; i++)
			{
				auto it = std::next(ptrs.begin(), ((rand() % ptrs.size())));
				pool.free(*it);
				ptrs.erase(it);
			}		
			int m = rand() % ptrs.size()+100;
			insert_count += m;
			for (int i=0 ; i<m ; i++)
			{
				T * ptr = pool.alloc(x);
				ASSERT_MSG_ACT(ptrs.find(ptr) == ptrs.end(), ==, true, std::to_string(i), return false);
				ptrs.insert(ptr);
			}
		}

		ASSERT_ACT(pool.size(), ==, (size_t)x.use_count()-1, return false);

		size_t for_each_count = 0;
		pool.for_each([&](T &){ for_each_count++; });
		ASSERT_ACT(for_each_count, ==, pool.size(), return false);

		std::cout << insert_count << " inserts, " << delete_count << " deletes." << std::endl;
	}
	ASSERT_ACT(0, ==, x.use_count()-1, return false);

	return true;
}

int main()
{
	auto seed = time(NULL); // 1646605269;
	std::cout << "Seed: " << seed << std::endl;
	srand(seed);

	bool success = true
		&& test_basics()
		&& test_full()
		&& test_holes()
		&& test_contains()
		&& test_index_of()
		&& test_fuzz()
		;

	if (success)
		std::cout << "Success" << std::endl;
	else
		std::cout << "Failure" << std::endl;

	return success ? 0 : 1;
}
