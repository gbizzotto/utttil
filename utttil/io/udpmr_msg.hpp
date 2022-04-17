
#pragma once

#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <deque>

#include <utttil/ring_buffer.hpp>
#include <utttil/url.hpp>
#include <utttil/srlz.hpp>

#include <utttil/io/peer.hpp>
#include <utttil/io/udpm_msg.hpp>
#include <utttil/io/tcp_msg.hpp>

namespace utttil {
namespace io {


template<typename Seq>
struct ReplayRequest
{
	int req_id;
	Seq begin;
	Seq end;

	inline ReplayRequest() {}
	ReplayRequest(const ReplayRequest & other)
		: begin(other.begin)
		, end(other.end)
	{}
	ReplayRequest & operator=(const ReplayRequest & other)
	{
		begin      = other.begin     ;
		end        = other.end       ;
		return *this;
	}

	template<typename Serializer>
	void serialize(Serializer && s) const
	{
		s << req_id
		  << begin
		  << end
		  << std::flush;
	}
	template<typename Deserializer>
	void deserialize(Deserializer && s)
	{
		s >> req_id
		  >> begin
		  >> end
		  ;
	}
};
template<typename Seq, typename MsgOut>
struct ReplayResponse
{
	int req_id;
	int status;
	Seq available_begin;
	Seq available_end;
	MsgOut payload;

	inline ReplayResponse() {}
	ReplayResponse(const ReplayResponse & other)
		: req_id         (other.req_id)
		, status         (other.status)
		, available_begin(other.available_begin)
		, available_end  (other.available_end)
		, payload        (other.payload)
	{}
	ReplayResponse & operator=(const ReplayResponse & other)
	{
		req_id          = other.req_id;
		status          = other.status;
		available_begin = other.available_begin;
		available_end   = other.available_end;
		payload         = other.payload;
		return *this;
	}

	template<typename Serializer>
	void serialize(Serializer && s) const
	{
		s << req_id
		  << status;
		if (status == 0)
			s << payload;
		else
			s << available_begin
			  << available_end;
		s << std::flush;
	}
	template<typename Deserializer>
	void deserialize(Deserializer && s)
	{
		s >> req_id
		  >> status;
		if (status == 0)
			s >> payload;
		else
			s >> available_begin
			  >> available_end;
	}
};

// MsgIn type has to have a seq_type and a get_seq() so that the client can
// check for sequence and ask for replays
template<typename MsgIn=no_msg_t, typename MsgOut=no_msg_t, typename DataT=int>
struct udpmr_client_msg : peer_msg<MsgIn,MsgOut,DataT>
{
	using Seq = typename MsgIn::seq_type;

	inline static const int frame_size = 1500;

	struct frame
	{
		std::unique_ptr<char[]> data = std::make_unique<char[]>(frame_size);
		std::optional<Seq> first_seq;
		std::optional<Seq>  last_seq;
		size_t size;

		void reset()
		{
			first_seq.reset();
			 last_seq.reset();
			size = 0;
		}

		Seq get_first_seq()
		{
			if ( !! first_seq)
				return *first_seq;
			auto deserializer = utttil::srlz::from_binary(utttil::srlz::device::ptr_reader(&data[0], size));
			size_t msg_size;
			MsgIn msg;
			try {
				deserializer >> msg_size;
				deserializer >> msg;
				first_seq = msg.get_seq();
			} catch (utttil::srlz::device::stream_end_exception &) {
				std::cout << __LINE__ << " >> failed" << std::endl;
				return Seq(0);
			}
			return *first_seq;
		}
		Seq get_last_seq()
		{
			if ( !! last_seq)
				return *last_seq;
			auto deserializer = utttil::srlz::from_binary(utttil::srlz::device::ptr_reader(&data[0], size));
			size_t msg_size;
			MsgIn msg;
			auto checkpoint = deserializer.read;
			try {
				while (deserializer.read.size() < size)
				{
					checkpoint = deserializer.read;
					deserializer >> msg_size;
					deserializer.read.skip(msg_size);
				}
			} catch (utttil::srlz::device::stream_end_exception &) {
				std::cout << __LINE__ << " >> failed" << std::endl;
				return Seq(0);
			}
			deserializer.read = checkpoint;
			try {
				deserializer >> msg_size;
				deserializer >> msg;
				last_seq = msg.get_seq();
			} catch (utttil::srlz::device::stream_end_exception &) {
				std::cout << __LINE__ << " >> failed" << std::endl;
				return Seq(0);
			}
			return *last_seq;
		}
	};

	int fd;
	bool good_;

	Seq next_ordered_seq;
	Seq next_expected_seq;

	utttil::ring_buffer<frame> buffers;
	tcp_socket_msg<ReplayResponse<Seq,MsgIn>,ReplayRequest<Seq>,DataT> replay_client;

	utttil::ring_buffer<MsgIn> inbox_msg;
	int tcp_req_id;

	udpmr_client_msg(const utttil::url & url_udp, const utttil::url & url_tcp)
		: fd(client_socket_udpm(url_udp.host.c_str(), std::stoull(url_udp.port)))
		, good_(true)
		, next_ordered_seq(0)
		, next_expected_seq(0)
		, buffers(10)
		, replay_client(url_tcp)
		, inbox_msg(12)
		, tcp_req_id(0)
	{
		std::cout << "fd: " << fd << " url: " << url_udp.to_string() << std::endl;
	}

	bool does_accept() override { return false; }
	bool does_read  () override { return true ; }
	bool does_write () override { return true ; }

	utttil::ring_buffer<MsgIn> * get_inbox_msg  () override { return &this->inbox_msg; }

	void close() override { if (fd != -1) {::close(fd); fd=-1;} replay_client.close(); }
	bool good() const override { return this->good_ & replay_client.good(); }

	void request_replay(Seq begin, Seq end)
	{
		ReplayRequest<Seq> & req = replay_client.get_outbox_msg()->back();
		req.req_id = ++tcp_req_id;
		req.begin = begin;
		req.end   = end;
		replay_client.get_outbox_msg()->advance_back(1);
	}

	int read() override
	{
		int count = replay_client.read();
		if (count < 0 && errno != 0 && errno != EAGAIN)
		{
			// TODO: try reconnecting
			std::cout << __FILE__ << ":" << __LINE__ << " read tcp failed with errno: " << strerror(errno) << std::endl;
			close();
			this->good_ = false;
			return count;
		}

		if (buffers.full())
			return 0;
		frame & f = buffers.back();
		f.reset();
		count = ::read(this->fd, &f.data[0], frame_size);
		if (count > 0)
		{
			f.size = count;
			Seq seq = f.get_first_seq();
			if (next_expected_seq <= seq)
			{
				if (next_expected_seq < seq) {
					std::cout << "Gap: " << next_expected_seq << " - " << seq << std::endl;
					request_replay(next_expected_seq, seq);
				} else if (seq < next_expected_seq) {
					std::cout << "Frame is out of order" << std::endl;
				}
				next_expected_seq = f.get_last_seq();
				++next_expected_seq;
				buffers.advance_back(1);	
			}
		} else if (count < 0 && errno != 0 && errno != EAGAIN) {
			std::cout << this->fd << " udpmr_client_msg read() good = false because of errno: " << errno << " - " << strerror(errno) << " fd: " << this->fd << std::endl;
			this->good_ = false;
		}
		return count;
	}
	int write() override { return replay_client.write(); }
	void pack() override {        replay_client.pack (); }

	void unpack() override
	{
		if (inbox_msg.full())
			return;

		// handle TCP msgs
		replay_client.unpack();

		while( ! replay_client.inbox_msg.empty())
		{
			ReplayResponse<Seq,MsgIn> & req = replay_client.inbox_msg.front();
			if (req.status != 0)
			{
				std::cout << __FILE__ << ":" << __LINE__ << " unrecoverable gap status=0" << std::endl;
				this->good_ = false;
				close();
				break;
			}
			if (req.payload.get_seq() == next_ordered_seq)
			{
				++next_ordered_seq;
				inbox_msg.push_back(req.payload);
				replay_client.inbox_msg.pop_front();
			}
			else if (req.payload.get_seq() < next_ordered_seq)
			{
				std::cout << "dropping obsolete tcp replay msg #" << req.payload.get_seq() << std::endl;
				replay_client.inbox_msg.pop_front();
			}
			else if (next_ordered_seq < req.payload.get_seq())
			{
				break;
			}
		}

		for (auto it_buffers = buffers.begin() ; it_buffers != buffers.end() ; ++it_buffers)
		{
			frame & f = *it_buffers;
			if (f.get_first_seq() != next_ordered_seq)
				continue;

			auto msg_count = ++(f.get_last_seq() - f.get_first_seq());
			if (inbox_msg.free_size() < msg_count)
				break;
	
			auto deserializer = utttil::srlz::from_binary(utttil::srlz::device::ptr_reader(&f.data[0], f.size));
			try {
				for (auto i = msg_count ; (decltype(i))0<i ; --i)
				{
					size_t msg_size;
					deserializer >> msg_size;
					auto & msg = inbox_msg.back();
					deserializer >> msg;
					inbox_msg.advance_back(1);
				}
				next_ordered_seq += msg_count;
			} catch (utttil::srlz::device::stream_end_exception &) {
				std::cout << __FILE__ << ":" << __LINE__ << ">> failed" << std::endl;
				std::cout << "Gap (bad frame): " << next_ordered_seq << " - " << next_ordered_seq + msg_count << std::endl;
				request_replay(next_ordered_seq, next_ordered_seq + msg_count);
			}
			if (deserializer.read.size() != f.size)
				std::cout << "Extraneous data in datagram" << std::endl;
		}

		while ( ! buffers.empty())
		{
			frame & f = buffers.front();
			if (f.get_first_seq() < next_ordered_seq)
				buffers.pop_front();
			else
				break;
		}
	}
};

template<typename MsgIn=no_msg_t, typename MsgOut=no_msg_t, typename DataT=int>
struct udpmr_server_msg : peer_msg<MsgIn,MsgOut,DataT>
{
	using Seq = typename MsgOut::seq_type;

	udpm_server_msg<MsgIn,MsgOut> multicast_server;
	tcp_server_msg<ReplayRequest<Seq>,ReplayResponse<Seq,MsgOut>,DataT> replay_server;
	std::deque<std::shared_ptr<tcp_socket_msg<ReplayRequest<Seq>,ReplayResponse<Seq,MsgOut>,DataT>>> replay_clients;

	utttil::ring_buffer<MsgOut> outbox_msg;
	typename utttil::ring_buffer<MsgOut>::iterator sent_it;

	udpmr_server_msg(const utttil::url & url_udp, const utttil::url & url_tcp)
		: multicast_server(url_udp)
		, replay_server(url_tcp)
		, outbox_msg(24) // make it allocate a fixed size in bytes using sizeof() and such?
		, sent_it(outbox_msg.begin())
	{
		//std::cout << "fd: " << multicast_server.fd << " url: " << url_udp.to_string() << std::endl;
		//std::cout << "fd: " << replay_server   .fd << " url: " << url_tcp.to_string() << std::endl;
	}

	bool does_accept() override { return true ; }
	bool does_read  () override { return true ; }
	bool does_write () override { return true ; }

	utttil::ring_buffer<MsgOut> * get_outbox_msg  () override { return &this->outbox_msg; }

	void close() override { multicast_server.close(); replay_server.close(); for (auto & c:replay_clients) c->close(); }
	bool good() const override { return multicast_server.good() & replay_server.good(); }

	std::shared_ptr<peer> accept() override
	{
		auto new_peer_sptr = replay_server.accept();
		if ( ! new_peer_sptr)
			return nullptr;
		replay_server.get_accept_inbox()->pop_front();
		replay_clients.push_back(std::dynamic_pointer_cast<tcp_socket_msg<ReplayRequest<Seq>,ReplayResponse<Seq,MsgOut>,DataT>>(new_peer_sptr));
		return new_peer_sptr;
	}

	int write() override
	{
		for (int i=replay_clients.size()-1 ; i>=0 ; --i)
			replay_clients[i]->write();
		return multicast_server.write();
	}
	int read() override
	{
		int count = 0;
		for (int i=replay_clients.size()-1 ; i>=0 ; --i)
		{
			int read = replay_clients[i]->read();
			if (read > 0)
				count += read;
			else if ( ! replay_clients[i]->good())
				replay_clients.erase(replay_clients.begin() + i);
		}
		return count;
	}

	void pack() override
	{
		while(sent_it != outbox_msg.end())
			multicast_server.get_outbox_msg()->push_back(*sent_it++);
		multicast_server.pack();
		for (int i=replay_clients.size()-1 ; i>=0 ; --i)
			replay_clients[i]->pack();
	}
	void unpack() override
	{
		for (int i=replay_clients.size()-1 ; i>=0 ; --i)
		{
			replay_clients[i]->unpack();
			if ( ! replay_clients[i]->get_inbox_msg()->empty())
			{
				ReplayRequest<Seq> & req = replay_clients[i]->get_inbox_msg()->front();
				std::cout << "ReplayRequest " << req.req_id << ": " << req.begin << " - " << req.end << std::endl;
				if (too_old(req.begin) || too_old(req.end))
				{
					std::cout << "ReplayRequest " << req.req_id << ": can't be honored, too old" << std::endl;
					// can't honor request
					ReplayResponse<Seq,MsgOut> & resp = replay_clients[i]->get_outbox_msg()->back();
					resp.req_id = req.req_id;
					resp.status = 2;
					resp.available_begin  = req.end;
					resp.available_end    = req.end;
					replay_clients[i]->get_outbox_msg()->advance_back(1);
				}
				else if (too_new(req.begin) || too_new(req.end))
				{
					std::cout << "ReplayRequest " << req.req_id << ": can't be honored, too new" << std::endl;
					// can't honor request
					ReplayResponse<Seq,MsgOut> & resp = replay_clients[i]->get_outbox_msg()->back();
					resp.status = 1;
					resp.req_id = req.req_id;
					resp.available_begin  = req.begin;
					resp.available_end    = req.begin;
					replay_clients[i]->get_outbox_msg()->advance_back(1);
				}
				else
				{
					std::cout << "ReplayRequest " << req.req_id << ": ok" << std::endl;
					auto it  = get_iterator(req.begin);
					auto end = get_iterator(req.end  );
					for ( ; it!=end ; ++it)
					{
						ReplayResponse<Seq,MsgOut> & resp = replay_clients[i]->get_outbox_msg()->back();
						resp.req_id  = req.req_id;
						resp.status  = 0;
						resp.payload = *it;
						replay_clients[i]->get_outbox_msg()->advance_back(1);
					}
				}
				replay_clients[i]->get_inbox_msg()->pop_front();
			}
		}
		if (outbox_msg.free_size() < outbox_msg.capacity()/1024) // totally arbitrary number
			if (outbox_msg.begin() + outbox_msg.capacity()/1024 <= sent_it)
				outbox_msg.advance_front(outbox_msg.capacity()/1024);
	}

	/*
	virtual void async_send(const MsgOut & msg)
	{
		outbox_msg.push_back(msg);
		multicast_server.async_send(msg);
	}
	virtual void async_send(MsgOut && msg)
	{
		outbox_msg.push_back(msg);	
		multicast_server.async_send(std::move(msg));
	}
	*/

	bool too_old(Seq seq)
	{
		return outbox_msg.empty() || seq < outbox_msg.front().get_seq();
	}
	bool too_new(Seq seq)
	{
		return outbox_msg.empty() || (outbox_msg.begin()+(outbox_msg.size()-1))->get_seq() < seq;
	}
	auto get_iterator(Seq seq)
	{
		return outbox_msg.begin() + (seq - outbox_msg.front().get_seq()).value();
	}
};

}} // namespace
