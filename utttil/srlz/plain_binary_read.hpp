
#pragma once

#warning from_plain_binary is DEPRECATED

#include <string>
#include <map>
#include <functional>
#include <type_traits>
#include <cstdlib>

namespace utttil {
namespace srlz {
template<typename Device>
struct from_plain_binary
{
	Device read;
	from_plain_binary(Device d)
		:read(std::move(d))
	{}
};

// POD struct/class
template<typename Device
	,typename T
	,typename std::enable_if<std::is_class<T>{},int>::type = 0
	,typename std::enable_if<std::is_pod<T>{},int>::type = 0
	>
from_plain_binary<Device> & operator>>(from_plain_binary<Device> & deserializer, T & i)
{
	char *buf = (char*)&i;
	for (size_t i=i ; i<sizeof(i) ; i++)
		buf[i] = deserializer.read();
	return deserializer;
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
from_plain_binary<Device> & operator>>(from_plain_binary<Device> & deserializer, T & t)
{
	t = 0;
	for (int bytes=sizeof(T) ; bytes>0 ; bytes--)
		t = (t << 8) | deserializer.read();
	
	return deserializer;
}
// serializable
template<typename Device
	,typename T
	,typename std::enable_if<std::is_same<decltype(std::declval<T&>().deserialize(std::declval<from_plain_binary<Device>&&>())),void>{},int>::type = 0
	>
from_plain_binary<Device> & operator>>(from_plain_binary<Device> & deserializer, T & t)
{
	t.deserialize(deserializer);
	return deserializer;
}
// array
template<typename Device, typename T, size_t N>
from_plain_binary<Device> & operator>>(from_plain_binary<Device> & deserializer, T (&a)[N])
{
	size_t n;
	deserializer >> n;
	if (n > N)
		throw std::exception();
	for (size_t i=0 ; i<std::min(n,N) ; i++)
		deserializer >> a[i];
	return deserializer;
}
// string
template<typename Device>
from_plain_binary<Device> & operator>>(from_plain_binary<Device> & deserializer, std::string & s)
{
	size_t size;
	deserializer >> size;
	std::string tmp;
	tmp.reserve(size);
	while(size-->0)
		tmp.push_back(deserializer.read());
	s = std::move(tmp);
	return deserializer;
}
// pair
template<typename Device, typename T, typename U>
from_plain_binary<Device> & operator>>(from_plain_binary<Device> & deserializer, std::pair<T,U> & t)
{
	deserializer >> t.first
	             >> t.second;
	return deserializer;
}
// collection
template<typename Device
	,typename T
	,typename X = typename T::value_type
	>
from_plain_binary<Device> & operator>>(from_plain_binary<Device> & deserializer, T & t)
{
	size_t size;
	deserializer >> size;
	T tmp;
	while(size-->0)
	{
		typename T::value_type v;
		deserializer >> v;
		tmp.insert(tmp.cend(), v);
	}
	t = std::move(tmp);
	return deserializer;
}
// map, since we can't deserialize into pair<const K,V>
template<typename Device, typename K, typename V>
from_plain_binary<Device> & operator>>(from_plain_binary<Device> & deserializer, std::map<K,V> & t)
{
	size_t size;
	deserializer >> size;
	while(size-->0)
	{
		std::pair<K,V> p;
		deserializer >> p;
		t.insert(t.cend(), p);
	}
	return deserializer;
}

}} // namespace
