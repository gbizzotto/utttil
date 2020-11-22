
#pragma once

#include <iostream>
#include <vector>
#include <mutex>

#include "utttil/timestamp.hpp"

namespace utttil {

template<size_t S=0>
struct Log
{
	std::vector<std::ostream*> & outs;
	std::scoped_lock<std::recursive_mutex> lock;
	bool do_log;

	Log(std::vector<std::ostream*> & outs, std::recursive_mutex & mutex, bool do_log)
		: outs(outs)
		, lock(mutex)
		, do_log(do_log)
	{}

	void flush()
	{
		if ( ! do_log)
			return;
		for (auto & out : outs)
			out->flush();
	}
};
template<typename T, size_t S=0>
std::unique_ptr<Log<S>> operator<<(std::unique_ptr<Log<S>> log, const T & t)
{
	if (log->do_log)
		for (auto & out : log->outs)
			*out << t;
	return log;
}
template<typename T, size_t S=0>
std::unique_ptr<Log<S>> operator<<(std::unique_ptr<Log<S>> log, T && t)
{
	if (log->do_log)
		for (auto & out : log->outs)
			*out << t;
	return log;
}
template<size_t S=0>
std::unique_ptr<Log<S>> operator<<(std::unique_ptr<Log<S>> log, const char * t)
{
	if (log->do_log)
		for (auto & out : log->outs)
			*out << t;
	return log;
}
// support for std::endl and other modifiers
template<size_t S=0>
std::unique_ptr<Log<S>> operator<<(std::unique_ptr<Log<S>> log, std::ostream& (*f)(std::ostream&))
{
	if (log->do_log)
		for (auto & out : log->outs)
			f(*out);
	return log;
}

enum class LogLevel
{
	INFO,
	WARNING,
	ERROR,
	FATAL,
	MUST_HAVE,
};
static inline const char * LogLevelNames[] =
{
	"INFO",
	"WARNING",
	"ERROR",
	"FATAL",
	"MUST_HAVE",
};

template<size_t S>
struct LogWithPrefix_
{
	std::string prefix;
	LogLevel level;
	std::vector<std::ostream*> outs;

	static inline std::recursive_mutex cout_mutex;

	LogWithPrefix_(std::string prefix = "")
		: prefix(std::move(prefix))
		, level(LogLevel::INFO)
	{}

	void set_prefix(std::string p) { prefix = std::move(p); }
	void set_level(LogLevel l) { level = l; }
	void add(std::ostream & out)
	{
		outs.push_back(&out);
	}
	void flush()
	{
		for (auto & out : outs)
			out->flush();
	}
};
template<size_t S=0>
std::unique_ptr<Log<S>> operator<<(LogWithPrefix_<S> & log, LogLevel level)
{
	return std::make_unique<Log<S>>(log.outs, LogWithPrefix_<S>::cout_mutex, level >= log.level) << utttil::UTCTimestampISO8601() << " [" << log.prefix << "] [" << LogLevelNames[(int)level] << "] ";
}

using LogWithPrefix = LogWithPrefix_<0>;

} // namespace
