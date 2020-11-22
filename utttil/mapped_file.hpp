
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
		fd = ::open(filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
		if (fd == -1)
			return;
		this->size = FdGetFileSize(fd);

		auto flag = PROT_READ;
		mapping = mmap(0, size, flag, MAP_SHARED, fd, 0);

		if (mapping == MAP_FAILED || mapping == nullptr)
			::close(fd);
		this->filename = filename;
	}

	~mmapped_file_read()
	{
		if (mapping != nullptr)
			munmap(mapping, size);
		::close(fd);
	}
};

struct mmapped_file_write
{
	int fd;
	std::string filename;
	void * mapping = nullptr;
	const size_t size;
			
	mmapped_file_write(const char * filename, size_t size)
		: size(size)
	{
		fd = ::open(filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
		if (fd == -1)
			return;
#ifdef __linux__
		if (fallocate(fd, 0, 0, size) == -1)
#else
    	if (ftruncate(fd, size) == -1)
#endif
		{
			::close(fd);
			return;
		}

		auto flag = PROT_READ | PROT_WRITE;
		mapping = mmap(0, size, flag, MAP_SHARED, fd, 0);

		if (mapping == MAP_FAILED || mapping == nullptr)
			::close(fd);
		this->filename = filename;
	}

	~mmapped_file_write()
	{
		if (mapping != nullptr)
			munmap(mapping, size);
		::close(fd);
	}
};

} // namespace
