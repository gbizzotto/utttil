
#include <iostream>

#include <utttil/assert.hpp>

#include <utttil/dict.hpp>

bool test_seqdict_pool()
{
	using seqdict_pool = utttil::seqdict_pool<size_t, int>;

	seqdict_pool p(1024);

	auto [seq0, inserted0] = p.push_back(100);
	auto [seq1, inserted1] = p.push_back(101);

	ASSERT_ACT(inserted0, ==, true, return false);
	ASSERT_ACT(inserted1, ==, true, return false);
	ASSERT_ACT(seq0, ==, typename seqdict_pool::seq_type(0), return false);
	ASSERT_ACT(seq1, ==, typename seqdict_pool::seq_type(1), return false);
	ASSERT_ACT(p.size(), ==, size_t(2), return false);
	ASSERT_ACT(p.find(seq0), !=, nullptr, return false);
	ASSERT_ACT(p.find(seq1), !=, nullptr, return false);
	ASSERT_ACT(*p.find(seq0), ==, 100, return false);
	ASSERT_ACT(*p.find(seq1), ==, 101, return false);

	p.erase(seq0);
	ASSERT_ACT(p.size(), ==, size_t(1), return false);
	ASSERT_ACT(p.contains(seq0), ==, false, return false);

	auto [seq2, inserted2] = p.push_back(102);
	ASSERT_ACT(seq2, ==, typename seqdict_pool::seq_type(2), return false);
	ASSERT_ACT(p.size(), ==, size_t(2), return false);
	ASSERT_ACT(p.find(seq2), !=, nullptr, return false);
	ASSERT_ACT(p.find(seq1), !=, nullptr, return false);
	ASSERT_ACT(*p.find(seq2), ==, 102, return false);
	ASSERT_ACT(*p.find(seq1), ==, 101, return false);

	return true;
}

int main()
{
	bool success = true
		&& test_seqdict_pool()
		;

	if (success)
		std::cout << "Success" << std::endl;
	else
		std::cout << "Failure" << std::endl;

	return success ? 0 : 1;
}