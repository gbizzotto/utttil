
#include <vector>
#include <algorithm>

#include "utttil/assert.hpp"
#include "utttil/dict.hpp"

bool test_remove_if_unstable()
{
	std::vector<int> v{1,2,6,3,4,78,65,3,4,5,8};
	v.erase(utttil::remove_if_unstable(v, [](int & i){ return i&1; }), v.end());
	ASSERT_ACT(v.size(), ==, 6ull, return false);
	return true;
}

int main()
{
	bool success = true
		&& test_remove_if_unstable()
		;

	if (success)
		std::cout << "Success" << std::endl;
	else
		std::cout << "Error" << std::endl;

	return success ? 0 : 1;
}