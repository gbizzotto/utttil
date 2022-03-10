
#pragma once

#include <immintrin.h>
#include <memory>
#include <atomic>

namespace utttil {

template<typename T>
struct ring_buffer
{
	using value_type = T;

	T * data;
	const size_t capacity_;
	std::atomic_size_t front_ = 0;
	std::atomic_size_t back_ = 0;

	ring_buffer(size_t capacity)
		: data((T*) malloc(sizeof(T) * (capacity+1)))
		, capacity_(capacity+1)
	{}
	~ring_buffer()
	{
		clear();
		free(data);
	}

		// TODO no %
	size_t inc(size_t v) const { return (v+1) % capacity_; }
	size_t dec(size_t v) const { return (v-1+capacity_) % capacity_; }

	struct iterator
	{
		ring_buffer * rb;
		size_t pos;
		iterator & operator++() { pos = rb->inc(pos); return *this; }
		T & operator*() { return rb->data[pos]; }
		T * operator->() { return &rb->data[pos]; }
		bool operator==(const iterator & other) { return pos == other.pos; }
		bool operator!=(const iterator & other) { return pos != other.pos; }
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
		return capacity_-1;
	}
	size_t size() const
	{
		// TODO branchless
		if (front_ <= back_)
			return back_-front_;
		else
			return capacity_-front_ + back_;
	}
	bool empty() const
	{
		return front_ == back_;
	}
	bool full() const
	{
		return inc(back_) == front_;
	}

	T & front()
	{
		while (empty())
			_mm_pause();
		return data[front_];
	}
	T & back()
	{
		while (empty())
			_mm_pause();
		return data[dec(back_)];
	}
	const T & front() const
	{
		while (empty())
			_mm_pause();
		return data[front_];
	}
	const T & back() const
	{
		while (empty())
			_mm_pause();
		return data[dec(back_)];
	}

	void pop_front()
	{
		while (empty())
			_mm_pause();
		data[front_].~T();
		front_ = inc(front_);
	}
	void pop_back()
	{
		while (empty())
			_mm_pause();
		size_t b = dec(back_);
		data[b].~T();
		back_ = b;
	}

	T & push_front(T && t)
	{
		while (full())
			_mm_pause();
		size_t f = dec(front_);
		new (&data[f]) T(std::forward<T>(t));
		T & result = data[f];
		front_ = f;
		return result;
	}
	T & push_back(T && t)
	{
		while (full())
			_mm_pause();
		new (&data[back_]) T(std::forward<T>(t));
		T & result = data[back_];
		back_ = inc(back_);
		return result;
	}

	T & push_front(const T & t)
	{
		while (full())
			_mm_pause();
		size_t f = dec(front_);
		new (&data[f]) T(t);
		T & result = data[f];
		front_ = f;
		return result;
	}
	T & push_back(const T & t)
	{
		while (full())
			_mm_pause();
		new (&data[back_]) T(t);
		T & result = data[back_];
		back_ = inc(back_);
		return result;
	}

	template<typename ...P>
	T & emplace_front(P... params)
	{
		while (full())
			_mm_pause();
		size_t f = dec(front_);
		new (&data[f]) T(params...);
		T & result = data[f];
		front_ = f;
		return result;
	}
	template<typename ...P>
	T & emplace_back(P... params)
	{
		while (full())
			_mm_pause();
		new (&data[back_]) T(params...);
		T & result = data[back_];
		back_ = inc(back_);
		return result;
	}
};

template<typename Out, typename T>
Out & operator<<(Out & out, const utttil::ring_buffer<T> & rb)
{
	return out << "ring_buffer sizeof(T): " << sizeof(T) << ", capacity: " << rb.capacity() << ", front_: " << rb.front_ << ", back: " << rb.back_;
}

} // namespace