
#pragma once

#include <iostream>
#include <string>
#include <cstdint>
#include <assert.h>
#include <algorithm>
#include <type_traits>

#include "utttil/math.hpp"

namespace std {
	template<>
	struct make_unsigned<__int128_t> {
		using type=__uint128_t;
	};
	template<>
	struct make_unsigned<__uint128_t> {
		using type=__uint128_t;
	};
}

namespace utttil {

template<typename T>
constexpr size_t useful_exponent(T t)
{
	T m = max<T>();

	size_t exp_bits = 0;
	while(utttil::exp<T>(2, exp_bits)-1 < utttil::digits(m, 10))
	{
		exp_bits++;
		m = m/2 + 1;
	}
	return exp_bits;
}

template<typename M>
constexpr M* max_per(M max_mantissa, int max_exponent)
{
	M * result = new M[max_exponent+1];
	for (int e = 0 ; e<=max_exponent ; e++)
		result[e] = max_mantissa / utttil::exp<M>(10, e);
	return result;
}

struct dfloat_default_tag {};

template<typename M, size_t Bits, typename Tag=dfloat_default_tag>
struct dfloat
{
	static_assert(Bits > 0);
	static_assert(Bits < 8*sizeof(M));

	using mantissa_t = M;
	using exponent_t = typename std::make_unsigned<mantissa_t>::type;
	static inline const size_t       exponent_bits = Bits;
	static inline const size_t       mantissa_bits = 8*sizeof(M) - exponent_bits;
	static inline const mantissa_t   max_mantissa  = utttil::max<mantissa_t>() >> exponent_bits;
	static inline const mantissa_t   min_mantissa  = utttil::lowest<mantissa_t>() >> exponent_bits;
	static inline const exponent_t   max_exponent  = utttil::max<exponent_t>() >> mantissa_bits;
	static inline const size_t       digits        = utttil::digits(max_mantissa, 10);
	static inline const mantissa_t * max_per_E     = max_per(max_mantissa, max_exponent);

	mantissa_t mantissa : mantissa_bits;
	exponent_t exponent : exponent_bits;

	dfloat()
		: mantissa(0)
		, exponent(0)
	{}
	template<typename T
		,typename std::enable_if<
			std::disjunction< // this is an OR
					std::is_integral<T>,
					std::is_same<T,__int128_t>,
					std::is_same<T,__uint128_t>
				>{},int
			>::type = 0
		>
	dfloat(T t)
		: mantissa(t)
		, exponent(0)
	{
		assert(t <= max_mantissa);
	}
	dfloat(const dfloat & other) = default;
	// 	: mantissa(other.mantissa)
	// 	, exponent(other.exponent)
	// {}
	dfloat(dfloat && other) = default;
	// 	: mantissa(other.mantissa)
	// 	, exponent(other.exponent)
	// {}
	template<typename Tag2>
	explicit dfloat(const dfloat<M,Bits,Tag2> & other)
		: mantissa(other.mantissa)
		, exponent(other.exponent)
	{}
	dfloat(mantissa_t man, exponent_t exp)
		: mantissa(man)
		, exponent(exp)
	{
		assert(man <= max_mantissa);
		assert(exp <= max_exponent);
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
	dfloat & operator=(T t)
	{
		assert(t <= max_mantissa);
		mantissa = t;
		exponent = 0;
		return *this;
	}
	dfloat & operator=(const dfloat & other)
	{
		mantissa = other.mantissa;
		exponent = other.exponent;
		return *this;
	}
	template<typename Tag2>
	dfloat & operator=(dfloat<M,Bits,Tag2> & other)
	{
		mantissa = other.mantissa;
		exponent = other.exponent;
		return *this;
	}

	template<typename M2, size_t B2, typename Tag2>
	bool add_loss(dfloat<M2,B2,Tag2> other)
	{
		bool overflow = other.normalize_loss(this->exponent);
		if (overflow)
			overflow = this->normalize_loss(other.exponent);

		// mantissa_t is bigger than mantissa, so no overflow
		mantissa_t result = mantissa + other.mantissa;
		overflow = result < min_mantissa || result > max_mantissa;
		if ( ! overflow)
			mantissa = result;
		return overflow;
	}
	template<typename M2, size_t B2, typename Tag2>
	bool sub_loss(dfloat<M2,B2,Tag2> other)
	{
		bool overflow = other.normalize_loss(this->exponent);
		if (overflow)
			overflow = this->normalize_loss(other.exponent);

		// mantissa_t is bigger than mantissa, so no overflow
		mantissa_t result = mantissa - other.mantissa;
		overflow = result < min_mantissa || result > max_mantissa;
		if ( ! overflow)
			mantissa = result;
		return overflow;
	}

	bool normalize_loss(exponent_t e)
	{
		if (exponent == e)
			return false;
		else if (exponent > e)
		{
			mantissa_t divisor = utttil::exp<mantissa_t>(10, exponent - e);
			bool overflow = (mantissa % divisor) != 0;
			if ( ! overflow)
				mantissa /= divisor;
			return overflow;
		}
		else
		{
			// unsigned int (0,+268435455), 4 bits exponent

			// 12.3 exp=1
			// e=4 -> 12.3000
			// fits into mantissa_t? Yes

			// 268435.455, exp=3
			// e=8 -> 268435.45500000
			// fits into mantissa_t? No

			bool overflow = mantissa > max_per_E[e-exponent];
			if ( ! overflow)
			{
				mantissa *= utttil::exp<mantissa_t>(10, e-exponent);
				exponent = e;
			}
			return overflow;
		}
	}

	template<typename Serializer>
	void serialize(Serializer && s) const
	{
		s << mantissa_t(mantissa)
		  << exponent_t(exponent);
	}
	template<typename Deserializer>
	void deserialize(Deserializer && s)
	{
		mantissa_t m;
		exponent_t e;
		s >> (mantissa_t&)m >> (exponent_t&)e;
		mantissa = m;
		exponent = e;
	}

	static inline dfloat rand()
	{
		return dfloat(utttil::rand<mantissa_t>()%max_mantissa, utttil::rand<exponent_t>()%max_exponent);
	}
};

template<typename M1, size_t B1, typename Tag1, typename M2, size_t B2, typename Tag2>
bool operator==(const utttil::dfloat<M1,B1,Tag1> & left, const utttil::dfloat<M2,B2,Tag2> & right)
{
	if (left.exponent > right.exponent)
	{
		M1 reduced_left_mantissa = left.mantissa;
		auto divisor = utttil::exp<M1>(10, left.exponent-right.exponent);
		bool remainder = (reduced_left_mantissa % divisor) != 0;
		reduced_left_mantissa /= divisor;
		return reduced_left_mantissa == right.mantissa && ! remainder;
	}
	else if (left.exponent < right.exponent)
	{
		M1 reduced_right_mantissa = right.mantissa;
		auto divisor = utttil::exp<M2>(10, right.exponent-left.exponent);
		bool remainder = (reduced_right_mantissa % divisor) != 0;
		reduced_right_mantissa /= divisor;
		return left.mantissa == reduced_right_mantissa && ! remainder;
	}
	else // (left.exponent == right.exponent)
		return left.mantissa == right.mantissa;
}

template<typename M1, size_t B1, typename Tag1, typename M2, size_t B2, typename Tag2>
bool operator<(const utttil::dfloat<M1,B1,Tag1> & left, const utttil::dfloat<M2,B2,Tag2> & right)
{
	if (left.exponent > right.exponent)
	{
		M1 reduced_left_mantissa = left.mantissa;
		auto divisor = utttil::exp<M1>(10, left.exponent-right.exponent);
		reduced_left_mantissa /= divisor;
		return reduced_left_mantissa < right.mantissa;
	}
	else if (left.exponent < right.exponent)
	{
		M1 reduced_right_mantissa = right.mantissa;
		auto divisor = utttil::exp<M2>(10, right.exponent-left.exponent);
		bool remainder = (reduced_right_mantissa % divisor) != 0;
		reduced_right_mantissa /= divisor;
		return left.mantissa < reduced_right_mantissa || (remainder && left.mantissa == reduced_right_mantissa);
	}
	else // (left.exponent == right.exponent)
		return left.mantissa < right.mantissa;
}

template<typename M1, size_t B1, typename Tag1, typename M2, size_t B2, typename Tag2>
bool operator<=(const utttil::dfloat<M1,B1,Tag1> & left, const utttil::dfloat<M2,B2,Tag2> & right)
{
	if (left.exponent > right.exponent)
	{
		M1 reduced_left_mantissa = left.mantissa;
		auto divisor = utttil::exp<M1>(10, left.exponent-right.exponent);
		bool remainder = (reduced_left_mantissa % divisor) != 0;
		reduced_left_mantissa /= divisor;
		return reduced_left_mantissa < right.mantissa || (reduced_left_mantissa == right.mantissa && ! remainder);
	}
	else if (left.exponent < right.exponent)
	{
		M1 reduced_right_mantissa = right.mantissa;
		auto divisor = utttil::exp<M2>(10, right.exponent-left.exponent);
		reduced_right_mantissa /= divisor;
		return left.mantissa <= reduced_right_mantissa;
	}
	else // (left.exponent == right.exponent)
		return left.mantissa <= right.mantissa;
}

template<typename M, size_t B,typename Tag>
std::ostream & operator<<(std::ostream & out, const utttil::dfloat<M,B,Tag> & dfp)
{
	std::string buf;
	buf.reserve(50);
	
	int decimal_digits = dfp.exponent;
	auto x = dfp.mantissa;
	while(decimal_digits)
	{
		auto r = x%10;
		if (r != 0)
			break;
		x /= 10;
		decimal_digits--;
	}
	while(decimal_digits-->0)
	{
		buf.append(1, '0'+x%10);
		x /= 10;
	}
	if (buf.size() > 0)
		buf.append(1, '.');
	do
	{
		buf.append(1, '0'+x%10);
		x /= 10;
	} while(x>0);
	if (dfp.mantissa < 0)
		buf.append(1, '-');
	std::reverse(buf.begin(), buf.end());
	return out << buf;
}

} // namespace
