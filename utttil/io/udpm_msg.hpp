
#pragma once

#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include <utttil/ring_buffer.hpp>
#include <utttil/url.hpp>
#include <utttil/srlz.hpp>

#include <utttil/io/peer.hpp>
#include <utttil/io/udpm_raw.hpp>
#include <utttil/assert.hpp>

namespace utttil {
namespace io {


template<typename MsgIn=no_msg_t, typename MsgOut=no_msg_t, typename DataT=int>
struct udpm_client_msg : peer_msg<MsgIn,MsgOut,DataT>
{
	udpm_client_raw<> raw;

	utttil::ring_buffer<MsgIn> inbox_msg;

	udpm_client_msg(const utttil::url & url)
		: raw(url)
		, inbox_msg(peer_msg<MsgIn,MsgOut,DataT>::inbox_msg_capacity_bits)
	{}

	bool does_accept() override { return false; }
	bool does_read  () override { return true ; }
	bool does_write () override { return false; }

	void close() override { return raw.close(); }
	bool good() const override { return raw.good(); }

	utttil::ring_buffer<MsgIn> * get_inbox_msg() override { return &inbox_msg; }

	void unpack()
	{
		auto & inbox = *raw.get_inbox();

		while ( ! inbox_msg.full())
		{
			auto deserializer = utttil::srlz::from_binary(utttil::srlz::device::ring_buffer_reader(inbox, inbox.size()));
			size_t msg_size;
			size_t total_size;
			try {
				deserializer >> msg_size;
			} catch (utttil::srlz::device::stream_end_exception &) {
				break;
			}
			total_size = msg_size + deserializer.read.size();
			if (total_size > inbox.size()) {
				break;
			}
			MsgIn & msg = inbox_msg.back();
			try {
				deserializer >> msg;
			} catch (utttil::srlz::device::stream_end_exception &) {
				std::cout << ">> failed" << std::endl;
				break;
			}
			ASSERT_ACT(deserializer.read.size(), ==, total_size, std::cout << msg << std::endl);
			inbox_msg.advance_back(1);
			inbox.advance_front(deserializer.read.size());
		}
	}

	int read() override
	{
		return raw.read();
	}
};

template<typename MsgIn=no_msg_t, typename MsgOut=no_msg_t, typename DataT=int>
struct udpm_server_msg : peer_msg<MsgIn,MsgOut,DataT>
{
	int fd;
	bool good_;

	// writer
	sockaddr_in sendto_addr;
	sockaddr_in *sendto_addr_ptr;
	size_t sendto_addr_len;

	char   send_buffer[1400];
	int    send_size;
	utttil::ring_buffer<MsgOut> outbox_msg;

	udpm_server_msg(const utttil::url & url)
		: peer_msg<MsgIn,MsgOut,DataT>()
		, fd(server_socket_udpm())
		, good_(true)
		, sendto_addr_ptr(nullptr)
		, sendto_addr_len(0)
		, send_size(0)
		, outbox_msg(peer_msg<MsgIn,MsgOut,DataT>::outbox_msg_capacity_bits)
	{
	    memset(&sendto_addr, 0, sizeof(sendto_addr));
	    sendto_addr.sin_family = AF_INET;
	    sendto_addr.sin_addr.s_addr = inet_addr(url.host.c_str());
	    sendto_addr.sin_port = htons(std::stol(url.port));
	    sendto_addr_ptr = &sendto_addr;
	    sendto_addr_len = sizeof(sendto_addr);
	}

	bool does_accept() override { return false; }
	bool does_read  () override { return false; }
	bool does_write () override { return true ; }

	void close() override
	{
		if (fd != -1)
		{
			::close(fd);
			fd = -1;
		}
	}
	bool good() const override { return (fd != -1) & good_; }

	utttil::ring_buffer<MsgOut> * get_outbox_msg() override { return &outbox_msg; }

	int write() override
	{
		if (send_size == 0)
			return 0;
		int count = ::sendto(this->fd, send_buffer, send_size, 0, (sockaddr*)sendto_addr_ptr, sendto_addr_len);
		if (count > 0) {
			assert(count == send_size);
			send_size = 0;
		} else if (count < 0 && errno != 0 && errno != EAGAIN) {
			std::cout << "sendto() good = false because of errno: " << errno << " - " << strerror(errno) << std::endl;
			this->good_ = false;
		}
		return count;
	}

	void pack() override
	{
		if (send_size != 0)
			return;
		while ( ! outbox_msg.empty())
		{
			MsgOut & msg = outbox_msg.front();

			auto size_preview_serializer = utttil::srlz::to_binary(utttil::srlz::device::null_writer());
			size_preview_serializer << msg;
			size_t msg_size = size_preview_serializer.write.size();
			size_preview_serializer << msg_size; // add size field
			size_t total_size = size_preview_serializer.write.size();

			if (sizeof(send_buffer)-send_size < total_size) {
				return;
			}
			try {
				auto s = utttil::srlz::to_binary(utttil::srlz::device::ptr_writer(&send_buffer[send_size]));
				s << msg_size;
				s << msg;
				send_size += s.write.size();
				outbox_msg.pop_front();
			} catch (utttil::srlz::device::stream_end_exception &) {
				std::cout << "send() stream_end_exception" << std::endl;
				return;
			}
		}
	}

	void async_send(const MsgOut & msg) override
	{
		outbox_msg.push_back(msg);
	}
	void async_send(MsgOut && msg) override
	{
		outbox_msg.push_back(std::move(msg));
	}
};

}} // namespace
