
#pragma once

#include <memory>
#include <type_traits>
#include <limits>
#include <exception>

namespace utttil {

template<typename T, typename Index=std::uint32_t>
struct fixed_pool
{
	using index_t = Index;

	union Obj
	{
		T t;
		Index next_index;
	};

	Obj * collection;
	index_t first_free;
	index_t size_;
	const index_t capacity_;

	fixed_pool(size_t capacity)
		: collection((Obj*)malloc(sizeof(Obj) * capacity))
		, first_free(0)
		, size_(0)
		, capacity_(capacity)
	{
		if ( ! collection)
			throw std::bad_alloc();
		if (std::numeric_limits<index_t>::max() < capacity)
			throw std::invalid_argument("pool dynamic size too big for static index type");
		for (index_t i=0 ; i<capacity-1 ; ++i)
			collection[i].next_index = i+1;
	}
	~fixed_pool()
	{
		for_each([](T & t){ t.~T(); });
		::free(collection);
	}

	size_t size() const { return size_; }
	size_t capacity() const { return capacity_; }
	bool empty() const { return size_ == 0; }
	bool full() const { return size_ == capacity_; }

	template<typename...Args>
	T * alloc(Args...args)
	{
		if (full())
			return nullptr;

		T * result = &collection[first_free].t;
		first_free = collection[first_free].next_index;
		++size_;

		new (result) T(args...);
		return result;
	}

	void free(T * ptr)
	{
		index_t idx = index_of(ptr);

		ptr->~T();
		collection[idx].next_index = first_free;
		first_free = idx;
		--size_;
	}

	bool contains(T * ptr)
	{
		return ptr >= &collection[0].t && ptr < &collection[capacity_].t;
	}
	index_t index_of(T * ptr) { return ((Obj*) ptr) - &collection[0]; }
	index_t index_of(T & ptr) { return ((Obj*)&ptr) - &collection[0]; }
	const T & element_at(index_t idx) const
	{
		return collection[idx].t;
	}
	T & element_at(index_t idx)
	{
		return collection[idx].t;
	}

	template<typename F>
	void for_each(F f)
	{
		std::vector<bool> constructed(capacity(), true);
		size_t free_cells = capacity() - size();
		for ( index_t idx = first_free
			; free_cells-- != 0
			; idx = collection[idx].next_index)
		{
			constructed[idx] = false;
		}
		for ( index_t idx = 0
			; idx < capacity()
			; idx++ )
		{
			if (constructed[idx])
				f(collection[idx].t);
		}
	}
};

template<typename T> using fixed_pool_tiny  = fixed_pool<T,std::uint8_t>;
template<typename T> using fixed_pool_large = fixed_pool<T,std::uint64_t>;

} // namespace
