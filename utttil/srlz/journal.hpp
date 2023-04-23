
#pragma once

#include <string>
#include <memory>
#include <filesystem>

#include <utttil/mapped_file.hpp>
#include <utttil/srlz.hpp>

namespace utttil {
namespace srlz {

struct journal_write_file
{
	mmapped_file file;
	utttil::srlz::to_binary<device::mmap_writer> writer;
	//utttil::srlz::from_binary<device::mmap_reader> reader;

	journal_write_file(const char * filename, size_t max_size)
		: file(filename, max_size)
		, writer(device::mmap_writer(nullptr))
		//, reader(device::mmap_reader((char*)file.mapping + 4))
	{
		if ( ! file.good())
			return;
		_adjust_for_previous_data();
	}
	~journal_write_file()
	{
		_close();
	}

	template<typename T>
	void write(const T & t)
	{
		writer << t;

		// write 4-bytes little-endian size at beginning of the file
		*(uint32_t*)file.mapping = (uint32_t) size();
	}

	bool reset(const std::string & filename, size_t max_size)
	{
		_close();

		file.reset(filename.c_str(), max_size);
		if (file.good())
			_adjust_for_previous_data();
		return file.good();
	}

	void _close()
	{
		if ( ! file.good())
			return;
		file.async((char*)writer.write.begin_ptr, writer.write.size());
		file.async((char*)file.mapping, 4);
		file.unmap();
	}

	void _adjust_for_previous_data()
	{
		// read 4-bytes little-endian size
		uint32_t size = *(uint32_t*)file.mapping;
		writer = device::mmap_writer((char*)file.mapping + 4 + size);
		//reader = device::mmap_reader((char*)file.mapping + 4);
	}

	size_t size() const
	{
		return writer.write.c - (char*)file.mapping - 4;
	}
	size_t free_size() const
	{
		return file.max_size - size() - 4;
	}

	template<typename T>
	bool fits(const T & t)
	{
		return free_size() >= utttil::srlz::serialize_size(t);
	}
};

struct journal_write
{
	const std::string path;
	const size_t max_file_size;
	int next_file_id;
	journal_write_file file;

	journal_write(std::string path_, size_t max_file_size_)
		: path(path_ + (path_.empty() ? "./" : (path_.back() == '/' ? "" : "/")))
		, max_file_size(max_file_size_)
		, next_file_id(_find_last_file_id())
		, file(get_next_file_name().c_str(), max_file_size_)
	{}

	std::string get_next_file_name()
	{
		return path + std::to_string(next_file_id++);
	}
	std::string peek_next_file_name()
	{
		return path + std::to_string(next_file_id);
	}

	int _find_last_file_id()
	{
		next_file_id = 1;
		if ( ! std::filesystem::exists(peek_next_file_name()))
			return next_file_id; // path / 1

		next_file_id = 2;
		while (std::filesystem::exists(peek_next_file_name()))
			next_file_id++; // path / last_existing_file

		return next_file_id;
	}

	template<typename T>
	void write(const T & t)
	{
		if ( ! file.fits(t))
			file.reset(get_next_file_name(), max_file_size);
		file.write(t);
	}
};









struct journal_read_file
{
	mmapped_file file;
	//utttil::srlz::  to_binary<device::mmap_writer> writer;
	utttil::srlz::from_binary<device::mmap_reader> reader;

	journal_read_file(const char * filename, size_t max_size)
		: file(filename, max_size)
		//, writer(device::mmap_writer(nullptr))
		, reader(device::mmap_reader((char*)file.mapping + 4))
	{
		if ( ! file.good())
			return;
		_adjust_for_previous_data();
	}
	~journal_read_file()
	{
		_close();
	}

	template<typename T>
	T read()
	{
		T t;
		reader >> t;
		return t;
	}

	bool reset(const std::string & filename, size_t max_size)
	{
		_close();

		file.reset(filename.c_str(), max_size);
		if (file.good())
			_adjust_for_previous_data();
		return file.good();
	}

	void _close()
	{
		if ( ! file.good())
			return;
		file.unmap();
	}

	void _adjust_for_previous_data()
	{
		//writer = device::mmap_writer((char*)file.mapping + 4 + size);
		reader = device::mmap_reader((char*)file.mapping + 4);
	}

	size_t size() const
	{
		return reader.read.c - (char*)file.mapping - 4;
	}
	size_t free_size() const
	{
		return file.max_size - size() - 4;
	}

	bool eof() const
	{
		return size() == (size_t)*(int*)file.mapping;
	}
};

struct journal_read
{
	const std::string path;
	const size_t max_file_size;
	int next_file_id;
	journal_read_file file;

	journal_read(std::string path_, size_t max_file_size_)
		: path(path_ + (path_.empty() ? "./" : (path_.back() == '/' ? "" : "/")))
		, max_file_size(max_file_size_)
		, next_file_id(1)
		, file(get_next_file_name().c_str(), max_file_size_)
	{}

	std::string get_next_file_name()
	{
		return path + std::to_string(next_file_id++);
	}
	std::string peek_next_file_name()
	{
		return path + std::to_string(next_file_id);
	}

	// end of journal
	bool eoj()
	{
		if ( ! file.eof())
			return false;
		auto next_file_name = get_next_file_name();
		if ( ! std::filesystem::exists(next_file_name))
			return true;
		file.reset(next_file_name, max_file_size);
		return false;
	}

	template<typename T>
	T read()
	{
		while (file.eof())
		{
			auto next_file_name = get_next_file_name();
			if ( ! std::filesystem::exists(next_file_name))
				return T();
			file.reset(next_file_name, max_file_size);
		}
		return file.read<T>();
	}
};
}} // namespace
