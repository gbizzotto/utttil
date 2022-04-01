
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

namespace utttil {
namespace io {


template<typename MsgIn, typename MsgOut>
struct tcp_socket_msg : peer_msg<MsgIn,MsgOut>
{
	tcp_socket_raw raw;

	utttil::ring_buffer<MsgOut> outbox_msg;
	utttil::ring_buffer<MsgIn >  inbox_msg;

	tcp_socket_msg(int fd)
		: raw(fd)
		, outbox_msg(peer_msg<MsgIn,MsgOut>::outbox_msg_capacity_bits)
		, inbox_msg (peer_msg<MsgIn,MsgOut>:: inbox_msg_capacity_bits)
	{}
	tcp_socket_msg(const utttil::url & url)
		: raw(url)
		, outbox_msg(peer_msg<MsgIn,MsgOut>::outbox_msg_capacity_bits)
		, inbox_msg (peer_msg<MsgIn,MsgOut>:: inbox_msg_capacity_bits)
	{}
	tcp_socket_msg(tcp_socket_raw && other)
		: raw(std::move(other))
		, outbox_msg(peer_msg<MsgIn,MsgOut>::outbox_msg_capacity_bits)
		, inbox_msg (peer_msg<MsgIn,MsgOut>:: inbox_msg_capacity_bits)
	{}

	bool does_accept() override { return false; }
	bool does_read  () override { return true ; }
	bool does_write () override { return true ; }

	void close() override { return raw.close(); }
	bool good() const override { return raw.good(); }

	utttil::ring_buffer<MsgOut> * get_outbox_msg  () override { return &outbox_msg; }
	utttil::ring_buffer<MsgIn > * get_inbox_msg   () override { return & inbox_msg; }

	size_t write() override
	{
		return raw.write();
	}
	size_t read() override
	{
		return raw.read();
	}

	void pack() override
	{
		auto & outbox = *raw.get_outbox();

		while ( ! outbox_msg.empty())
		{
			MsgOut & msg = outbox_msg.front();

			auto size_preview_serializer = utttil::srlz::to_binary(utttil::srlz::device::null_writer());
			size_preview_serializer << msg;
			size_t msg_size = size_preview_serializer.write.size();
			size_preview_serializer << msg_size; // add size field
			size_t total_size = size_preview_serializer.write.size();

			if (outbox.free_size() < total_size) {
				return;
			}
			try {
				auto stretch1 = outbox.back_stretch();
				auto stretch2 = outbox.back_stretch_2();
				auto s = utttil::srlz::to_binary(utttil::srlz::device::ring_buffer_writer(stretch1, stretch2, total_size));
				s << msg_size;
				s << msg;
				outbox.advance_back(s.write.size());
				outbox_msg.pop_front();
			} catch (utttil::srlz::device::stream_end_exception &) {
				std::cout << "send() stream_end_exception" << std::endl;
				return;
			}
		}
	}
	void unpack() override
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
			//assert(msg_size == 45);
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
			assert(deserializer.read.size() == total_size);
			inbox_msg.advance_back(1);
			inbox.advance_front(deserializer.read.size());
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

template<typename MsgIn=no_msg_t, typename MsgOut=no_msg_t>
struct tcp_server_msg : peer_msg<MsgIn,MsgOut>
{
	tcp_server_raw raw;

	utttil::ring_buffer<std::shared_ptr<peer_msg<MsgIn,MsgOut>>> accept_inbox;

	tcp_server_msg(const utttil::url & url)
		: raw(url)
		, accept_inbox(peer_msg<MsgIn,MsgOut>::accept_inbox_capacity_bits)
	{}

	bool does_accept() override { return true ; }
	bool does_read  () override { return false; }
	bool does_write () override { return false; }

	void close() override { return raw.close(); }
	bool good() const override { return raw.good(); }

	utttil::ring_buffer<std::shared_ptr<peer_msg<MsgIn,MsgOut>>> * get_accept_inbox() override { return &accept_inbox; }

	std::shared_ptr<peer_msg<MsgIn,MsgOut>> accept() override
	{
		std::shared_ptr<peer_raw> new_raw_peer = raw.accept();
		if ( ! new_raw_peer)
			return nullptr;
		auto new_tcp_socket = std::dynamic_pointer_cast<tcp_socket_raw>(new_raw_peer);
		raw.accept_inbox.pop_front();
		auto new_peer_sptr = std::make_shared<tcp_socket_msg<MsgIn,MsgOut>>(std::move(*new_tcp_socket));
		accept_inbox.push_back(new_peer_sptr);
		return new_peer_sptr;
	}
};

}} // namespace
