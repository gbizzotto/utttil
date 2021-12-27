
// This seemingly redundant shared_ptr is designed to have a size of 8 bytes only, instead of 16 like std::shared_ptr
// This is needed in iou, where the user data is only 8 bytes

#pragma once

#include <cstddef> // size_t
#include <utility> // swap

namespace utttil {

template<typename T>
struct small_shared_ptr_counter
{
	size_t count = 0;
	size_t weak_count = 0;
	T * t = nullptr;
};

template<typename T>
struct enable_small_shared_from_this;

template<typename T>
struct small_shared_ptr
{
	small_shared_ptr_counter<T> * counter;

	small_shared_ptr() noexcept
		: counter(nullptr)
	{}	
	small_shared_ptr(std::nullptr_t) noexcept
		: counter(nullptr)
	{}	
	small_shared_ptr(T * && t)
	{
		if (t) {
			counter = new small_shared_ptr_counter<T>{1, 0, t};
			t = nullptr;
		} else
			counter = nullptr;
	}
	small_shared_ptr(enable_small_shared_from_this<T> * && t)
	{
		if (t) {
			counter = new small_shared_ptr_counter<T>{1, 0, t};
			t->enable_small_shared_from_this_counter = counter;
			t = nullptr;
		} else
			counter = nullptr;
	} 
	small_shared_ptr(small_shared_ptr & other)
	{
		if (this == &other)
			return;
		counter = other.counter;
		increase();
	}
	small_shared_ptr(small_shared_ptr && other)
	{
		if (this == &other)
			counter = nullptr;
		else
			std::swap(counter, other.counter);
	}
	small_shared_ptr & operator=(const small_shared_ptr & other)
	{
		if (this == &other)
			return *this;
		decrease();
		counter = other.counter;
		increase();
		return *this;
	}
	small_shared_ptr & operator=(small_shared_ptr && other)
	{
		if (this == &other)
			return *this;
		decrease();
		counter = other.counter;
		other.counter = nullptr;
		return *this;
	}
	void swap(small_shared_ptr & other)
	{
		std::swap(counter, other.counter);
	}
	~small_shared_ptr()
	{
		decrease();
	}

	void increase()
	{
		if ( ! counter)
			return;
		if (counter->count == 0)
			return;
		++counter->count;
	}
	void decrease()
	{
		if ( ! counter)
			return;
		if (counter->count == 0)
			return;
		--counter->count;
		if (counter->count == 0)
		{
			delete counter->t;
			counter->t = nullptr;
			if (counter->weak_count == 0)
			{
				delete counter;
				counter = nullptr;
			}
		}
	}

	void reset()
	{
		decrease();
		counter = nullptr;
	}
	void reset(T * t)
	{
		decrease();
		if (t)
			counter = new small_shared_ptr_counter<T>{1, 0, t};
		else
			counter = nullptr;
	}

	size_t use_count() const { return counter == nullptr ? 0 : counter->count; }

	      T * get()       { return counter == nullptr ? nullptr : counter->t; }
	const T * get() const { return counter == nullptr ? nullptr : counter->t; }
	      T * operator->()       { return counter == nullptr ? nullptr : counter->t; }
	const T * operator->() const { return counter == nullptr ? nullptr : counter->t; }
	      T & operator*()       { return *(counter->t); }
	const T & operator*() const { return *(counter->t); }

	operator bool() const
	{
		return counter != nullptr && counter->count != 0;
	}
};

template<typename T>
struct small_weak_ptr
{
	small_shared_ptr_counter<T> * counter;

	small_weak_ptr() noexcept
		: counter(nullptr)
	{}	
	small_weak_ptr(std::nullptr_t) noexcept
		: counter(nullptr)
	{}	
	small_weak_ptr(small_weak_ptr & other)
	{
		if (this == &other)
			return;
		counter = other.counter;
		increase();
	}
	small_weak_ptr(small_weak_ptr && other)
	{
		if (this == &other)
			counter = nullptr;
		else
			std::swap(counter, other.counter);
	}
	small_weak_ptr(small_shared_ptr<T> & shared)
	{
		counter = shared.counter;
		increase();
	}
	small_weak_ptr & operator=(const small_weak_ptr & other)
	{
		if (this == &other)
			return *this;
		decrease();
		counter = other.counter;
		increase();
		return *this;
	}
	small_weak_ptr & operator=(small_weak_ptr && other)
	{
		if (this == &other)
			return *this;
		decrease();
		counter = other.counter;
		other.counter = nullptr;
		return *this;
	}
	small_weak_ptr & operator=(small_shared_ptr<T> & other)
	{
		decrease();
		counter = other.counter;
		increase();
		return *this;
	}
	void swap(small_weak_ptr & other)
	{
		std::swap(counter, other.counter);
	}
	~small_weak_ptr()
	{
		decrease();
	}

	void increase()
	{
		if ( ! counter)
			return;
		++counter->weak_count;
	}
	void decrease()
	{
		if ( ! counter)
			return;
		if (counter->weak_count == 0)
			return;
		--counter->weak_count;
		if (counter->weak_count == 0 && counter->count == 0)
		{
			delete counter;
			counter = nullptr;
		}
	}

	void reset()
	{
		decrease();
		counter = nullptr;
	}

	utttil::small_shared_ptr<T> lock()
	{
		utttil::small_shared_ptr<T> result;
		result.counter = counter;
		if (use_count() > 0)
			result.increase();
		return result;
	}

	size_t use_count() const { return counter == nullptr ? 0 : counter->count; }
	bool expired() const { return use_count == 0; }
};

template<typename T, typename...Args, std::enable_if_t<!std::is_base_of<enable_small_shared_from_this<T>,T>::value, bool> = true>
small_shared_ptr<T> make_small_shared(Args...args)
{
	return small_shared_ptr(new T(args...));
}

template<typename T, typename...Args, std::enable_if_t<std::is_base_of<enable_small_shared_from_this<T>,T>::value, bool> = true>
small_shared_ptr<T> make_small_shared(Args...args)
{
	auto result = small_shared_ptr(new T(args...));
	result.counter->t->enable_small_shared_from_this_counter = result.counter;
	return result;
}

template<typename T>
struct enable_small_shared_from_this
{
	small_shared_ptr_counter<T> * enable_small_shared_from_this_counter;

	small_shared_ptr<T> small_shared_from_this()
	{
		utttil::small_shared_ptr<T> result;
		result.counter = enable_small_shared_from_this_counter;
		result.increase();
		return result;
	}
};

} // namespace
