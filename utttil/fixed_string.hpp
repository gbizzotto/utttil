
#pragma once

#include <string>
#include <string_view>
#include <cstring>
#include <exception>

namespace utttil {

struct fixed_string_default_tag {};

template<size_t C, typename Tag=fixed_string_default_tag>
struct fixed_string
{
	static const size_t Capacity = C;
	size_t size_;
	char data_[Capacity];

	fixed_string()
		: size_(0ull)
	{}
	fixed_string(const char * str)
	{
		*this = str;
	}

	const char * data() const { return data_; }
	      char * data()       { return data_; }
	size_t size() const { return size_; }
	size_t size()       { return size_; }
	void resize(size_t s) { size_ = s; }

	fixed_string & operator=(const char * str)
	{
		size_ = 0;
		const char *src = str;
		for ( char *dst = data_
		    ; *src != 0 && size_ < Capacity
			; src++, dst++ )
		{
			*dst = *src;
			size_++;
		}
		return *this;
	}
	fixed_string & operator=(const std::string & str)
	{
		return *this = str.c_str();
	}
	template<size_t C2, typename Tag2>
	void append(const fixed_string<C2,Tag2> & other)
	{
		const char *src=other.data_;
		const char *end=&other.data_[other.size_];
		for ( char *dst = &data_[size_]
		    ; size_ < Capacity && src!=end
			; dst++,src++,size_++ )
		{
			*dst = *src;
		}
	}

	template<typename Serializer>
	void serialize(Serializer && s) const
	{
		s << size_;
		const char * src = data_;
		for (size_t i=0 ; i<size_ ; i++)
			s.write(*(src++));
	}
	template<typename Deserializer>
	void deserialize(Deserializer && s)
	{
		s >> size_;
		if (size_ > Capacity)
			throw std::overflow_error("fixed_string size_ over capacity");
		char * dst = data_;
		for (size_t i=0 ; i<size_ ; i++)
			*(dst++) = s.read();
	}

	// for abseil flat_has_map/set
	template <typename H>
	friend H AbslHashValue(H h, const fixed_string & s)
	{
		return H::combine(std::move(h), std::string_view(s.data_, s.size_));
	}

	template<typename Rand>
	void randomize(Rand & rand)
	{
		size_ = 0;
		for ( char *dst = data_
		    ; size_ < Capacity
			; dst++,size_++ )
		{
			*dst = 32 + (rand()%(256-32));
		}
	}
};

template<size_t C, typename T>
std::ostream & operator<<(std::ostream & out, const fixed_string<C,T> & fs)
{
	return out << std::string_view(fs.data_, fs.size_);
}

template<size_t C, typename T>
bool operator<(const fixed_string<C,T> & left, const fixed_string<C,T> & right)
{
	return std::strncmp(left.data_, right.data_, std::min(left.size_, right.size_)) < 0
		|| left.size_ < right.size_;
}

template<size_t C, typename T>
bool operator==(const fixed_string<C,T> & left, const fixed_string<C,T> & right)
{
	return left.size_ == right.size_
		&& std::strncmp(left.data_, right.data_, left.size_) == 0;
}
template<size_t C, typename T>
bool operator!=(const fixed_string<C,T> & left, const fixed_string<C,T> & right)
{
	return left.size_ != right.size_
		|| std::strncmp(left.data_, right.data_, left.size_) != 0;
}

} // namespace
