
#include <iostream>
#include <memory>
#include <string>
#include <atomic>

#include <utttil/io.hpp>
#include <utttil/perf.hpp>

int ok()
{
	int fd;
	fd = ::socket(PF_INET, SOCK_STREAM, 0);
	if (fd == -1) {
		std::cerr << "socket() " << ::strerror(errno) << std::endl;
		return -1;
	}
	int enable = 1;
	if (::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
		std::cerr << "setsockopt(SO_REUSEADDR) " << ::strerror(errno) << std::endl;
		return -1;
	}
	sockaddr_in srv_addr;
	::memset(&srv_addr, 0, sizeof(srv_addr));
	srv_addr.sin_family = AF_INET;
	srv_addr.sin_port = ::htons(1234);
	srv_addr.sin_addr.s_addr = ::inet_addr("127.0.0.1");
	if (::connect(fd, (const sockaddr *)&srv_addr, sizeof(srv_addr)) < 0) {
		std::cerr << "connect() " << strerror(errno) << std::endl;;
		return -1;
	}

	std::cout << "Start client" << std::endl;

	auto time_start = std::chrono::high_resolution_clock::now();
	utttil::measurement_point mp("send()");
	size_t count_bytes = 0;
	const std::string sent_by_client = "32jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h4032jk1hkjh1k3j4h62kj345h6345kljh345kj7h40";
	for ( auto deadline = std::chrono::steady_clock::now()+std::chrono::seconds(10)
		; std::chrono::steady_clock::now() < deadline
		; )
	{
		utttil::measurement m(mp);
		int count = ::send(fd, sent_by_client.data(), sent_by_client.size(), 0);
		if (count != -1)
			count_bytes += count;
		else
			break;
	}
	auto time_stop = std::chrono::high_resolution_clock::now();
	auto elapsed = time_stop - time_start;
	std::cout << count_bytes*1000000000 / elapsed.count() << " B / s" << std::endl;
	return 0;
}


int main()
{
	ok();
	return 0;





	const std::string sent_by_client = "32jk1hkjh1k3j4h62kj345h6345kljh345kj7h40";

	utttil::io::context ctx;
	ctx.start_accept();
	ctx.start_read();
	ctx.start_write();
	
	std::cout << "context running" << std::endl;

	// client
	auto client_sptr = ctx.connect(utttil::url("tcp://127.0.0.1:1234"));
	if ( ! client_sptr)
	{
		std::cerr << "Couldnt connect" << std::endl;
		return 1;
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	std::cout << "connect done" << std::endl;

	auto time_start = std::chrono::high_resolution_clock::now();
	size_t msg_count = 0;
	size_t written = 0;
	for ( auto deadline = std::chrono::steady_clock::now()+std::chrono::seconds(10)
		; std::chrono::steady_clock::now() < deadline && client_sptr->fd != -1
		; )
	{
		if (client_sptr->outbox.free_size() < sent_by_client.size())
		{
			_mm_pause();
			continue;
		}
		client_sptr->async_write(sent_by_client.data(), sent_by_client.size());
		written += sent_by_client.size();
		msg_count++;
		//if (written > sent_by_client.size())
		//	std::cout << written << std::endl;
	}
	auto time_stop = std::chrono::high_resolution_clock::now();
	auto elapsed = time_stop - time_start;
	std::cout << msg_count*1000000000 / elapsed.count() << " msg / s" << std::endl;
	std::cout << written*1000000000 / elapsed.count() << " B / s" << std::endl;

	ctx.stop();

	return 0;

}
