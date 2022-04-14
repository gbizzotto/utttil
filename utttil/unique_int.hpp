
#pragma once

namespace utttil {

struct unique_int_default_tag {};

template<typename Type, typename Tag>
struct unique_int
{
	using T = Type;
	using tag = Tag;

	T t;
	unique_int() = default;
	explicit unique_int(const T& t_) : t(t_) {}
	unique_int(const unique_int & other) : t(other.t) {}
	unique_int& operator=(const unique_int & rhs) { t = rhs.t; return *this; }
	unique_int& operator=(const T& rhs) { t = rhs; return *this; }
	explicit operator const T&() const { return t; }
	explicit operator T&() { return t; }
	template<typename To>
	explicit operator To() const { return t; }
	T value() const { return t; }
	unique_int & operator++() { ++t; return *this; }
	unique_int operator++(int) { return unique_int(t++); }
	unique_int & operator--() { --t; return *this; }
	unique_int operator--(int) { return unique_int(t--); }
	unique_int & operator+=(const unique_int & other) { t += other.t; return *this; }
	unique_int & operator-=(const unique_int & other) { t -= other.t; return *this; }
	unique_int operator-(const unique_int & other) const { return unique_int{t - other.t}; }
	unique_int operator+(const unique_int & other) const { return unique_int{t + other.t}; }

	template<typename Serializer>
	void serialize(Serializer && s) const
	{
		s << t;
	}
	template<typename Deserializer>
	void deserialize(Deserializer && s)
	{
		s >> (T&)t;
	}
};

template<typename Type, typename Tag> bool operator==(const unique_int<Type,Tag> & left, const unique_int<Type,Tag> & right) { return left.t == right.t; }
template<typename Type, typename Tag> bool operator< (const unique_int<Type,Tag> & left, const unique_int<Type,Tag> & right) { return left.t <  right.t; }
template<typename Type, typename Tag> bool operator<=(const unique_int<Type,Tag> & left, const unique_int<Type,Tag> & right) { return left.t <= right.t; }
template<typename Type, typename Tag> bool operator!=(const unique_int<Type,Tag> & left, const unique_int<Type,Tag> & right) { return left.t != right.t; }

template<typename LT, typename Type, typename Tag> bool operator==(const LT & left, const unique_int<Type,Tag> & right) { return left == right.t; }
template<typename LT, typename Type, typename Tag> bool operator< (const LT & left, const unique_int<Type,Tag> & right) { return left <  right.t; }
template<typename LT, typename Type, typename Tag> bool operator<=(const LT & left, const unique_int<Type,Tag> & right) { return left <= right.t; }
template<typename LT, typename Type, typename Tag> bool operator!=(const LT & left, const unique_int<Type,Tag> & right) { return left != right.t; }

template<typename RT, typename Type, typename Tag> bool operator==(const unique_int<Type,Tag> & left, const RT & right) { return left.t == right; }
template<typename RT, typename Type, typename Tag> bool operator< (const unique_int<Type,Tag> & left, const RT & right) { return left.t <  right; }
template<typename RT, typename Type, typename Tag> bool operator<=(const unique_int<Type,Tag> & left, const RT & right) { return left.t <= right; }
template<typename RT, typename Type, typename Tag> bool operator!=(const unique_int<Type,Tag> & left, const RT & right) { return left.t != right; }

template<typename Type, typename Tag>
std::ostream & operator<<(std::ostream & out, const unique_int<Type,Tag> & ui)
{
	return out << ui.t;
}

} // namespace
