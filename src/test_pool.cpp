
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

bool test_fuzz()
{
	auto seed = time(NULL); // 1646605269;
	std::cout << "Seed: " << seed << std::endl;
	srand(seed);

	utttil::fixed_pool<int> pool(1024*1024);
	std::set<int*> ptrs;

	for (int i=0 ; i<1024 ; i++)
	{
		int * ptr = pool.alloc();
		ASSERT_ACT(ptrs.find(ptr) == ptrs.end(), ==, true, return false);
		ptrs.insert(ptr);
	}

	size_t insert_count = 0;
	size_t delete_count = 0;

	for ( auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10)
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
			int * ptr = pool.alloc();
			ASSERT_MSG_ACT(ptrs.find(ptr) == ptrs.end(), ==, true, std::to_string(i), return false);
			ptrs.insert(ptr);
		}
	}

	std::cout << insert_count << " inserts, " << delete_count << " deletes." << std::endl;

	return true;
}

int main()
{
	bool success = true
		&& test_basics()
		&& test_full()
		&& test_holes()
		&& test_fuzz()
		;

	return success ? 0 : 1;
}
