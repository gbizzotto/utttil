
#pragma once

#include <string>
#include <string_view>
#include <cstring>
#include <exception>
#include <algorithm>
#include <cassert>

namespace utttil {

struct stringz_default_tag {};

template<size_t C, typename Tag=stringz_default_tag>
struct fixed_stringz
{
	static_assert(C > 0);
	static const size_t Capacity = C;
	char data_[Capacity];

	fixed_stringz()
	{
		data_[0] = 0;
	}
	fixed_stringz(const char * str)
	{
		*this = str;
	}

	const char * data() const { return data_; }
	      char * data()       { return data_; }
	size_t size() const
	{
		return std::distance(&data_[0], std::find(&data_[0], &data_[Capacity], 0));
	}
	void resize(size_t s)
	{
		assert(s <= Capacity);
		if (s < Capacity)
			data_[s] = 0;
	}

	fixed_stringz & operator=(const char * str)
	{
		strncpy(data_, str, Capacity);
		return *this;
	}
	fixed_stringz & operator=(const std::string & str)
	{
		return *this = str.c_str();
	}
	// template<size_t C2, typename Tag2>
	// void append(const fixed_stringz<C2,Tag2> & other)
	// {
	// 	const char *src=other.data_;
	// 	const char *end=&other.data_[other.size_];
	// 	for ( char *dst = &data_[size_]
	// 	    ; size_ < Capacity && src!=end
	// 		; dst++,src++,size_++ )
	// 	{
	// 		*dst = *src;
	// 	}
	// }

	template<typename Serializer>
	void serialize(Serializer && s) const
	{
		for (size_t i=0 ; i<Capacity && data_[i] != 0 ; i++)
			s.write(data_[i]);
	}
	template<typename Deserializer>
	void deserialize(Deserializer && s)
	{
		for (size_t i=0 ; i<Capacity != 0 ; i++) {
			data_[i] = s.read();
			if (data_[i] == 0)
				break;
		}
	}

	// for abseil flat_has_map/set
	template <typename H>
	friend H AbslHashValue(H h, const fixed_stringz & s)
	{
		return H::combine(std::move(h), std::string_view(s.data_, s.size()));
	}

	template<typename Rand>
	void randomize(Rand & rand)
	{
		for (size_t i=0 ; i<Capacity != 0 ; i++)
			data_[i] = 32 + (rand()%(128-32));
	}
};

template<size_t C, typename T>
std::ostream & operator<<(std::ostream & out, const fixed_stringz<C,T> & fs)
{
	return out << std::string_view(fs.data_, fs.size());
}

template<size_t C, typename T>
bool operator<(const fixed_stringz<C,T> & left, const fixed_stringz<C,T> & right)
{
	return std::strncmp(left.data_, right.data_, C) < 0;
}

template<size_t C, typename T>
bool operator==(const fixed_stringz<C,T> & left, const fixed_stringz<C,T> & right)
{
	return std::strncmp(left.data_, right.data_, C) == 0;
}
template<size_t C, typename T>
bool operator!=(const fixed_stringz<C,T> & left, const fixed_stringz<C,T> & right)
{
	return std::strncmp(left.data_, right.data_, C) != 0;
}

} // namespace
