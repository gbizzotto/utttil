
#pragma once

#include <cstdlib>
#include <type_traits>

namespace utttil {

template<typename T, decltype(std::numeric_limits<T>::max())=0>
constexpr T max()
{
	return std::numeric_limits<T>::max();
}
template<typename T,
	typename X = typename T::T
	>
constexpr T max()
{
	// for unique_int
	return T(std::numeric_limits<typename T::T>::max());
}
template<>
constexpr __int128_t max<__int128_t>()
{
	return ((((__int128_t(1)) << 126) - 1) << 1) + 1;
}
template<>
constexpr __uint128_t max<__uint128_t>()
{
	return ((((__uint128_t(1)) << 127) - 1) << 1) + 1;
}

template<typename T, decltype(std::numeric_limits<T>::lowest())=0>
constexpr T lowest()
{
	return std::numeric_limits<T>::lowest();
}
template<typename T,
	typename X = typename T::T
	>
constexpr T lowest()
{
	// for unique_int
	return T(std::numeric_limits<typename T::T>::lowest());
}
template<>
constexpr __int128_t lowest<__int128_t>()
{
	//return ((((__int128_t(1)) << 126) - 1) << 1) + 1;
	return __int128_t(1) << 127;
}
template<>
constexpr __uint128_t lowest<__uint128_t>()
{
	return ((((__uint128_t(1)) << 127) - 1) << 1) + 1;
}

template<typename T>
constexpr inline T exp(T v, size_t e)
{
	T r = 1;
	while(e-->0)
		r *= v;
	return r;
}
template<typename T>
constexpr size_t digits(T t, int base)
{
	size_t l = 0;
	while(t != 0)
	{
		t /= base;
		l++;
	}
	return l;
}
template<typename T
	,typename std::enable_if<
		std::disjunction< // this is an OR
				std::is_integral<T>,
				std::is_same<T,__int128_t>,
				std::is_same<T,__uint128_t>
			>{},int
		>::type = 0
	>
T rand()
{
	T max = utttil::max<T>();
	size_t bits_size = digits(max, 2);
	size_t bits = std::rand()%(bits_size+1);
	T result = 0;
	while (bits-->0)
		result = (result<<1) | (std::rand()&1);
	return result;
}
template<typename T, typename X = typename T::T>
T rand()
{
	return T(rand<typename T::T>());
}

// inline bool add_w_overflow(unsigned int a, unsigned int b, unsigned int * res)
// {
// 	return __builtin_uadd_overflow(a, b, res);
// }
// inline bool add_w_overflow(int a, int b, int * res)
// {
// 	return __builtin_sadd_overflow(a, b, res);
// }

// inline bool add_w_overflow(unsigned long int a, unsigned long int b, unsigned long int * res)
// {
// 	return __builtin_uaddl_overflow(a, b, res);
// }
// inline bool add_w_overflow(long int a, long int b, long int * res)
// {
// 	return __builtin_saddl_overflow(a, b, res);
// }

// inline bool add_w_overflow(unsigned long long int a, unsigned long long int b, unsigned long long int * res)
// {
// 	return __builtin_uaddll_overflow(a, b, res);
// }
// inline bool add_w_overflow(long long int a, long long int b, long long int * res)
// {
// 	return __builtin_saddll_overflow(a, b, res);
// }

} // namespace
