
#pragma once

#include <iostream>
#include <functional>
#include <type_traits>
#include <string>

#include "utttil/srlz/int.hpp"

namespace utttil {
namespace srlz {
	
template<typename Device>
struct to_binary
{
	Device write;
	to_binary(Device d)
		:write(std::move(d))
	{}
};

// POD struct/class
template<typename Device
	,typename T
	,typename std::enable_if<std::is_class<T>{},int>::type = 0
	,typename std::enable_if<std::is_pod<T>{},int>::type = 0
	>
to_binary<Device> & operator<<(to_binary<Device> & serializer, T i)
{
	char *buf = (char*)&i;
	for (size_t j=0 ; j<sizeof(i) ; j++)
		serializer.write(buf[j]);
	return serializer;
}
// 8-bits integral type
template<typename Device
	,typename T
	,typename std::enable_if<std::is_integral<T>{},int>::type = 0
	,typename std::enable_if<(sizeof(T) == 1),int>::type = 0
	>
to_binary<Device> & operator<<(to_binary<Device> & serializer, T t)
{
	serializer.write(t);
	return serializer;
}
// integral
template<typename Device
	,typename T
	,typename std::enable_if<std::is_integral<T>{},int>::type = 0
	,typename std::enable_if<(sizeof(T) > 1),int>::type = 0
	>
to_binary<Device> & operator<<(to_binary<Device> & serializer, T t)
{
	char buf[50]; // int128 is 40 chars long, plus dot, sign and ending zero
	size_t s = integral::serialize(t, buf);
	for (size_t i=0 ; i<s ; i++)
		serializer.write(buf[i]);
	return serializer;
}
template<typename Device>
to_binary<Device> & operator<<(to_binary<Device> & serializer, __int128_t t)
{
	char buf[50]; // int128 is 40 chars long, plus dot, sign and ending zero
	size_t s = integral::serialize(t, buf);
	for (size_t i=0 ; i<s ; i++)
		serializer.write(buf[i]);
	return serializer;
}
template<typename Device>
to_binary<Device> & operator<<(to_binary<Device> & serializer, __uint128_t t)
{
	char buf[50]; // int128 is 40 chars long, plus dot, sign and ending zero
	size_t s = integral::serialize(t, buf);
	for (size_t i=0 ; i<s ; i++)
		serializer.write(buf[i]);
	return serializer;
}

// serializable
template<typename Device
	,typename T
	,typename std::enable_if<std::is_same<decltype(std::declval<const T&>().serialize(std::declval<to_binary<Device>&&>())),void>{},int>::type = 0
	>
to_binary<Device> & operator<<(to_binary<Device> & serializer, const T & t)
{
	t.serialize(serializer);
	return serializer;
}
// array
template<typename Device, typename T, size_t N>
to_binary<Device> & operator<<(to_binary<Device> & serializer, const T (&a)[N])
{
	serializer << N;
	for (size_t i=0 ; i<N ; i++)
		serializer << a[i];
	return serializer;
}
// string
template<typename Device>
to_binary<Device> & operator<<(to_binary<Device> & serializer, std::string s)
{
	serializer << s.size();
	for (const char c : s)
		serializer.write(c);
	return serializer;
}
// pair
template<typename Device, typename T, typename U>
to_binary<Device> & operator<<(to_binary<Device> & serializer, const std::pair<T,U> & t)
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
to_binary<Device> & operator<<(to_binary<Device> & serializer, const T & t)
{
	serializer << t.size();
	for (const typename T::value_type & v : t)
		serializer << v;
	return serializer;
}

// support for std::endl and other modifiers
template<typename Device>
to_binary<Device> & operator<<(to_binary<Device> & serializer, std::ostream& (*)(std::ostream&))
{
	serializer.write.flush();
	return serializer;
}

}} // namespace
