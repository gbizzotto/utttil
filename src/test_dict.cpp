
#include <iostream>
#include <immintrin.h>

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

bool test_seqdict_pool_string()
{
	using seqdict_pool = utttil::seqdict_pool<int16_t, std::string>;

	seqdict_pool p(1024);

	ASSERT_ACT(p.next_key(), ==, 0, return false);

	auto [seq0, inserted0] = p.push_back(std::string("100"));
	auto [seq1, inserted1] = p.push_back("101");

	ASSERT_ACT(p.next_key(), ==, 2, return false);
	ASSERT_ACT(inserted0, ==, true, return false);
	ASSERT_ACT(inserted1, ==, true, return false);
	ASSERT_ACT(seq0, ==, typename seqdict_pool::seq_type(0), return false);
	ASSERT_ACT(seq1, ==, typename seqdict_pool::seq_type(1), return false);
	ASSERT_ACT(p.size(), ==, size_t(2), return false);
	ASSERT_ACT(p.find(seq0), !=, nullptr, return false);
	ASSERT_ACT(p.find(seq1), !=, nullptr, return false);
	ASSERT_ACT(*p.find(seq0), ==, "100", return false);
	ASSERT_ACT(*p.find(seq1), ==, "101", return false);

	p.erase(seq0);
	ASSERT_ACT(p.size(), ==, size_t(1), return false);
	ASSERT_ACT(p.contains(seq0), ==, false, return false);

	auto [seq2, inserted2] = p.push_back("102");
	ASSERT_ACT(seq2, ==, typename seqdict_pool::seq_type(2), return false);
	ASSERT_ACT(p.size(), ==, size_t(2), return false);
	ASSERT_ACT(p.find(seq2), !=, nullptr, return false);
	ASSERT_ACT(p.find(seq1), !=, nullptr, return false);
	ASSERT_ACT(*p.find(seq2), ==, "102", return false);
	ASSERT_ACT(*p.find(seq1), ==, "101", return false);

	return true;
}

bool test_seqdict_vector()
{
	using seqdict_vector = utttil::seqdict_vector<uint64_t, std::string>;

	seqdict_vector s(16);

	ASSERT_ACT(s.next_key(), ==,seqdict_vector::seq_type(0), return false);

	auto [seq0, inserted0] = s.push_back(std::string("100"));
	auto [seq1, inserted1] = s.push_back("101");

	ASSERT_ACT(s.next_key(), ==, seqdict_vector::seq_type(2), return false);
	ASSERT_ACT(inserted0, ==, true, return false);
	ASSERT_ACT(inserted1, ==, true, return false);
	ASSERT_ACT(seq0, ==, typename seqdict_vector::seq_type(0), return false);
	ASSERT_ACT(seq1, ==, typename seqdict_vector::seq_type(1), return false);
	ASSERT_ACT(s.size(), ==, size_t(2), return false);
	ASSERT_ACT(s.find(seq0), !=, nullptr, return false);
	ASSERT_ACT(s.find(seq1), !=, nullptr, return false);
	ASSERT_ACT(*s.find(seq0), ==, "100", return false);
	ASSERT_ACT(*s.find(seq1), ==, "101", return false);

	for (int i=s.next_key() ; i<1025 ; i++)
	{
		ASSERT_ACT(s.size(), ==, (size_t)i, return false);
		auto [seq, inserted] = s.push_back(std::to_string(i));
		ASSERT_ACT(seq, ==, (size_t)i, return false);
		ASSERT_ACT(s.find(seq), !=, nullptr, return false);
		ASSERT_ACT(*s.find(seq), ==, std::to_string(i), return false);
	}

	std::cout << s.capacity() << std::endl;

	return true;
}

int main()
{
	bool success = true
		&& test_seqdict_pool()
		&& test_seqdict_pool_string()
		&& test_seqdict_vector()
		;

	if (success)
		std::cout << "Success" << std::endl;
	else
		std::cout << "Failure" << std::endl;

	return success ? 0 : 1;
}