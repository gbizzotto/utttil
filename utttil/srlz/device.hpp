
#pragma once

#include <memory>
#include <exception>
#include <iostream>

#include <utttil/ring_buffer.hpp>

namespace utttil {
namespace srlz {
namespace device {

struct stream_end_exception : std::exception {};

struct null_writer
{
	size_t size_ = 0;
	void operator()(char) { size_++; }
	inline void flush() {}
	inline size_t size() const { return size_; }
};

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
	void reset(volatile char * c_)
	{
		c = c_;
	}
};
struct mmap_writer
{
	char * c;
	void * begin_ptr;
	inline mmap_writer(void * c)
		: c((char*)c)
		, begin_ptr(c)
	{}
	inline void operator()(char v)
	{
		*c++ = v;
	}
	inline void flush() {}
	inline size_t size() const { return std::distance((char*)begin_ptr, c); }
};
struct ptr_reader
{
	char * c;
	char * begin_ptr;
	char * end_ptr;
	inline ptr_reader(char * c, size_t max_size)
		: c(c)
		, begin_ptr(c)
		, end_ptr(c + max_size)
	{}
	inline char operator()()
	{
		if (c < end_ptr)
			return *c++;
		else
			throw stream_end_exception();
	}
	inline size_t size() const { return std::distance(begin_ptr, c); }
	inline void skip(size_t count) { c += count; }
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

template<typename RB>
struct ring_buffer_reader
{
	using T = typename RB::value_type;
	using value_type = T;

	std::tuple<T*, size_t> stretch_1;
	std::tuple<T*, size_t> stretch_2;
	size_t read_count = 0;
	size_t max;

	inline ring_buffer_reader(RB & rb, size_t max_)
		: stretch_1(rb.front_stretch())
		, stretch_2(rb.front_stretch_2())
		, max(max_)
	{}
	inline T & operator()()
	{
		if (read_count >= max)
			throw stream_end_exception();
		if (std::get<1>(stretch_1) > 0)
		{
			T & result = *std::get<0>(stretch_1);
			++std::get<0>(stretch_1);
			--std::get<1>(stretch_1);
			++read_count;
			return result;
		}
		else if (std::get<1>(stretch_2) > 0)
		{
			T & result = *std::get<0>(stretch_2);
			++std::get<0>(stretch_2);
			--std::get<1>(stretch_2);
			++read_count;
			return result;
		}
		else
			throw stream_end_exception();
	}
	inline size_t size() const { return read_count; }
};

template<typename T>
struct ring_buffer_writer
{
	std::tuple<T*, size_t> stretch_1;
	std::tuple<T*, size_t> stretch_2;
	size_t written_count = 0;
	size_t max;

	inline ring_buffer_writer(std::tuple<T*, size_t> & stretch_1_
	                         ,std::tuple<T*, size_t> & stretch_2_
	                         ,size_t max_)
		: stretch_1(stretch_1_)
		, stretch_2(stretch_2_)
		, max(max_)
	{}
	inline void operator()(const T & t)
	{
		if (written_count >= max)
			throw stream_end_exception();
		if (std::get<1>(stretch_1) > 0)
		{
			*std::get<0>(stretch_1) = t;
			++std::get<0>(stretch_1);
			--std::get<1>(stretch_1);
			++written_count;
		}
		else if (std::get<1>(stretch_2) > 0)
		{
			*std::get<0>(stretch_2) = t;
			++std::get<0>(stretch_2);
			--std::get<1>(stretch_2);
			++written_count;
		}
		else
			throw stream_end_exception();
	}
	inline void flush() {};
	inline size_t size() const { return written_count; }
};

}}} // namespace
