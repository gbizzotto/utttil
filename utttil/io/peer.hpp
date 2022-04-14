
#pragma once

#include <memory>
#include <string>

#include <utttil/ring_buffer.hpp>
#include <utttil/url.hpp>
#include <utttil/srlz.hpp>

namespace utttil {
namespace io {

struct peer
{
	virtual inline bool does_accept() { return false; }
	virtual inline bool does_read  () { return false; }
	virtual inline bool does_write () { return false; }

	virtual void close() = 0;
	virtual bool good() const = 0;

	virtual inline std::shared_ptr<peer> accept() { assert(false); return nullptr; };
	virtual inline int write()                    { assert(false); return 0; }
	virtual inline int read ()                    { assert(false); return 0; }
	virtual inline void   pack()                  { assert(false); }
	virtual inline void unpack()                  { assert(false); }
};

template<typename DataT>
struct peer_data : peer
{
	DataT data;
};

template<typename DataT=int>
struct peer_raw : peer_data<DataT>
{
	int fd;
	bool good_;

	inline static constexpr size_t accept_inbox_capacity_bits =  8;
	inline static constexpr size_t       outbox_capacity_bits = 16;
	inline static constexpr size_t        inbox_capacity_bits = 16;

	inline peer_raw(int fd_)
		: fd(fd_)
		, good_(true)
	{}
	inline peer_raw(const peer_raw & other)
		: fd(other.fd)
		, good_(other.good_)
	{}
	inline peer_raw(peer_raw && other)
		: fd(other.fd)
		, good_(other.good_)
	{
		other.fd = -1;
		other.good_ = true;
	}

	inline void close() override
	{
		if (fd != -1)
		{
			::close(fd);
			fd = -1;
		}
	}
	inline bool good() const override { return (fd != -1) & good_; }

	void   pack() override {}
	void unpack() override {}

	virtual inline void async_write(const char*, size_t) { assert(false); }

	virtual inline utttil::ring_buffer<std::shared_ptr<peer_raw<DataT>>> * get_accept_inbox() { return nullptr; }
	virtual inline utttil::ring_buffer<char                            > * get_outbox      () { return nullptr; }
	virtual inline utttil::ring_buffer<char                            > * get_inbox       () { return nullptr; }
};

struct no_msg_t {};

template<typename MsgIn=no_msg_t, typename MsgOut=no_msg_t, typename DataT=int>
struct peer_msg : peer_data<DataT>
{
	inline static constexpr size_t accept_inbox_capacity_bits =  8;
	inline static constexpr size_t   outbox_msg_capacity_bits = 10;
	inline static constexpr size_t    inbox_msg_capacity_bits = 10;

	virtual void async_send(const MsgOut  &) { assert(false); }
	virtual void async_send(      MsgOut &&) { assert(false); }

	virtual utttil::ring_buffer<std::shared_ptr<peer_msg<MsgIn,MsgOut,DataT>>> * get_accept_inbox() { return nullptr; }
	virtual utttil::ring_buffer<MsgOut                                       > * get_outbox_msg  () { return nullptr; }
	virtual utttil::ring_buffer<MsgIn                                        > * get_inbox_msg   () { return nullptr; }
};

}} // namespace
