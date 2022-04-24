
#include <iostream>
#include <string>
#include <chrono>
#include <string_view>

#include <utttil/perf.hpp>
#include <utttil/assert.hpp>

#include "utttil/sha256.hpp"

bool test(std::string s, utttil::Hash256 oracle)
{
	utttil::Hash256 h = utttil::fill_sha256(s);
	ASSERT_ACT(h, ==, oracle, return false);
	return true;
}

bool test_perf()
{
	utttil::measurement_point mp("sha256");

	utttil::sha256_padding padding;
	padding.set_data_size(80);
	
	for ( auto timeout=std::chrono::system_clock::now() + std::chrono::seconds(10)
	    ; std::chrono::system_clock::now() < timeout 
		; )
	{
		char header[80];
		utttil::measurement m(mp);
		utttil::Hash256 h;
		utttil::svistreamN svi({std::string_view(header, 80), padding.to_string_view()});
		utttil::fill_sha256_stream(h, svi);
	}
	return true;
}

int main()
{
	bool success = true
		&& test("a", "ca978112ca1bbdcafac231b39a23dc4da786eff8147c4e72b9807785afee48bb"_littleendian_Hash256)
		&& test("", "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"_littleendian_Hash256)
		&& test("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", "635361c48bb9eab14198e76ea8ab7f1a41685d6ad62aa9146d301d4f17eb0ae0"_littleendian_Hash256)
		&& test("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", "ffe054fe7ae0cb6dc65c3af9b61d5209f439851db43d0ba5997337df154668eb"_littleendian_Hash256)
		&& test("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", "7d3e74a05d7db15bce4ad9ec0658ea98e3f06eeecf16b4c6fff2da457ddc2f34"_littleendian_Hash256)
		&& test("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", "f506898cc7c2e092f9eb9fadae7ba50383f5b46a2a4fe5597dbb553a78981268"_littleendian_Hash256)
		&& test("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", "35d5fc17cfbbadd00f5e710ada39f194c5ad7c766ad67072245f1fad45f0f530"_littleendian_Hash256)
		&& test("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", "11ee391211c6256460b6ed375957fadd8061cafbb31daf967db875aebd5aaad4"_littleendian_Hash256)
		&& test("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", "111bb261277afd65f0744b247cd3e47d386d71563d0ed995517807d5ebd4fba3"_littleendian_Hash256)
		&& test("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", "d5c039b748aa64665782974ec3dc3025c042edf54dcdc2b5de31385b094cb678"_littleendian_Hash256)
		&& test("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", "f13b2d724659eb3bf47f2dd6af1accc87b81f09f59f2b75e5c0bed6589dfe8c6"_littleendian_Hash256)
		&& test("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", "b35439a4ac6f0948b6d6f9e3c6af0f5f590ce20f1bde7090ef7970686ec6738a"_littleendian_Hash256)
		&& test("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", "9f4390f8d30c2dd92ec9f095b65e2b9ae9b0a925a5258e241c9f1e910f734318"_littleendian_Hash256)
		&& test("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", "c12cb024a2e5551cca0e08fce8f1c5e314555cc3fef6329ee994a3db752166ae"_littleendian_Hash256)
		&& test("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", "6836cf13bac400e9105071cd6af47084dfacad4e5e302c94bfed24e013afb73e"_littleendian_Hash256)
		&& test("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", "c57e9278af78fa3cab38667bef4ce29d783787a2f731d4e12200270f0c32320a"_littleendian_Hash256)
		&& test("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", "36bcf9292589fe6ea3e82fefe3aab1b8ca8b8347ea5a14b23e470ecb3ad7c57b"_littleendian_Hash256)
		&& test("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", "a8e1a0f35c15c01a458bcb345528b751556c14850f6f9bdcc9933b8003d29b43"_littleendian_Hash256)
		&& test("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", "9c1ed60a6337af7d1659339ef95dccb7c91a55b9eaab349341a7f97198eee519"_littleendian_Hash256)
		&& test("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", "6675ba780648c8506cb002c86621f9d33f8093e12541c690a8d3261c47c92bcc"_littleendian_Hash256)
		&& test("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", "cf6c368c04c239cc1688a5a3b39f94a55ce5b7631e027fc0138f0548189c62fa"_littleendian_Hash256)
		&& test("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", "e9615320128cc7a3d6078e9af05603188e5ccbf0d07d8b735d3df5e8e0c1281f"_littleendian_Hash256)
		&& test("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", "2f3d335432c70b580af0e8e1b3674a7c020d683aa5f73aaaedfdc55af904c21c"_littleendian_Hash256)
		&& test("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", "31eba51c313a5c08226adf18d4a359cfdfd8d2e816b13f4af952f7ea6584dcfb"_littleendian_Hash256)
		//&& test_perf()
		;

	return success ? 0 : 1;
}