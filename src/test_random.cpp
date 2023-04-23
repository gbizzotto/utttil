
#include "headers.hpp"

#include <iostream>
#include <variant>
#include "utttil/assert.hpp"
#include "utttil/random.hpp"
#include "utttil/dfloat.hpp"
#include "utttil/fixed_string.hpp"

struct mystruct
{
	utttil::dfloat<int,5> df;
	utttil::fixed_string<12> fs;

	template<typename R>
	void randomize(R & r)
	{
		df.randomize(r);
		fs.randomize(r);
	}
};
bool operator==(const mystruct & left, const mystruct & right)
{
	return true
		&& left.df == right.df
		&& left.fs == right.fs
		;
}
template<typename O>
O & operator<<(O & o, const mystruct & s)
{
	return o << s.df << "," << s.fs;
}

template<typename O, typename T, typename... Ts>
O & operator<<(O & os, const std::variant<T, Ts...>& v)
{
    std::visit([&os](auto&& arg) { os << arg; }, v);
    return os;
}

bool test_random_generator()
{
	int seed = time(NULL);
	std::cout << seed << std::endl;

	utttil::random_generator random1(seed);
	utttil::random_generator random2(seed);

	for (size_t i=0 ; i<1000000 ; i++)
	{
		auto v1 = random1.next<std::variant<int,float,std::string,mystruct>>();
		auto v2 = random2.next<std::variant<int,float,std::string,mystruct>>();
		ASSERT_MSG_ACT(v1, ==, v2, std::to_string(v1.index()), return false);
	}

	return true;
}

int main()
{
	bool success = true
		&& test_random_generator()
		;

	if (success) {
		std::cout << "Success" << std::endl;
		return 0;
	} else {
		std::cout << "Failure" << std::endl;
		return 1;
	}
}