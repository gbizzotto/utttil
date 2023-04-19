
#pragma once

#include <ctime>
#include <random>
#include <string>
#include <variant>
#include <limits>

namespace utttil {


struct random_generator
{
	std::mt19937 gen;
	std::uniform_int_distribution<> distrib;

	random_generator(int seed)
		: gen(seed)
		, distrib(std::numeric_limits<int>::lowest(), std::numeric_limits<int>::max())
	{}

	template<typename T>
	T next()
	{
		T t;
		t.randomize(*this);
		return t;
	}

	template<size_t i, typename V>
	void fill_variant(int, V&)
	{}
	template<size_t i, typename V, typename U, typename...T>
	void fill_variant(int idx, V & v)
	{
		if (i == idx)
			v.template emplace<i>(next<U>());
		else 
			fill_variant<i+1, V, T...>(idx, v);
	}

	template<typename...T>
	std::variant<T...> next_variant()
	{
		std::variant<T...> result;
		auto idx = next<int>() % sizeof...(T);

		fill_variant<0, std::variant<T...>, T...>(idx, result);

		return result;
	}
	template<typename T>
	int ok() { return 123; }
};

template<>
int random_generator::next<int>()
{
	return distrib(gen);
}
template<>
unsigned int random_generator::next<unsigned int>()
{
	return (unsigned int) distrib(gen);
}
template<>
float random_generator::next<float>()
{
	int i;
	do {
		i = distrib(gen);
	} while (std::isnan(*reinterpret_cast<float*>(&i)));
	return *reinterpret_cast<float*>(&i);
}
template<>
std::string random_generator::next<std::string>()
{
	std::string result;
	int size = next<unsigned int>()%1024;
	result.reserve(size);
	while (size-->0)
		result.push_back(32 + (next<int>()%(256-32)));
	return result;
}


} // namespace
