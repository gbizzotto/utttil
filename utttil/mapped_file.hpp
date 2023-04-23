
#pragma once

#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string>

namespace utttil {

inline long FdGetFileSize(int fd)
{
	struct stat stat_buf;
	int rc = fstat(fd, &stat_buf);
	return rc == 0 ? stat_buf.st_size : -1;
}

struct mmapped_file_read
{
	int fd;
	std::string filename;
	void * mapping = nullptr;
	size_t size;
			
	mmapped_file_read(const char * filename)
	{
		reset(filename);
	}
	bool reset(const char * filename)
	{
		fd = ::open(filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
		if (fd == -1)
			return false;
		this->size = FdGetFileSize(fd);

		auto flag = PROT_READ;
		mapping = mmap(0, size, flag, MAP_SHARED, fd, 0);

		//if (mapping == MAP_FAILED || mapping == nullptr)
			::close(fd);
		this->filename = filename;
		return true;
	}
	~mmapped_file_read()
	{
		if (mapping != nullptr)
			munmap(mapping, size);
		::close(fd);
	}

	bool good() const
	{
		return mapping != nullptr;
	}
};

struct mmapped_file
{
	int fd;
	std::string filename;
	void * mapping = nullptr;
	size_t max_size;
	size_t size_when_opening;
			
	mmapped_file(const char * filename, size_t max_size_)
		: max_size(max_size_)
	{
		reset(filename, max_size);
	}
	~mmapped_file()
	{
		unmap();
	}

	bool good() const
	{
		return mapping != nullptr;
	}
	void sync(char * beginning, size_t size)
	{
		if (mapping)
			::msync(beginning, size, MS_SYNC);
	}
	void async(char * beginning, size_t size)
	{
		if (mapping)
			::msync(beginning, size, MS_ASYNC);
	}
	void flush()
	{
		sync((char*)mapping, max_size);
	}

	void unmap()
	{
		if (mapping != nullptr)
		{
			munmap(mapping, max_size);
			mapping = nullptr;
		}
	}
	void unmap(void * beginning, size_t size)
	{
		if (mapping != nullptr)
		{
			munmap(beginning, size);
			mapping = nullptr;
		} 
	}

	bool reset(const char * filename, size_t max_size_)
	{
		if (mapping)
			munmap(mapping, max_size_);

		fd = ::open(filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
		if (fd == -1)
			return false;

		size_when_opening = FdGetFileSize(fd);

		if (size_when_opening < max_size_)
		{
	#ifdef __linux__
			if (fallocate(fd, 0, 0, max_size_) == -1)
	#else
			if (ftruncate(fd, max_size_) == -1)
	#endif
			{
				::close(fd);
				return false;
			}
		}

		auto flag = PROT_READ | PROT_WRITE;
		mapping = mmap(0, max_size, flag, MAP_SHARED, fd, 0);

		// always close file descriptor. mem mapping does not depend on it
		::close(fd);

		if (mapping == MAP_FAILED || mapping == nullptr)
		{
			mapping = nullptr;
			this->filename.clear();
			max_size = 0;
			size_when_opening = 0;
			return false;
		}
		else
		{
			this->filename = filename;
			max_size = max_size_;
			return true;
		}
	}
};

} // namespace
