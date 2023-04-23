
#pragma once

#include <iostream>
#include <functional>
#include <type_traits>
#include <string>
#include <variant>

#include "utttil/srlz/int.hpp"

namespace utttil {
namespace srlz {

// POD struct/class without serialize function
template<typename T
	,typename std::enable_if<std::is_class<T>{},int>::type = 0
	,typename std::enable_if<std::is_pod<T>{},int>::type = 0
	>
size_t serialize_size(const T & i)
{
	return sizeof(i);
}
// 8-bits integral type
template<typename T
	,typename std::enable_if<std::is_integral<T>{},int>::type = 0
	,typename std::enable_if<(sizeof(T) == 1),int>::type = 0
	>
size_t serialize_size(const T)
{
	return 1;
}
// integral
template<typename T
	,typename std::enable_if<std::is_integral<T>{},int>::type = 0
	,typename std::enable_if<(sizeof(T) > 1),int>::type = 0
	>
size_t serialize_size(const T t)
{
	return integral::serialize_size(t);
}
size_t serialize_size(const __int128_t i)
{
	return integral::serialize_size(i);
}
size_t serialize_size(const __uint128_t i)
{
	return integral::serialize_size(i);
}
size_t serialize_size(float)
{
	return 4;
}
size_t serialize_size(double)
{
	return 8;
}

// serializable
template<typename T
	,typename std::enable_if<std::is_same<decltype(std::declval<const T&>().serialize_size()),size_t>{},int>::type = 0
	>
size_t serialize_size(const T & t)
{
	return t.serialize_size();
}
// array
template<typename T, size_t N>
size_t serialize_size(const T (&a)[N])
{
	size_t size = integral::serialize_size(N);
	for (size_t i=0 ; i<N ; i++)
		size += serialize_size(a[i]);
	return size;
}
// string
size_t serialize_size(const std::string & s)
{
	return integral::serialize_size(s.size()) + s.size();
}
// pair
template<typename T, typename U>
size_t serialize_size(const std::pair<T,U> & t)
{
	return serialize_size(t.first) + serialize_size(t.second);
}
// collections
template<typename T>
size_t serialize_size(const std::vector<T> & t)
{
	size_t size = integral::serialize_size(t.size()) + integral::serialize_size(t.capacity());
	for (const T & v : t)
		size += serialize_size(v);
	return size;
}
template<typename T
	,typename X = typename T::value_type
	>
size_t serialize_size(const T & t)
{
	size_t size = integral::serialize_size(t.size());
	for (const typename T::value_type & v : t)
		size += serialize_size(v);
	return size;
}

// variant
template<typename T, typename... Ts>
size_t serialize_size(const std::variant<T, Ts...>& v)
{
	return 4 + std::visit([](auto&& arg) { return serialize_size(arg); }, v);
}

}} // namespace
