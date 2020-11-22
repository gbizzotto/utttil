
#pragma once

namespace utttil {

template<typename T>
struct object_pool
{
	std::deque<T> collection;
	std::deque<size_t> free_idx;

	T* alloc()
	{
		if ( ! free_idx.empty())
		{
			T *ptr = &collection[free_idx.back()];
			free_idx.pop_back();
			return ptr;
		}
		collection.emplace_back();
		collection.back().mempool_idx = collection.size()-1;
		return & collection.back();
	}

	void free(T *ptr)
	{
		size_t idx = ptr->mempool_idx;
		free_idx.push_back(idx);
	}
};

} // namespace
