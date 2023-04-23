
#include "headers.hpp"

#include <iostream>
#include <variant>
#include "utttil/srlz.hpp"
#include "utttil/random.hpp"
#include "utttil/dfloat.hpp"
#include "utttil/fixed_string.hpp"
#include "utttil/assert.hpp"
#include "utttil/timer.hpp"

template<typename T, typename... Ts>
std::ostream & operator<<(std::ostream & os, const std::variant<T, Ts...>& v)
{
	os << (char)v.index();
    std::visit([&os](auto&& arg) { os << arg; }, v);
    return os;
}

struct mystruct
{
	utttil::dfloat<size_t,5> df;
	utttil::fixed_string<16> fs;

	template<typename Serializer>
	void serialize(Serializer && s) const
	{
		s << df << fs;
	}
	template<typename Deserializer>
	void deserialize(Deserializer && s)
	{
		s >> df >> fs;
	}

	template<typename Rand>
	void randomize(Rand & r)
	{
		df.randomize(r);
		fs.randomize(r);
	}

	size_t serialize_size() const
	{
		return df.serialize_size()
		     + fs.serialize_size()
		     ;
	}
};
bool operator==(const mystruct & left, const mystruct & right)
{
	return true
		&& left.df == right.df
		&& left.fs == right.fs
		;
}
std::ostream & operator<<(std::ostream & o, const mystruct & s)
{
	return o << s.df << ", " << s.fs;
}

using T = std::variant<int,float,std::string,mystruct>;

bool test_write(std::string path, utttil::random_generator & random, int count)
{
	utttil::srlz::journal_write j(path, 2048);

	for (int i=0 ; i<count ; i++)
		j.write(random.next<T>());

	return true;
}

bool test_read(std::string path, utttil::random_generator random, int count)
{
	utttil::srlz::journal_read j(path, 2048);

	for (int i=0 ; i<count ; i++)
		ASSERT_MSG_ACT(j.read<T>(), ==, random.next<T>(), std::to_string(i), return false);

	return true;
}

bool test_fuzz(std::string path, int seed)
{
	{
		utttil::random_generator random(seed);
		utttil::srlz::journal_write j(path, 1024*1024);
		utttil::do_for(std::chrono::seconds(10), [&]()
			{
				j.write(random.next<T>());
				return true;
			});
	}
	{
		utttil::random_generator random(seed);
		utttil::srlz::journal_read j(path, 1024*1024);
		size_t i=0;
		while ( ! j.eoj())
		{
			ASSERT_MSG_ACT(j.read<T>(), ==, random.next<T>(), std::to_string(i), return false);
			i++;
		}
	}
	return true;
}

int main()
{
	int seed1 = time(NULL);
	int seed2 = seed1 * 2;
	std::cout << "seed1: " << seed1 << std::endl;
	std::cout << "seed2: " << seed2 << std::endl;

	std::string path1 = std::string("/tmp/").append(std::to_string(seed1));
	std::filesystem::create_directory(path1);
	utttil::random_generator random1(seed1);

	std::string path2 = std::string("/tmp/").append(std::to_string(seed2));
	std::filesystem::create_directory(path2);
	utttil::random_generator random2(seed1);

	bool success = true
		&& test_write(path1, random1, 100)
		&& test_write(path1, random1, 100)
		&& test_read(path1, utttil::random_generator(seed1), 200)
		&& test_fuzz(path2, seed2)
		;

	if (success) {
		std::cout << "Success" << std::endl;
		return 0;
	} else {
		std::cout << "Failure" << std::endl;
		return 1;
	}
}