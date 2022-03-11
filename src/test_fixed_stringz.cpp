
#include <iostream>
#include <random>
#include <vector>
#include "utttil/fixed_stringz.hpp"
#include "utttil/fixed_string.hpp"
#include "utttil/assert.hpp"
#include "utttil/perf.hpp"
#include "utttil/srlz.hpp"

#define STR_SIZE 64

bool test_integrity()
{
	utttil::fixed_stringz<10> fsz1("abc");
	utttil::fixed_stringz<10> fsz2("abc");
	utttil::fixed_stringz<10> fsz3("abcdef");
	ASSERT_ACT(fsz1, ==, fsz2, return false);
	ASSERT_ACT(fsz1, !=, fsz3, return false);
	ASSERT_ACT(fsz3, ==, fsz3, return false);
	ASSERT_ACT(fsz1.size(), ==, 3ull, return false);
	ASSERT_ACT(fsz3.size(), ==, 6ull, return false);
	utttil::fixed_stringz<1> fsz4("abcdef");
	utttil::fixed_stringz<1> fsz5;
	ASSERT_ACT(fsz5.size(), ==, 0ull, return false);
	ASSERT_ACT(fsz4, !=, fsz5, return false);
	auto fsz1_2 = fsz1;
	ASSERT_ACT(fsz1, ==, fsz1_2, return false);
	fsz3.resize(3);
	ASSERT_ACT(fsz3, ==, fsz1, return false);
	fsz5 = "abc";
	ASSERT_ACT(fsz5, ==, fsz4, return false);
	return true;
}
bool test_perf()
{
	{
		utttil::measurement_point mp("fixed_string copy");
		utttil::fixed_stringz<STR_SIZE> fs, fs2;
		fs.randomize(std::rand);
		for (int i=0 ; i<100000 ; i++)
		{
			utttil::measurement m(mp);
			fs2 = fs;
		}
	}
	{
		utttil::measurement_point mp("fixed_stringz copy");
		utttil::fixed_string<STR_SIZE> fs, fs2;
		fs.randomize(std::rand);
		for (int i=0 ; i<100000 ; i++)
		{
			utttil::measurement m(mp);
			fs2 = fs;
		}
	}
	{
		utttil::measurement_point mpd("fixed_string desrlz");
		utttil::measurement_point mps("fixed_string srlz");
		utttil::fixed_string<STR_SIZE> fs, fs2;
		std::vector<char> buffer;
		buffer.reserve(33);
		fs.randomize(std::rand);
		for (int i=0 ; i<100000 ; i++)
		{
			auto out = utttil::srlz::to_binary(utttil::srlz::device::back_inserter(buffer));
			{
				utttil::measurement m(mps);
				out << fs;
			}
			auto in = utttil::srlz::from_binary(utttil::srlz::device::iterator_reader(buffer.begin(), buffer.end()));
			{
				utttil::measurement m(mpd);
				in >> fs2;
			}
			ASSERT_ACT(fs, ==, fs2, return false);
		}
	}
	{
		utttil::measurement_point mpd("fixed_stringz desrlz");
		utttil::measurement_point mps("fixed_stringz srlz");
		utttil::fixed_stringz<STR_SIZE> fs, fs2;
		std::vector<char> buffer;
		buffer.reserve(33);
		fs.randomize(std::rand);
		for (int i=0 ; i<100000 ; i++)
		{
			auto out = utttil::srlz::to_binary(utttil::srlz::device::back_inserter(buffer));
			{
				utttil::measurement m(mps);
				out << fs;
			}
			auto in = utttil::srlz::from_binary(utttil::srlz::device::iterator_reader(buffer.begin(), buffer.end()));
			{
				utttil::measurement m(mpd);
				in >> fs2;
			}
			ASSERT_ACT(fs, ==, fs2, return false);
		}
	}
	return true;
}

int main()
{
	bool success = test_integrity()
		&& test_perf()
		;
	return success?0:1;
}