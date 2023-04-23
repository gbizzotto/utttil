
#pragma once

#include <memory>
#include <type_traits>
#include <limits>
#include <exception>
#include <algorithm>

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
	index_t capacity_;

	explicit fixed_pool()
		: collection(nullptr)
		, first_free(0)
		, size_(0)
		, capacity_(0)
	{}

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
	index_t index_of(T * ptr) const { return ((Obj*) ptr) - &collection[0]; }
	index_t index_of(T & ptr) const { return ((Obj*)&ptr) - &collection[0]; }
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
	
	template<typename Serializer>
	void serialize(Serializer && s) const
	{
		std::vector<index_t> free_cells_idx;
		free_cells_idx.reserve(capacity()-size());
		index_t i = first_free;
		for (size_t s=size() ; s>0 ; s--)
			free_cells_idx.push_back(i);
		std::sort(free_cells_idx.begin(), free_cells_idx.end());

		s << capacity()
		  << size();
		for (i=0 ; i<size() ; i++)
			if (std::find(free_cells_idx.begin(), free_cells_idx.end(), i) == free_cells_idx.end())
				s << i
				  << collection[i].t;
	}
	template<typename Deserializer>
	void deserialize(Deserializer && s)
	{
		if (collection != nullptr)
		{
			this->~fixed_pool();
		}
		s >> capacity_
		  >> size_;
		collection = (Obj*)malloc(sizeof(Obj) * capacity_);
		std::vector<index_t> used_cells_idx;
		used_cells_idx.reserve(size_);
		for (size_t i=0 ; i<size_ ; i++)
		{
			index_t idx;
			s >> idx;
			s >> collection[idx].t;
			used_cells_idx.push_back(idx);
		}
		for (index_t idx=capacity_ ; idx>0 ; )
		{
			idx--;
			if (std::find(used_cells_idx.begin(), used_cells_idx.end(), idx) == used_cells_idx.end())
			{
				collection[idx].next_index = first_free;
				first_free = idx;
			}
		}
	}
};

template<typename T> using fixed_pool_tiny  = fixed_pool<T,std::uint8_t>;
template<typename T> using fixed_pool_large = fixed_pool<T,std::uint64_t>;

template<typename T, typename Index>
bool operator==(const fixed_pool<T,Index> & left, const fixed_pool<T,Index> & right)
{
	// test the basics
	if (left.size() != right.size() || left.capacity() != right.capacity())
		return false;

	// find free cells on left
	std::vector<Index> left_free_cells_idx;
	left_free_cells_idx.reserve(left.capacity()-left.size());
	auto i = left.first_free;
	for (size_t s=left.size() ; s>0 ; s--)
		left_free_cells_idx.push_back(i);
	std::sort(left_free_cells_idx.begin(), left_free_cells_idx.end());

	// find free cells on right
	std::vector<Index> right_free_cells_idx;
	right_free_cells_idx.reserve(right.capacity()-right.size());
	i = right.first_free;
	for (size_t s=right.size() ; s>0 ; s--)
		right_free_cells_idx.push_back(i);
	std::sort(right_free_cells_idx.begin(), right_free_cells_idx.end());

	// compare free cells
	for (size_t idx=0 ; idx<left_free_cells_idx.size() ; idx++)
		if (left_free_cells_idx[idx] != right_free_cells_idx[idx])
			return false;

	// compare used cells
	for (size_t idx=0 ; idx<left.capacity() ; idx++)
		if (std::find(left_free_cells_idx.begin(), left_free_cells_idx.end(), i) == left_free_cells_idx.end())
			if (left.element_at(idx) != right.element_at(idx))
				return false;

	return true;
}

} // namespace
