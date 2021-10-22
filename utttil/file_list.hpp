
#pragma once

#include <string>
#include <vector>
#include <algorithm>
#include <filesystem>

namespace utttil {

struct file_list : std::vector<std::string>
{
	std::string path;
	std::string current_file;

	inline file_list(const std::string & path)
		: path(path)
	{
		list();
	}
	inline void list()
	{
		this->clear();
		for(const auto & p: std::filesystem::directory_iterator(path))
			this->push_back(p.path().string());
		std::sort(this->begin(), this->end());
		current_file.clear();
	}
	inline std::string next()
	{
		if (current_file == this->back())
			list();
		if (this->empty())
			return {};
		if (current_file.empty()) {
			current_file = this->front();
			return current_file;
		}
		return *std::next(std::find(this->begin(), this->end(), current_file));
	}
};

} // namespace
