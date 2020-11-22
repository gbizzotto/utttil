
#pragma once

#include <chrono>
#include <iomanip>
#include <ctime>
#include <string>
#include <ostream>
#include <sstream>

namespace utttil {

template<typename Clock>
std::string to_string(const std::chrono::time_point<Clock> & time_point)
{
	// for nanoseconds
    typename Clock::duration tp = time_point.time_since_epoch();
    tp -= std::chrono::duration_cast<std::chrono::seconds>(tp);
	typename Clock::duration fraction = tp;

	// for days and shit
	std::string result;
	result.resize(31); // maybe constant time if compiler optimizes out CharT() ?
    time_t tt = Clock::to_time_t(time_point);
	tm t = *gmtime(&tt);
    size_t size = std::sprintf(result.data(),
				"%04u-%02u-%02uT%02u:%02u:%02u.%09lldZ",
				t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec,
                std::chrono::duration_cast<std::chrono::nanoseconds>(fraction).count()
				);
	return result;
}

template<typename Clock>
std::ostream & operator<<(std::ostream & out, const std::chrono::time_point<Clock> & time)
{
	return out << std::to_string(time);
}

inline std::string UTCTimestampISO8601()
{
	return to_string(std::chrono::system_clock::now());
}

} // namespace
