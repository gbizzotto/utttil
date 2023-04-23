
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

	auto operator()() { return distrib(gen); }

	template<typename T>
	T next()
	{
		T t;
		randomize(*this, t);
		return t;
	}
};

///////////////////////// ints
template<typename T
	,typename std::enable_if<std::is_integral<T>{},int>::type = 0
	,typename std::enable_if<(sizeof(T) <= 4),int>::type = 0
	>
void randomize(random_generator & r, T & t)
{
	t = r();
}

template<typename T
	,typename std::enable_if<std::is_integral<T>{},int>::type = 0
	,typename std::enable_if<(sizeof(T) == 8),int>::type = 0
	>
void randomize(random_generator & r, T & t)
{
	t = ((T)r() << 32) | (T)r();
}

///////////////////////// floats
template<typename T
	,typename std::enable_if<std::is_floating_point<T>{},int>::type = 0
	,typename std::enable_if<(sizeof(T) == 4),int>::type = 0
	>
void randomize(random_generator & r, T & t)
{
	do {
		int i = r();
		t = *reinterpret_cast<T*>(&i);
	} while (std::isnan(t));
}
template<typename T
	,typename std::enable_if<std::is_floating_point<T>{},int>::type = 0
	,typename std::enable_if<(sizeof(T) == 8),int>::type = 0
	>
void randomize(random_generator & r, T & t)
{
	do {
		uint64_t i = (((uint64_t)r() << 32)) | r();
		t = *reinterpret_cast<T*>(&i);
	} while (std::isnan(t));
}

///////////////////////// structs with a ::randomize() member function
template<typename T
	,typename std::enable_if<std::is_same<decltype(std::declval<T&>().randomize(std::declval<random_generator&>())),void>{},int>::type = 0
	>
void randomize(random_generator & r, T & t)
{
	t.randomize(r);
}

///////////////////////// variants
template<size_t i, typename V>
void fill_variant(random_generator&, int, V&)
{}
template<size_t i, typename V, typename U, typename...T>
void fill_variant(random_generator & r, int idx, V & v)
{
	if (i == idx)
		v.template emplace<i>(r.next<U>());
	else 
		fill_variant<i+1, V, T...>(r, idx, v);
}
template<typename...T>
void randomize(random_generator & r, std::variant<T...> & t)
{
	auto idx = r() % sizeof...(T);
	fill_variant<0, std::variant<T...>, T...>(r, idx, t);
}

///////////////////////// string
void randomize(random_generator & r, std::string & t)
{
	size_t size = r.next<unsigned>()%1024;
	t.reserve(size);
	while (size-->0)
		t.push_back((char)(32 + (r()%(256-32))));
}

} // namespace
