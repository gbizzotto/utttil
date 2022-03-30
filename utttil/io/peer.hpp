
#pragma once

#include <memory>
#include <string>

#include <utttil/ring_buffer.hpp>
#include <utttil/url.hpp>
#include <utttil/srlz.hpp>

namespace utttil {
namespace io {

struct no_msg_t {};

template<typename MsgIn=no_msg_t, typename MsgOut=no_msg_t>
struct peer
{
	int fd;
	bool good_;

	inline static constexpr size_t accept_inbox_capacity_bits =  8;
	inline static constexpr size_t       outbox_capacity_bits = 16;
	inline static constexpr size_t   outbox_msg_capacity_bits = 10;
	inline static constexpr size_t        inbox_capacity_bits = 16;
	inline static constexpr size_t    inbox_msg_capacity_bits = 10;

	peer(int fd_)
		: fd(fd_)
		, good_(true)
	{}

	virtual bool does_accept() { return false; }
	virtual bool does_read  () { return false; }
	virtual bool does_write () { return false; }

	virtual void close()
	{
		if (fd != -1)
		{
			::close(fd);
			fd = -1;
		}
	}
	virtual bool good() const { return (fd != -1) & good_; }

	virtual std::shared_ptr<peer<MsgIn,MsgOut>> accept() { assert(false); return nullptr; };
	virtual size_t write()                               { assert(false); return 0; }
	virtual size_t read ()                               { assert(false); return 0; }
	virtual void   pack()                                { assert(false); }
	virtual void unpack()                                { assert(false); }
	virtual void async_write(const char*, size_t)        { assert(false); }
	virtual void async_send(const MsgOut  &)             { assert(false); }
	virtual void async_send(      MsgOut &&)             { assert(false); }

	virtual utttil::ring_buffer<std::shared_ptr<peer<MsgIn,MsgOut>>, peer<MsgIn,MsgOut>::accept_inbox_capacity_bits> * get_accept_inbox() { return nullptr; }
	virtual utttil::ring_buffer<char                               , peer<MsgIn,MsgOut>::      outbox_capacity_bits> * get_outbox      () { return nullptr; }
	virtual utttil::ring_buffer<MsgOut                             , peer<MsgIn,MsgOut>::  outbox_msg_capacity_bits> * get_outbox_msg  () { return nullptr; }
	virtual utttil::ring_buffer<char                               , peer<MsgIn,MsgOut>::       inbox_capacity_bits> * get_inbox       () { return nullptr; }
	virtual utttil::ring_buffer<MsgIn                              , peer<MsgIn,MsgOut>::   inbox_msg_capacity_bits> * get_inbox_msg   () { return nullptr; }
};

}} // namespace
