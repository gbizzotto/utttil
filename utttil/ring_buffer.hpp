
#pragma once

#include <immintrin.h>
#include <memory>
#include <atomic>
#include <assert.h>

namespace utttil {

template<typename T, size_t SizeBits>
struct ring_buffer
{
	using value_type = T;
	inline static constexpr size_t Capacity = (1<<SizeBits);
	inline static constexpr size_t Mask = Capacity-1;

	T data[Capacity];
	std::atomic_size_t front_ = 0;
	std::atomic_size_t back_ = 0;

	struct iterator
	{
		ring_buffer * rb;
		size_t pos;
		iterator & operator++() { ++pos; return *this; }
		T & operator*() { return rb->data[pos & Mask]; }
		T * operator->() { return &rb->data[pos & Mask]; }
		bool operator==(const iterator & other) { return (pos & Mask) == (other.pos & Mask); }
		bool operator!=(const iterator & other) { return (pos & Mask) != (other.pos & Mask); }
	};
	iterator begin() { return iterator{this, front_}; }
	iterator   end() { return iterator{this,  back_}; }

	void erase(iterator it, const iterator & end_)
	{
		//std::cout << "ring_buffer sizeof(T) " << sizeof(T) << ", erase" << std::endl;
		if (it != begin())
			throw std::exception();
		for ( ; it!=end_ ; ++it)
		{
			if (it == end())
				throw std::exception();
			pop_front();
		}
	}

	void clear()
	{
		//std::cout << "ring_buffer sizeof(T) " << sizeof(T) << ", clear" << std::endl;
		while( ! empty())
			pop_front();
	}

	size_t capacity() const
	{
		return Capacity;
	}
	size_t size() const
	{
		return back_-front_;
	}
	size_t free_size() const
	{
		return capacity() - size();
	}
	bool empty() const
	{
		return front_ == back_;
	}
	bool full() const
	{
		return size() == capacity();
	}

	T & front()
	{
		while (empty())
			_mm_pause();
		return data[front_ & Mask];
	}
	T & back()
	{
		while (full())
			_mm_pause();
		return data[back_ & Mask];
	}
	const T & front() const
	{
		while (empty())
			_mm_pause();
		return data[front_ & Mask];
	}
	const T & back() const
	{
		while (full())
			_mm_pause();
		return data[back_ & Mask];
	}

	void pop_front(size_t n)
	{
		while (n>0)
		{
			while (empty())
				_mm_pause();
			size_t s = std::min(size(), n);
			front_ += s;
			n -= s;
		}
	}
	void pop_front()
	{
		while (empty())
			_mm_pause();
		++front_;
	}

	std::tuple<T*,size_t> back_stretch()
	{
		// TODO: branchless
		if (back_ == front_+capacity())
			return std::make_tuple(
					&data[back_ & Mask],
					0
				);
		if ((back_ & Mask) >= (front_ & Mask)) {
			assert(capacity() - (back_ & Mask) <= free_size());
			return std::make_tuple(
					&data[back_ & Mask],
					capacity() - (back_ & Mask)
				);
		} else {
			assert((front_ & Mask) - (back_ & Mask) <= free_size());
			return std::make_tuple(
					&data[back_ & Mask],
					(front_ & Mask) - (back_ & Mask)
				);
		}
	}
	std::tuple<T*,size_t> back_stretch_2()
	{
		// TODO: branchless
		if (back_ == front_+capacity())
			return std::make_tuple(
					&data[front_ & Mask],
					0
				);
		if ((back_ & Mask) >= (front_ & Mask)) {
			assert((front_ & Mask) <= free_size());
			return std::make_tuple(
					data,
					front_ & Mask
				);
		} else {
			return std::make_tuple(
					&data[front_ & Mask],
					0
				);
		}
	}
	std::tuple<T*,size_t> front_stretch()
	{
		// TODO: branchless
		if (back_ == front_) 
			return std::make_tuple(
					&data[front_ & Mask],
					0
				);
		if ((back_ & Mask) > (front_ & Mask)) {
			assert(back_ - front_ <= capacity());
			return std::make_tuple(
					&data[front_ & Mask],
					back_ - front_
				);
		} else {
			assert(capacity() - (front_ & Mask) <= size());
			return std::make_tuple(
					&data[front_ & Mask],
					capacity() - (front_ & Mask)
				);
		}
	}
	std::tuple<T*,size_t> front_stretch_2()
	{
		// TODO: branchless
		if (back_ == front_) 
			return std::make_tuple(
					&data[back_ & Mask],
					0
				);
		if ((back_ & Mask) > (front_ & Mask)) {
			return std::make_tuple(
					&data[back_ & Mask],
					0
				);
		} else {
			assert((back_ & Mask) < size());
			return std::make_tuple(
					data,
					back_ & Mask
				);
		}
	}
	void advance_back(size_t n)
	{
		assert(n <= free_size());
		back_ += n;
	}
	void advance_front(size_t n)
	{
		assert(n <= size());
		front_ += n;
	}

	T & push_back()
	{
		while (full())
			_mm_pause();
		T & result = data[back_ & Mask];
		++back_;
		return result;
	}
	T & push_back(T && t)
	{
		while (full())
			_mm_pause();
		data[back_ & Mask] = std::forward<T>(t);
		T & result = data[back_ & Mask];
		++back_;
		return result;
	}
	T & push_back(const T & t)
	{
		while (full())
			_mm_pause();
		data[back_ & Mask] = t;
		T & result = data[back_ & Mask];
		++back_;
		return result;
	}
	template<typename ...P>
	T & emplace_back(P... params)
	{
		while (full())
			_mm_pause();
		data[back_ & Mask] = T(params...);
		T & result = data[back_ & Mask];
		++back_;
		return result;
	}
};

template<typename Out, typename T, size_t Capacity>
Out & operator<<(Out & out, const utttil::ring_buffer<T, Capacity> & rb)
{
	return out << "ring_buffer sizeof(T): " << sizeof(T) << ", capacity: " << rb.capacity() << ", front_: " << rb.front_ << ", back: " << rb.back_;
}

} // namespace