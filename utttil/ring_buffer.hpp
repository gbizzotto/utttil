
#pragma once

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
	//std::atomic_bool full_ = false;
	//std::atomic_bool empty_ = true;

	ring_buffer(size_t capacity)
		: data((T*) malloc(sizeof(T) * (capacity+1)))
		, capacity_(capacity+1)
	{}
	~ring_buffer()
	{
		while( ! empty())
			pop_front();
		free(data);
	}

		// TODO no %
	size_t inc(size_t v) const { return (v+1) % capacity_; }
	size_t dec(size_t v) const { return (v-1+capacity_) % capacity_; }

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
		if (empty())
			throw std::exception();
		return data[front_];
	}
	T & back()
	{
		if (empty())
			throw std::exception();
		return data[dec(back_)];
	}

	void pop_front()
	{
		if (empty())
			throw std::exception();
		data[front_].~T();
		front_ = inc(front_);
		//empty_ = front_ == back_;
		//full_ = false;
	}
	void pop_back()
	{
		if (empty())
			throw std::exception();
		size_t b = dec(back_);
		data[b].~T();
		back_ = b;
		//empty_ = front_ == back_;
		//full_ = false;
	}

	T & push_front(T && t)
	{
		if (full())
			throw std::exception();
		size_t f = dec(front_);
		new (&data[f]) T(std::forward<T>(t));
		T & result = data[f];
		front_ = f;
		//full_ = front_ == back_;
		//empty_ = false;
		return result;
	}
	T & push_back(T && t)
	{
		if (full())
			throw std::exception();
		new (&data[back_]) T(std::forward<T>(t));
		T & result = data[back_];
		back_ = inc(back_);
		//full_ = front_ == back_;
		//empty_ = false;
		return result;
	}

	T & push_front(const T & t)
	{
		if (full())
			throw std::exception();
		size_t f = dec(front_);
		new (&data[f]) T(t);
		T & result = data[f];
		front_ = f;
		//full_ = front_ == back_;
		//empty_ = false;
		return result;
	}
	T & push_back(const T & t)
	{
		if (full())
			throw std::exception();
		new (&data[back_]) T(t);
		T & result = data[back_];
		back_ = inc(back_);
		//full_ = front_ == back_;
		//empty_ = false;
		return result;
	}

	template<typename ...P>
	T & emplace_front(P... params)
	{
		if (full())
			throw std::exception();
		size_t f = dec(front_);
		new (&data[f]) T(params...);
		T & result = data[f];
		front_ = f;
		//full_ = front_ == back_;
		//empty_ = false;
		return result;
	}
	template<typename ...P>
	T & emplace_back(P... params)
	{
		if (full())
			throw std::exception();
		new (&data[back_]) T(params...);
		T & result = data[back_];
		back_ = inc(back_);
		//full_ = front_ == back_;
		//empty_ = false;
		return result;
	}
};

} // namespace
