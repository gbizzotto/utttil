
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
template<typename MsgIn=no_msg_t, typename MsgOut=no_msg_t>
struct udpmr_client_msg : peer_msg<MsgIn,MsgOut>
{
	using Seq = typename MsgIn::seq_type;

	udpm_client_msg<MsgIn,MsgOut> udp_client;
	tcp_socket_msg<ReplayResponse<Seq,MsgIn>,ReplayRequest<Seq>> replay_client;

	utttil::ring_buffer<MsgIn> inbox_msg;

	Seq next_seq;

	udpmr_client_msg(const utttil::url & url_udp, const utttil::url & url_tcp)
		: udp_client(url_udp)
		, replay_client(url_tcp)
		, inbox_msg(12)
		, next_seq(0)
	{}

	bool does_accept() override { return false; }
	bool does_read  () override { return true ; }
	bool does_write () override { return true ; }

	utttil::ring_buffer<MsgIn> * get_inbox_msg  () override { return &this->inbox_msg; }

	void close() override { udp_client.close(); replay_client.close(); }
	bool good() const override { return udp_client.good() & replay_client.good(); }

	int read() override
	{
		//	auto count = 
		replay_client.read();
		//	if (count > 0)
		//		replay_client.raw.print_inbox();
		return udp_client.read();
	}
	int write() override
	{
		return replay_client.write();
	}
	void pack() override
	{
		replay_client.pack();
	}
	void unpack() override
	{		
		if (inbox_msg.full())
			return;

		// TODO: unpack from udp client directly to inbox_msg
		// while checking for seq

		replay_client.unpack();
		while( ! replay_client.inbox_msg.empty())
		{
			ReplayResponse<Seq,MsgIn> & req = replay_client.inbox_msg.front();
			if (req.status != 0)
			{
				// unrecoverable gap
				udp_client.raw.good_ = false;
				close();
				break;
			}				
			if (next_seq < req.payload.get_seq())
			{
				// unrecoverable gap
				udp_client.raw.good_ = false;
				close();
				break;
			}
			else if (req.payload.get_seq() == next_seq)
			{
				++next_seq;
				inbox_msg.push_back(req.payload);
			}
			replay_client.inbox_msg.pop_front();
		}

		udp_client.unpack();
		while( ! udp_client.inbox_msg.empty())
		{
			MsgIn & msg = udp_client.inbox_msg.front();
			if (next_seq < msg.get_seq())
			{
				std::cout << "Gap: " << next_seq << " - " << msg.get_seq() << std::endl;
				ReplayRequest<Seq> & req = replay_client.get_outbox_msg()->back();
				req.req_id = 1234;
				req.begin = next_seq;
				req.end   = msg.get_seq();
				replay_client.get_outbox_msg()->advance_back(1);
				break;
			}
			else if (msg.get_seq() == next_seq)
			{
				++next_seq;
				inbox_msg.push_back(udp_client.inbox_msg.front());
			}
			udp_client.inbox_msg.pop_front();
		}
	}
};

template<typename MsgIn=no_msg_t, typename MsgOut=no_msg_t>
struct udpmr_server_msg : peer_msg<MsgIn,MsgOut>
{
	using Seq = typename MsgOut::seq_type;

	udpm_server_msg<MsgIn,MsgOut> multicast_server;
	tcp_server_msg<ReplayRequest<Seq>,ReplayResponse<Seq,MsgOut>> replay_server;
	std::deque<std::shared_ptr<tcp_socket_msg<ReplayRequest<Seq>,ReplayResponse<Seq,MsgOut>>>> replay_clients;

	utttil::ring_buffer<MsgOut> outbox_msg;

	udpmr_server_msg(const utttil::url & url_udp, const utttil::url & url_tcp)
		: multicast_server(url_udp)
		, replay_server(url_tcp)
		, outbox_msg(24) // make it allocate a fixed size in bytes using sizeof() and such?
	{}

	bool does_accept() override { return true ; }
	bool does_read  () override { return true ; }
	bool does_write () override { return true ; }

	//utttil::ring_buffer<MsgOut> * get_outbox_msg  () override { return &this->outbox_msg; }

	void close() override { multicast_server.close(); replay_server.close(); for (auto & c:replay_clients) c->close(); }
	bool good() const override { return multicast_server.good() & replay_server.good(); }

	std::shared_ptr<peer> accept() override
	{
		auto new_peer_sptr = replay_server.accept();
		if ( ! new_peer_sptr)
			return nullptr;
		replay_server.get_accept_inbox()->pop_front();
		replay_clients.push_back(std::dynamic_pointer_cast<tcp_socket_msg<ReplayRequest<Seq>,ReplayResponse<Seq,MsgOut>>>(new_peer_sptr));
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
				//std::cout << "ReplayRequest " << req.req_id << ": " << req.begin << " - " << req.end << std::endl;
				if (too_old(req.begin) || too_old(req.end))
				{
					//std::cout << "ReplayRequest " << req.req_id << ": can't be honored, too old" << std::endl;
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
					//std::cout << "ReplayRequest " << req.req_id << ": can't be honored, too new" << std::endl;
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
					//std::cout << "ReplayRequest " << req.req_id << ": ok" << std::endl;
					auto it  = get_iterator(req.begin);
					auto end = get_iterator(req.end  );
					for ( ; it!=end ; ++it)
					{
						//std::cout << "ReplayRequest " << req.req_id << ": done" << std::endl;
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
			outbox_msg.advance_front(outbox_msg.capacity()/1024);
	}

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
