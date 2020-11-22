
#pragma once

#include <string>
#include <type_traits>

#include "utttil/string_list.hpp"

namespace utttil {
namespace srlz {
template<typename Device>
struct to_json
{
	Device write;
	to_json(Device d)
		:write(std::move(d))
	{}
};

// string
template<typename Device>
to_json<Device> & operator<<(to_json<Device> & serializer, std::string s)
{
	serializer.write("\"");
	serializer.write(s);
	serializer.write("\"");
	return serializer;
}
// integral (to_string exists)
//template<typename B, typename T, typename = decltype( std::to_string(std::declval<T>()) )>
template<typename Device, typename T, typename std::enable_if<std::is_integral<T>{},int>::type = 0>
to_json<Device> & operator<<(to_json<Device> & serializer, T t)
{
	serializer.write(std::to_string(t));
	return serializer;
}
// serializable
template<typename Device
	,typename T
	,typename std::enable_if<std::is_same<decltype(std::declval<const T&>().serialize(std::declval<to_json<Device>&&>())),void>{},int>::type = 0
	>
to_json<Device> & operator<<(to_json<Device> & serializer, const T & t)
{
	utttil::string_list names;
	t.serialize_names(device::stream_to_lambda([&](std::string v)
		{
			names.push_back(std::move(v));
		}));

	bool comma = false;
	
	serializer.write("{");
	t.serialize(device::stream_to_lambda([&](auto && v)
		{
			if (comma)
				serializer.write(", ");
			comma = true;
			if ( ! names.empty()) {
				serializer << names.front();
				names.pop_front();
			} else {
				serializer.write("\"\"");
			}
			serializer.write(": ");
			serializer << v;
		}));
	serializer.write("}");

	return serializer;
}
// pair
template<typename Device, typename T, typename U>
to_json<Device> & operator<<(to_json<Device> & serializer, const std::pair<T,U> & t)
{
	serializer.write("[");
	serializer << t.first;
	serializer.write(",");
	serializer << t.second;
	serializer.write("]");
	
	return serializer;
}
// collection
template<typename Device
	,typename T
	,typename X = typename T::value_type
	>
to_json<Device> & operator<<(to_json<Device> & serializer, const T & t)
{
	serializer.write("[");
	bool comma = false;
	for (const auto & v : t)
	{
		if (comma)
			serializer.write(",");
		serializer << v;
		comma = true;
	}
	serializer.write("]");

	return serializer;
}

}} // namespace
