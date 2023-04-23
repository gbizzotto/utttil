
#pragma once

#include <string>
#include <map>
#include <functional>
#include <type_traits>
#include <cstdlib>
#include <cstdlib>
#include <variant>

#include "utttil/srlz/int.hpp"

namespace utttil {
namespace srlz {
template<typename Device>
struct from_binary
{
	Device read;
	from_binary(Device d)
		:read(std::move(d))
	{}
};

// POD struct/class
template<typename Device
	,typename T
	,typename std::enable_if<std::is_class<T>{},int>::type = 0
	,typename std::enable_if<std::is_pod<T>{},int>::type = 0
	>
from_binary<Device> & operator>>(from_binary<Device> & deserializer, T & i)
{
	char *buf = (char*)&i;
	for (size_t j=0 ; j<sizeof(i) ; j++)
		buf[j] = deserializer.read();
	return deserializer;
}
// 8-bits integral
template<typename Device
	,typename T
	,typename std::enable_if<std::is_integral<T>{},int>::type = 0
	,typename std::enable_if<(sizeof(T) == 1),int>::type = 0
	>
from_binary<Device> & operator>>(from_binary<Device> & deserializer, T & t)
{
	t = deserializer.read();
	return deserializer;
}
// integral
template<typename Device
	,typename T
	,typename std::enable_if<
		std::disjunction< // this is an OR
				std::is_integral<T>,
				std::is_same<T,__int128_t>,
				std::is_same<T,__uint128_t>
			>{},int
		>::type = 0
	,typename std::enable_if<(sizeof(T) > 1),int>::type = 0
	>
from_binary<Device> & operator>>(from_binary<Device> & deserializer, T & t)
{
	t = integral::deserialize<T>(deserializer.read);
	return deserializer;
}
// float
template<typename Device>
from_binary<Device> & operator>>(from_binary<Device> & deserializer, float & t)
{
	char * b = reinterpret_cast<char*>(&t);
	b[0] = deserializer.read();
	b[1] = deserializer.read();
	b[2] = deserializer.read();
	b[3] = deserializer.read();
	return deserializer;
}

// serializable
template<typename Device
	,typename T
	,typename std::enable_if<std::is_same<decltype(std::declval<T&>().deserialize(std::declval<from_binary<Device>&&>())),void>{},int>::type = 0
	>
from_binary<Device> & operator>>(from_binary<Device> & deserializer, T & t)
{
	t.deserialize(deserializer);
	return deserializer;
}
// array
template<typename Device, typename T, size_t N>
from_binary<Device> & operator>>(from_binary<Device> & deserializer, T (&a)[N])
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
from_binary<Device> & operator>>(from_binary<Device> & deserializer, std::string & s)
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
from_binary<Device> & operator>>(from_binary<Device> & deserializer, std::pair<T,U> & t)
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
from_binary<Device> & operator>>(from_binary<Device> & deserializer, T & t)
{
	size_t size;
	deserializer >> size;
	T tmp(std::move(t));
	while(size-->0)
	{
		typename T::value_type v;
		deserializer >> v;
		t.push_back(v);
	}
	//t = std::move(tmp);
	return deserializer;
}
// vector, so we can store capacity too
template<typename Device, typename T>
from_binary<Device> & operator>>(from_binary<Device> & deserializer, std::vector<T> & vec)
{
	size_t size;
	size_t capacity;
	deserializer >> size >> capacity;
	std::cout << "binary_read, size: " << size << ", capacity: " << capacity << std::endl;
	vec.clear();
	vec.reserve(capacity);
	while(size-->0)
	{
		T v;
		deserializer >> v;
		vec.push_back(std::move(v));
	}
	return deserializer;
}
// map, since we can't deserialize into pair<const K,V>
template<typename Device, typename K, typename V>
from_binary<Device> & operator>>(from_binary<Device> & deserializer, std::map<K,V> & t)
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

// variant
template<typename O, size_t i, typename V>
void fill_variant(O&, int, V&)
{}
template<typename O, size_t i, typename V, typename U, typename...T>
void fill_variant(O & o, int idx, V & v)
{
	if (i == idx)
	{
		U value;
		o >> value;
		v.template emplace<i>(value);
	}
	else 
		fill_variant<O, i+1, V, T...>(o, idx, v);
}
template<typename O, typename T, typename...Ts>
O & operator>>(O & os, std::variant<T,Ts...>& v)
{
	uint8_t idx;
	os >> idx;
 	fill_variant<O, 0, std::variant<T,Ts...>, T, Ts...>(os, idx, v);
    return os;
}

}} // namespace
