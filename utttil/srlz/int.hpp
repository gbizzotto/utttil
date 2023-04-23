
#pragma once

#include <random>
#include <algorithm>
#include <type_traits>
#include <cstdlib>

namespace utttil {
namespace srlz {
namespace integral {

template<typename T
	,typename std::enable_if<
		std::disjunction<
			std::is_integral<T>,
			std::is_same<T,__int128_t>,
			std::is_same<T,__uint128_t>
		>{},int
	>::type = 0
>
size_t serialize(T t, char * buf)
{
	char * const b = buf;
	do
	{
		*buf = t & 0x7F; // 7 lower bits
		buf++;
		t >>= 7;
	}while(t != 0 && t != (T)-1);

	// set stop bit on lowest part
	b[0] |= 0x80;

	if (t == (T)-1 && ((*(buf-1)) & 0x40) == 0)
	{
		// t negative but highest bit kept is zero
		// add an all-1 byte
		*buf = 0x7F;
		buf++;
	}
	else if (t == 0 && ((*(buf-1)) & 0x40) == 0x40)
	{
		// t positive but highest bit kept is one
		// add an all-0 byte
		*buf = 0;
		buf++;
	}

	std::reverse(b, buf);
	return buf-b;
}

template<typename T
	,typename std::enable_if<
		std::disjunction<
			std::is_same<T,__int128_t>,
			std::is_same<T,__uint128_t>
		>{},int
	>::type = 0
>
size_t serialize_size(T t)
{
	return 1
		+ (t >= ((T)1<<  7))
		+ (t >= ((T)1<< 14))
		+ (t >= ((T)1<< 21))
		+ (t >= ((T)1<< 28))
		+ (t >= ((T)1<< 35))
		+ (t >= ((T)1<< 42))
		+ (t >= ((T)1<< 49))
		+ (t >= ((T)1<< 56))
		+ (t >= ((T)1<< 63))
		+ (t >= ((T)1<< 70))
		+ (t >= ((T)1<< 77))
		+ (t >= ((T)1<< 84))
		+ (t >= ((T)1<< 91))
		+ (t >= ((T)1<< 98))
		+ (t >= ((T)1<<105))
		+ (t >= ((T)1<<112))
		+ (t >= ((T)1<<119))
		+ (t >= ((T)1<<126))
		;
}
template<typename T
	,typename std::enable_if<std::is_integral<T>{},int>::type = 0
	,typename std::enable_if<(sizeof(T) == 8),int>::type = 0
>
size_t serialize_size(T t)
{
	return 1
		+ (t >= ((T)1<<  7))
		+ (t >= ((T)1<< 14))
		+ (t >= ((T)1<< 21))
		+ (t >= ((T)1<< 28))
		+ (t >= ((T)1<< 35))
		+ (t >= ((T)1<< 42))
		+ (t >= ((T)1<< 49))
		+ (t >= ((T)1<< 56))
		+ (t >= ((T)1<< 63))
		;
}
template<typename T
	,typename std::enable_if<std::is_integral<T>{},int>::type = 0
	,typename std::enable_if<(sizeof(T) <= 4),int>::type = 0
>
size_t serialize_size(T t)
{
	return 1
		+ (t >= ((T)1<<  7))
		+ (t >= ((T)1<< 14))
		+ (t >= ((T)1<< 21))
		+ (t >= ((T)1<< 28))
		;
}

template<typename T, typename std::enable_if<std::is_integral<T>{},int>::type = 0>
void deserialize(T & t, const char * v)
{
	t = (T)-1 * (((*v) & 0x40)==0x40);
	do
	{
		t = (t<<7) | ((*v) & 0x7F);
	}while((*(v++) & 0x80) == 0);
}
template<typename T
	,typename F
	,typename std::enable_if<std::disjunction< // this is an OR
			std::is_integral<T>,
			std::is_same<T,__int128_t>,
			std::is_same<T,__uint128_t>
		>{},int>::type = 0
	,typename std::enable_if<std::is_invocable<F>{},int>::type = 0
	>
T deserialize(F & read)
{
	char v = read();
	T result = (T)-1 * ((v & 0x40)==0x40);
	while((v & 0x80) == 0)
	{
		result = (result<<7) | (v & 0x7F);
		v = read();
	}
	result = (result<<7) | (v & 0x7F);
	return result;
}

}}} // namespace
