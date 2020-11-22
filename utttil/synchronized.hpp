
#pragma once

#include <mutex>
#include <condition_variable>

namespace utttil {

template<typename T, class Mutex>
class synchronized_proxy
{
	template<typename X, class Y>
	friend class synchronized;

private:
	synchronized_proxy() = delete;
	synchronized_proxy(const synchronized_proxy &) = delete;
	synchronized_proxy & operator=(const synchronized_proxy &) = delete;
	synchronized_proxy & operator=(synchronized_proxy &&) = delete;
	
	synchronized_proxy(Mutex & m, T & obj)
		:lock(m)
		,t(&obj)
	{}
	synchronized_proxy(Mutex & m, T & obj, int)
		:lock(m, std::try_to_lock)
		,t((lock)?&obj:nullptr)
	{}

public:
	synchronized_proxy(synchronized_proxy && other)
		:lock(*other.lock.mutex(), std::adopt_lock)
		,t(std::move(other.t))
	{
		other.t = nullptr;
	}

	bool operator!() { return !lock; }

	const T * operator->() const { return t; }
	      T * operator->()       { return t; }

	const T & operator*() const { return *t; }
	      T & operator*()       { return *t; }

private:
	std::unique_lock<Mutex> lock;
	T * t;
};


template<typename T, class Mutex=std::mutex>
class synchronized
{
public:
	// Convenience typefed for subclasses to use
	typedef T SynchronizedObject;
	using value_type = T;
	
	synchronized() = default;
	synchronized(const T & t)
		: t(t)
	{}
	synchronized(T && t)
		:t(std::move(t))
	{}
	
	synchronized(const synchronized_proxy<T, Mutex> & sp)
		:t(*sp.t)
	{}
	synchronized(synchronized_proxy<T, Mutex> && sp)
		:t(std::move(*sp.t))
	{}

	synchronized_proxy<T,Mutex> lock() {
		return synchronized_proxy<T,Mutex>(mutex, t);
	}
	synchronized_proxy<T,Mutex> try_lock() {
		return synchronized_proxy<T,Mutex>(mutex, t, 0);
	}

	template<typename Check>
	synchronized_proxy<T,Mutex> wait_for_notification(Check check)
	{
		auto s = synchronized_proxy<T,Mutex>(mutex, t);
		if (check(t))
			return s;
		cv.wait(s.lock, [&](){ return check(t); });
		return s;
	}

	void notify_one()
	{
		cv.notify_one();
	}
	void notify_all()
	{
		cv.notify_all();
	}

protected:
	T t;
	Mutex mutex;
	std::condition_variable cv;
};

} // namespace
