
// This seemingly redundant shared_ptr is designed to have a size of 8 bytes only, instead of 16 like std::shared_ptr
// This is needed in iou, where the user data is only 8 bytes

#pragma once

#include <cstddef> // size_t
#include <utility> // swap
#include <type_traits>

namespace utttil {

template<typename T>
struct small_shared_ptr_counter_base
{
	size_t count = 1;
	size_t weak_count = 0;
	void inc() { count += (count != 0); }
	virtual void dec() = 0;
	virtual T * get() = 0;
	virtual const T * get() const = 0;
};

template<typename T>
struct small_shared_ptr_counter : small_shared_ptr_counter_base<T>
{
	T * t = nullptr;
	small_shared_ptr_counter(T * t_)
		: t(t_)
	{}
	void dec() override
	{
		this->count -= (this->count != 0);
		if (this->count == 0)
		{
			delete t;
			t = nullptr;
		}
	}
	      T * get()       override { return t; }
	const T * get() const override { return t; }
};

template<typename T>
struct small_shared_ptr_counter_in_place : small_shared_ptr_counter_base<T>
{
	T t;
	template<typename...Args>
	small_shared_ptr_counter_in_place(Args...args)
		: t(args...)
	{}
	void dec() override
	{
		this->count -= (this->count != 0);
		if (this->count == 0)
			t.~T();
	}
	      T * get()       override { return this->count == 0 ? nullptr : &t; }
	const T * get() const override { return this->count == 0 ? nullptr : &t; }
};

template<typename T>
struct enable_small_shared_from_this;

template<typename T>
struct small_shared_ptr
{
	small_shared_ptr_counter_base<T> * counter;

	small_shared_ptr() noexcept
		: counter(nullptr)
	{}	
	small_shared_ptr(std::nullptr_t) noexcept
		: counter(nullptr)
	{}
	template<typename U, std::enable_if_t<!std::is_base_of<enable_small_shared_from_this<T>,U>::value, bool> = true>
	small_shared_ptr(U * && t)
	{
		if (t) {
			counter = new small_shared_ptr_counter<T>(t);
			t = nullptr;
		} else
			counter = nullptr;
	}
	template<typename U, std::enable_if_t<std::is_base_of<enable_small_shared_from_this<T>,U>::value, bool> = true>
	small_shared_ptr(enable_small_shared_from_this<U> * && t)
	{
		if (t)
		{
			counter = new small_shared_ptr_counter<T>((T*)t);
			counter->get()->enable_small_shared_from_this_counter = counter;
			t = nullptr;
		}
		else
			counter = nullptr;
	}
	small_shared_ptr(const small_shared_ptr<T> & other)
	{
		if (this == &other) {
			counter = nullptr;
			return;
		}
		counter = other.counter;
		increase();
	}
	template<typename U, std::enable_if_t<std::is_base_of<T,U>::value, bool> = true>
	small_shared_ptr(const small_shared_ptr<U> & other)
	{
		if ((ptrdiff_t)this == (ptrdiff_t)&other) {
			counter = nullptr;
			return;
		}
		counter = (decltype(counter))other.counter;
		increase();
	}
	template<typename U, std::enable_if_t<std::is_base_of<T,U>::value, bool> = true>
	small_shared_ptr(small_shared_ptr<U> && other)
	{
		counter = nullptr;
		if ((ptrdiff_t)this != (ptrdiff_t)&other)
		{
			decltype(other.counter) tmp = (decltype(other.counter))counter;
			counter = (decltype(counter))other.counter;
			other.counter = tmp;
		}
	}
	small_shared_ptr & operator=(const small_shared_ptr<T> & other)
	{
		if ((ptrdiff_t)this == (ptrdiff_t)&other)
			return *this;
		decrease();
		counter = (decltype(counter))other.counter;
		increase();
		return *this;
	}
	template<typename U, std::enable_if_t<std::is_base_of<T,U>::value, bool> = true>
	small_shared_ptr & operator=(const small_shared_ptr<U> & other)
	{
		if ((ptrdiff_t)this == (ptrdiff_t)&other)
			return *this;
		decrease();
		counter = (decltype(counter))other.counter;
		increase();
		return *this;
	}
	small_shared_ptr & operator=(small_shared_ptr && other)
	{
		if ((ptrdiff_t)this == (ptrdiff_t)&other)
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
		counter->inc();
	}
	void decrease()
	{
		if ( ! counter)
			return;
		counter->dec();
		if (counter->count == 0 && counter->weak_count == 0)
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
	void reset(T * t)
	{
		decrease();
		if (t)
			counter = new small_shared_ptr_counter<T>(t);
		else
			counter = nullptr;
	}

	size_t use_count() const { return counter == nullptr ? 0 : counter->count; }

	      T * get()       { return counter == nullptr ? nullptr : counter->get(); }
	const T * get() const { return counter == nullptr ? nullptr : counter->get(); }
	      T * operator->()       { return counter == nullptr ? nullptr : counter->get(); }
	const T * operator->() const { return counter == nullptr ? nullptr : counter->get(); }
	      T & operator*()       { return *(counter->get()); }
	const T & operator*() const { return *(counter->get()); }

	operator bool() const
	{
		return counter != nullptr && counter->count != 0;
	}

	template<typename U>
	void persist_in(U & u)
	{
		using ThisType = small_shared_ptr<T>;
		new (&u) ThisType (*this);
	}
	template<typename U>
	static small_shared_ptr<T> copy_persisted(U & u)
	{
		if (u == 0)
			return nullptr;
		using ThisType = small_shared_ptr<T>;
		ThisType & b = *reinterpret_cast<typename std::add_pointer<ThisType>::type>(&u);
		return b;
	}
	template<typename U>
	static void destroy_persisted(U & u)
	{
		if (u == 0)
			return;
		using ThisType = small_shared_ptr<T>;
		ThisType & b = *reinterpret_cast<typename std::add_pointer<ThisType>::type>(&u);
		b.~ThisType();
		u = 0;
	}
};

template<typename T>
struct small_weak_ptr
{
	small_shared_ptr_counter_base<T> * counter;

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
		counter = nullptr;
		if (this != &other)
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
		counter->weak_count += (counter != nullptr);
	}
	void decrease()
	{
		if ( ! counter)
			return;
		counter->weak_count -= (counter->weak_count != 0);
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
	small_shared_ptr<T> result;
	result.counter = new small_shared_ptr_counter_in_place<T>(args...);
	return result;
}

template<typename T, typename...Args, std::enable_if_t<std::is_base_of<enable_small_shared_from_this<T>,T>::value, bool> = true>
small_shared_ptr<T> make_small_shared(Args...args)
{
	small_shared_ptr<T> result;
	result.counter = new small_shared_ptr_counter_in_place<T>(args...);
	result.counter->get()->enable_small_shared_from_this_counter = result.counter;
	return result;
}

template<typename T>
struct enable_small_shared_from_this
{
	small_shared_ptr_counter_base<T> * enable_small_shared_from_this_counter;

	small_shared_ptr<T> small_shared_from_this()
	{
		utttil::small_shared_ptr<T> result;
		result.counter = enable_small_shared_from_this_counter;
		result.increase();
		return result;
	}
};

} // namespace
