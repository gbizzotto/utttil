
#pragma once

#include <atomic>
#include <thread>

namespace utttil {

struct spinlock{
	std::atomic_flag flag;

	inline spinlock()
		: flag(ATOMIC_FLAG_INIT)
	{}

	inline void lock()
	{
		while( flag.test_and_set() );
	}

	inline void unlock()
	{
		flag.clear();
	}
};

} // namespace
