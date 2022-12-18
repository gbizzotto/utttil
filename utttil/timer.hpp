
#pragma once

#include <chrono>

namespace utttil {

template<typename C>
struct Timer
{
	std::chrono::time_point<C> start = C::now();
	std::chrono::duration<C> Elapsed() const { return C::now() - start; }
	void Reset() { start = C::now(); }
	template<typename D>
	bool OlderThan(D duration) const { return start+duration < C::now(); }
};

template<typename D, typename F>
void do_for(D d, F f)
{
	for ( auto deadline = std::chrono::steady_clock::now()+d
		; std::chrono::steady_clock::now() < deadline
		; )
	{
		if ( ! f()) break;
	}	
}

} // namespace
