
#pragma once

#include <deque>
#include <string>
#include <utility>
#include <type_traits>

namespace utttil {

struct string_list : std::deque<std::string> {};

} // namespace

template<typename T, typename = decltype( std::to_string(std::declval<T>()) )>
utttil::string_list & operator<<(utttil::string_list & sl, T && s)
{
	sl.push_back(std::to_string(s));
	return sl;
}
inline utttil::string_list & operator<<(utttil::string_list & sl, std::string s)
{
	sl.push_back(std::move(s));
	return sl;
}