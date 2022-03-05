
#pragma once

#include <memory>
#include <type_traits>
#include <limits>
#include <exception>

namespace utttil {

template<typename T, typename Index=std::uint32_t>
struct pool
{
	union Obj
	{
		T t;
		Index next_index;
	};

	std::unique_ptr<Obj[]> collection;
	Index first_free;
	Index size_;
	const Index capacity_;

	pool(size_t size)
		: collection(std::make_unique<Obj[]>(size))
		, first_free(0)
		, size_(0)
		, capacity_(size)
	{
		if (std::numeric_limits<Index>::max() < size)
			throw std::invalid_argument("pool dynamic size too big for static index type");
	}

	size_t size() const { return size_; }
	size_t capacity() const { return capacity_; }
	bool empty() const { return size_ == 0; }
	bool full() const { return size_ == capacity_; }

	T* alloc()
	{
		if (full())
			return nullptr;

		T * result = &collection[first_free].t;
		if (first_free == size())
			++first_free;
		else
			first_free = collection[first_free].next_index;

		++size_;

		return result;
	}

	void free(T * ptr)
	{
		Index idx = ((Obj*)ptr) - &collection[0];

		collection[idx].next_index = first_free;
		first_free = idx;

		--size_;
	}
};

template<typename T> using pool_tiny = pool<T,std::uint8_t>;
template<typename T> using pool_large = pool<T,std::uint64_t>;

} // namespace
