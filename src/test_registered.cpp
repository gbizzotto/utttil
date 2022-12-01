
#include "headers.hpp"

#include <utttil/assert.hpp>
#include <utttil/registered.hpp>

struct A : utttil::registered<A>
{
	int x;
};

bool test_registered()
{
	A * x = (A*)0x12345678;
	ASSERT_ACT(A::is_instance(x), ==, false, return false);
	ASSERT_ACT(x->is_registered(), ==, false, return false);

	{
		A a;
		x = &a;
		ASSERT_ACT(A::is_instance(&a), ==, true, return false);
		ASSERT_ACT(A::is_instance(x), ==, true, return false);
		ASSERT_ACT(a.is_registered(), ==, true, return false);
		ASSERT_ACT(x->is_registered(), ==, true, return false);
	}	
	ASSERT_ACT(A::is_instance(x), ==, false, return false);
	ASSERT_ACT(x->is_registered(), ==, false, return false);

	{
		auto b = std::make_shared<A>();
		x = b.get();
		ASSERT_ACT(A::is_instance(b.get()), ==, true, return false);
		ASSERT_ACT(A::is_instance(x), ==, true, return false);
		ASSERT_ACT(b->is_registered(), ==, true, return false);
		ASSERT_ACT(x->is_registered(), ==, true, return false);
	}
	ASSERT_ACT(A::is_instance(x), ==, false, return false);
	ASSERT_ACT(x->is_registered(), ==, false, return false);

	return true;
}

int main()
{
	bool success = test_registered();

	if (success)
		std::cout << "Success" << std::endl;
	else
		std::cout << "Failure" << std::endl;

	return success ? 0 : 1;
}