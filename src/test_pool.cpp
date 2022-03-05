
#include <stdlib.h>
#include <vector>

#include <utttil/pool.hpp>
#include <utttil/assert.hpp>

bool test_basics()
{
	utttil::pool<int> pool(1024);

	ASSERT_ACT(pool.size(), ==, 0, return false);
	ASSERT_ACT(pool.empty(), ==, true, return false);
	ASSERT_ACT(pool.full(), ==, false, return false);

	int * a = pool.alloc();

	ASSERT_ACT(a, !=, nullptr, return false);
	ASSERT_ACT(pool.size(), ==, 1, return false);
	ASSERT_ACT(pool.empty(), ==, false, return false);
	ASSERT_ACT(pool.full(), ==, false, return false);

	*a = 1234;

	pool.free(a);

	ASSERT_ACT(pool.size(), ==, 0, return false);
	ASSERT_ACT(pool.empty(), ==, true, return false);
	ASSERT_ACT(pool.full(), ==, false, return false);

	return true;
}

bool test_full()
{
	utttil::pool<int> pool(1024);

	int *ptrs[1024];

	for (int i=0 ; i<1024 ; i++)
	{
		ptrs[i] = pool.alloc();
		*ptrs[i] = 1000000 + i;
	}

	ASSERT_ACT(pool.size(), ==, 1024, return false);
	ASSERT_ACT(pool.empty(), ==, false, return false);
	ASSERT_ACT(pool.full(), ==, true,  return false);

	for (int i=0 ; i<1024 ; i++)
	{
		ASSERT_MSG_ACT(*ptrs[i], ==, 1000000 + i, std::to_string(i), return false);
		pool.free(ptrs[i]);
	}

	ASSERT_ACT(pool.size(), ==, 0, return false);
	ASSERT_ACT(pool.empty(), ==, true, return false);
	ASSERT_ACT(pool.full(), ==, false,  return false);

	return true;
}

bool test_holes()
{
	utttil::pool<int> pool(1024);

	std::vector<int*> ptrs;
	std::vector<int> values;

	for (int i=0 ; i<1024 ; i++)
	{
		ptrs.push_back(pool.alloc());
		*ptrs.back() = 1000000 + i;
		values.push_back(1000000 + i);
	}

	ASSERT_ACT(pool.size(), ==, 1024, return false);
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

	ASSERT_ACT(pool.size(), ==, 1024-128, return false);
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

	ASSERT_ACT(pool.size(), ==, 1024, return false);
	ASSERT_ACT(pool.empty(), ==, false, return false);
	ASSERT_ACT(pool.full(), ==, true,  return false);
	for (size_t i=0 ; i<ptrs.size() ; i++)
		ASSERT_MSG_ACT(*ptrs[i], ==, values[i], std::to_string(i), return false);

	return true;
}

int main()
{
	bool success = true
		&& test_basics()
		&& test_full()
		&& test_holes()
		;

	return success ? 0 : 1;
}