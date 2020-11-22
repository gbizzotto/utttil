#pragma once

#include <iostream>
#include <chrono>

namespace utttil {

struct measurement_point
{
	std::string name;
	std::chrono::nanoseconds ns_total;
	std::uint64_t run_count;

	inline void add(std::chrono::nanoseconds d)
	{
		ns_total += d;
		run_count++;
	}

	inline measurement_point(std::string name)
		:name(name)
		,ns_total(0)
		,run_count(0)
	{}
	inline ~measurement_point()
	{
		std::cerr
			<< name << ": "
			<< run_count << " runs in " << ns_total.count() << " ns, "
			<< ((run_count!=0) ? std::to_string(ns_total.count() / run_count) : std::string("N/A")) << " ns/run."
			<< std::endl;
	}
};

struct measurement
{
	measurement_point & what;
	std::chrono::high_resolution_clock::time_point start;

	inline measurement(measurement_point & measurement_point)
		:what(measurement_point)
		,start(std::chrono::high_resolution_clock::now())
	{}
	inline ~measurement()
	{
		what.add(std::chrono::high_resolution_clock::now() - start);
	}
};

} // namespace
