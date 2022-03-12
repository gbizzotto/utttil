
#include "headers.hpp"

#include <iostream>
#include "utttil/srlz/int.hpp"

template<typename T>
bool test(T t)
{
	char buffer[100];
	T t2;
	utttil::srlz::integral::serialize(t, buffer);
	try
	{
		utttil::srlz::integral::deserialize<T>(t2, buffer);
	}
	catch (std::exception & e)
	{
		std::cerr << "Overflow for type " << typeid(T).name() << ", value = " << (int)t << std::endl;
		return false;
	}

	if (t != t2)
	{
		std::cerr << "Bad for type " << typeid(T).name() << ", value = " << (int)t << ", result = " << (int)t2 << std::endl;
		return false;
	}
	
	return true;
}

template<typename T>
bool test()
{
	T t = -1;
	for (int size=std::rand()%(1+sizeof(T)) ; size>0 ; size--)
		t = (t<<8) | (char)(std::rand()%256);
	return test(t);
}

bool test_int()
{
	srand(time(0));

	size_t i = 0;
	for ( auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3)
		; std::chrono::steady_clock::now() < deadline
		; )
	{
		if (!( test<char>()
			&& test<short>()
			&& test<int>()
			&& test<long>()
			&& test<long long>()
			&& test<unsigned char>()
			&& test<unsigned short>()
			&& test<unsigned int>()
			&& test<unsigned long>()
			&& test<unsigned long long>()
			&& test<int64_t>()
			// && test<int128_t>()
			&& test<uint64_t>()
			// && test<uint128_t>()
			))
		{
			return false;
		}
		if (i%1000 == 0)
			std::cout << i++ << "\r";
	}
	return true;
}

int main()
{
	bool success = true
		&& test_int()
		;
	return success ? 0 : 1;
}