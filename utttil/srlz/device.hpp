
#pragma once

#include <memory>
#include <exception>
#include <iostream>

namespace utttil {
namespace srlz {
namespace device {

struct stream_end_exception : std::exception {};

template<typename Container>
struct back_pusher
{
	Container & container;
	back_pusher(Container & c)
		: container(c)
	{}
	template<typename T>
	void operator()(T && v)
	{
		container.push_back(v);
	}
	void flush() {}
	size_t size() const { return container.size(); }
};
template<typename Container>
struct back_inserter
{
	Container & container;
	back_inserter(Container & c)
		: container(c)
	{}
	template<typename T>
	void operator()(T && v)
	{
		container.insert(container.end(), v);
	}
	void flush() {}
	size_t size() const { return container.size(); }
};

struct mmap_reader
{
	volatile char * c;
	inline mmap_reader(volatile char * c)
		: c(c)
	{}
	inline char operator()()
	{
		return *c++;
	}
};
struct mmap_writer
{
	volatile char * c;
	volatile char * begin_ptr;
	inline mmap_writer(volatile char * c)
		: c(c)
		, begin_ptr(c)
	{}
	inline void operator()(char v)
	{
		*c++ = v;
	}
	inline void flush() {}
	inline size_t size() const { return std::distance(begin_ptr, c); }
};
struct ptr_reader
{
	char * c;
	inline ptr_reader(char * c)
		: c(c)
	{}
	inline char operator()()
	{
		return *c++;
	}
};
struct ptr_writer
{
	char * c;
	char * begin_ptr;
	ptr_writer(char * c)
		: c(c)
		, begin_ptr(c)
	{}
	void operator()(char v)
	{
		*c++ = v;
	}
	inline void flush() {}
	inline size_t size() const { return std::distance(begin_ptr, c); }
};
template<typename It>
struct iterator_reader
{
	It it;
	It end;
	iterator_reader(It i, It e)
		: it(i)
		, end(e)
	{}
	const auto & operator()()
	{
		if (it == end)
			throw stream_end_exception();
		return *it++;
	}

};
template<typename Container>
struct front_popper
{
	Container & c;
	front_popper(Container & c)
		: c(c)
	{}
	const typename Container::value_type operator()()
	{
		if (c.empty())
			throw stream_end_exception();
		typename Container::value_type v = std::move(c.front());
		c.pop_front();
		return v;
	}
};

template<typename F>
struct stream_to_lambda
{
	F f;
	stream_to_lambda(F && f) :f(f) {}
	void flush(){}
};
template<typename F, typename T>
stream_to_lambda<F> & operator<<(stream_to_lambda<F> & sf, T && t)
{
	sf.f(t);
	return sf;
}

struct istream_reader
{
	std::istream & stream;
	inline istream_reader(std::istream & s)
		: stream(s)
	{}
	inline char operator()()
	{
		if (stream.eof())
			throw stream_end_exception();
		return (char) stream.get();
	}
};
struct ostream_writer
{
	std::ostream & stream;
	size_t count;
	inline ostream_writer(std::ostream & s)
		: stream(s)
		, count(0)
	{}
	inline void operator()(char c)
	{
		stream.put(c);
		++count;
	}
	inline void flush() {};
	inline size_t size() const { return count; }
};

}}} // namespace
