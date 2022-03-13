
#include <chrono>

#include <utttil/srlz.hpp>
#include <utttil/perf.hpp>

#include "msg.hpp"

utttil::measurement_point mps("srlz srlz");
utttil::measurement_point mpss("srlz operator<<");

bool test()
{
	msg mymsg{1234, "adsfasdlkfjasd;ljasd;lkdjsfaskjdafhskdj"};
	for ( auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3)
		; std::chrono::steady_clock::now() < deadline
		; )
	{
		utttil::measurement ms(mps);
		char buf[1024];
		auto s = utttil::srlz::to_binary(utttil::srlz::device::ptr_writer(buf+2));
		{
			utttil::measurement mss(mpss);
			s << mymsg;
		}
		size_t size = s.write.size() - 2;
		*buf = (size/256) & 0xFF;
		*(buf+1) = size & 0xFF;
	}
	return true;
}

int main()
{
	bool result = true
		&& test()
		;

	return result ? 0 : 1;
}