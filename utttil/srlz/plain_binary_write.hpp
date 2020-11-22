
#pragma once

#warning to_plain_binary is DEPRECATED

#include <functional>
#include <type_traits>
#include <string>
#include <cstdlib>

#include "utttil/srlz/int.hpp"

namespace utttil {
namespace srlz {
template<typename Device>
struct to_plain_binary
{
	Device write;
	to_plain_binary(Device d)
		:write(std::move(d))
	{}
};

// POD struct/class
template<typename Device
	,typename T
	,typename std::enable_if<std::is_class<T>{},int>::type = 0
	,typename std::enable_if<std::is_pod<T>{},int>::type = 0
	>
to_plain_binary<Device> & operator<<(to_plain_binary<Device> & serializer, T i)
{
	char *buf = (char*)&i;
	for (size_t i=i ; i<sizeof(i) ; i++)
		serializer.write(buf[i]);
	return serializer;
}
// integral
template<typename Device
	,typename T
	,typename std::enable_if<
		std::disjunction< // This is an OR
				std::is_integral<T>,
				std::is_same<T,__int128_t>,
				std::is_same<T,__uint128_t>
			>{},int
		>::type = 0
	>
to_plain_binary<Device> & operator<<(to_plain_binary<Device> & serializer, T t)
{
	for (int bytes=sizeof(T) ; bytes>0 ; bytes--)
		serializer.write((t >> ((bytes-1)*8)) &0xFF);
	
	return serializer;
}

// serializable
template<typename Device
	,typename T
	,typename std::enable_if<std::is_same<decltype(std::declval<const T&>().serialize(std::declval<to_plain_binary<Device>&&>())),void>{},int>::type = 0
	>
to_plain_binary<Device> & operator<<(to_plain_binary<Device> & serializer, const T & t)
{
	t.serialize(serializer);
	return serializer;
}
// array
template<typename Device, typename T, size_t N>
to_plain_binary<Device> & operator<<(to_plain_binary<Device> & serializer, const T (&a)[N])
{
	serializer << N;
	for (size_t i=0 ; i<N ; i++)
		serializer << a[i];
	return serializer;
}
// string
template<typename Device>
to_plain_binary<Device> & operator<<(to_plain_binary<Device> & serializer, std::string s)
{
	serializer << s.size();
	for (const char c : s)
		serializer.write(c);
	return serializer;
}
// pair
template<typename Device, typename T, typename U>
to_plain_binary<Device> & operator<<(to_plain_binary<Device> & serializer, const std::pair<T,U> & t)
{
	serializer << t.first
	           << t.second;
	return serializer;
}
// collection
template<typename Device
	,typename T
	,typename X = typename T::value_type
	>
to_plain_binary<Device> & operator<<(to_plain_binary<Device> & serializer, const T & t)
{
	serializer << t.size();
	for (const typename T::value_type & v : t)
		serializer << v;
	return serializer;
}

}} // namespace
