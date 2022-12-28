
#pragma once

#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include <vector>
#include <memory>
#include <thread>

#include <utttil/ring_buffer.hpp>
#include <utttil/url.hpp>

#include <utttil/io/tcp_raw.hpp>
#include <utttil/io/tcp_msg.hpp>
#include <utttil/io/udpm_raw.hpp>
#include <utttil/io/udpm_msg.hpp>
#include <utttil/io/udpmr_msg.hpp>

namespace utttil {
namespace io {


template<class T, class R = void>  
struct enable_if_type { typedef R type; };

template<class T, class Enable = void>
struct has_seq : std::false_type {};

template<class T>
struct has_seq<T, typename T::seq_type> : std::true_type
{};


struct context
{
	std::thread ta;
	std::thread tr;
	std::thread tw;
	bool go_on = true;
	utttil::ring_buffer<std::shared_ptr<peer>> new_accept_peers;
	utttil::ring_buffer<std::shared_ptr<peer>> new_read_peers;
	utttil::ring_buffer<std::shared_ptr<peer>> new_write_peers;
	size_t next_id = 1;

	context()
		: new_accept_peers(8)
		, new_read_peers  (8)
		, new_write_peers (8)
	{}
	context(context&&)=default;

	~context()
	{
		stop();
	}

	void run()
	{
		start_accept();
		start_read  ();
		start_write ();
	}

	template<typename Callback>
	void start_read  (Callback c) { tr = std::thread([&](){ this->loop_read(c); }); }

	void start_all   () { ta = std::thread([&](){ this->loop_all   (); }); }
	void start_accept() { ta = std::thread([&](){ this->loop_accept(); }); }
	void start_read  () { tr = std::thread([&](){ this->loop_read  (); }); }
	void start_write () { tw = std::thread([&](){ this->loop_write (); }); }
	void stop()
	{
		go_on = false;
		if (ta.joinable())
			ta.join();
		if (tr.joinable())
			tr.join();
		if (tw.joinable())
			tw.join();
	}

	void add(std::shared_ptr<peer> peer_sptr)
	{
		if (peer_sptr->does_accept())
			new_accept_peers.push_back(peer_sptr);
		if (peer_sptr->does_read())
			new_read_peers.push_back(peer_sptr);
		if (peer_sptr->does_write())
			new_write_peers.push_back(peer_sptr);
	}

	template<typename DataT=int>
	std::shared_ptr<peer_raw<DataT>> bind_raw(const utttil::url url)
	{
		std::shared_ptr<peer_raw<DataT>> peer_sptr;
		if (url.protocol == "tcp")
			peer_sptr = std::make_shared<tcp_server_raw<DataT>>(url);
		else if (url.protocol == "udpm")
			peer_sptr = std::make_shared<udpm_server_raw<DataT>>(url);
		if ( ! peer_sptr || ! peer_sptr->good())
		{
			std::cout << "bind failed: " << strerror(errno) << std::endl;
			return nullptr;
		}
		std::cout << "bound " << url.to_string() << std::endl;
		add(peer_sptr);
		return peer_sptr;
	}
	template<typename MsgIn=no_msg_t, typename MsgOut=no_msg_t, typename DataT=int>
	std::shared_ptr<peer_msg<MsgIn,MsgOut,DataT>> bind_msg(const utttil::url url, const utttil::url url_replay)
	{
		std::shared_ptr<peer_msg<MsgIn,MsgOut,DataT>> peer_sptr;
		//if (url.protocol == "tcp")
		//	peer_sptr = std::make_shared<tcp_server_msg<MsgIn,MsgOut,DataT>>(url);
		//else if (url.protocol == "udpm")
		//	peer_sptr = std::make_shared<udpm_server_msg<MsgIn,MsgOut,DataT>>(url);
		//else
		if (url.protocol == "udpmr")
			peer_sptr = std::make_shared<udpmr_server_msg<MsgIn,MsgOut,DataT>>(url, url_replay);
		if ( ! peer_sptr || ! peer_sptr->good())
		{
			std::cout << "bind failed: " << strerror(errno) << std::endl;
			return nullptr;
		}
		std::cout << "bound " << url.to_string() << std::endl;
		add(peer_sptr);
		return peer_sptr;
	}
	template<typename MsgIn=no_msg_t, typename MsgOut=no_msg_t, typename DataT=int>
	std::shared_ptr<peer_msg<MsgIn,MsgOut,DataT>> bind_msg(const utttil::url url)
	{
		std::shared_ptr<peer_msg<MsgIn,MsgOut,DataT>> peer_sptr;
		if (url.protocol == "tcp")
			peer_sptr = std::make_shared<tcp_server_msg<MsgIn,MsgOut,DataT>>(url);
		else if (url.protocol == "udpm")
			peer_sptr = std::make_shared<udpm_server_msg<MsgIn,MsgOut,DataT>>(url);
		if ( ! peer_sptr || ! peer_sptr->good())
		{
			std::cout << "bind failed: " << strerror(errno) << std::endl;
			return nullptr;
		}
		std::cout << "bound " << url.to_string() << std::endl;
		add(peer_sptr);
		return peer_sptr;
	}

	template<typename DataT=int>
	std::shared_ptr<peer_raw<DataT>> connect_raw(const utttil::url url)
	{
		std::shared_ptr<peer_raw<DataT>> peer_sptr;
		if (url.protocol == "tcp")
			peer_sptr = std::make_shared<tcp_socket_raw<DataT>>(url);
		else if (url.protocol == "udpm")
			peer_sptr = std::make_shared<udpm_client_raw<DataT>>(url);
		if ( ! peer_sptr || ! peer_sptr->good())
		{
			std::cout << "connect failed to " << url.to_string() << std::endl;
			return nullptr;
		}
		std::cout << "connected " << url.to_string() << std::endl;
		add(peer_sptr);
		return peer_sptr;
	}
	template<typename MsgIn=no_msg_t, typename MsgOut=no_msg_t, typename DataT=int>
	std::shared_ptr<peer_msg<MsgIn,MsgOut,DataT>> connect_msg(const utttil::url url, const utttil::url url_replay)
	{
		std::shared_ptr<peer_msg<MsgIn,MsgOut,DataT>> peer_sptr;
		//if (url.protocol == "tcp")
		//	peer_sptr = std::make_shared<tcp_socket_msg<MsgIn,MsgOut,DataT>>(url);
		//else if (url.protocol == "udpm")
		//	peer_sptr = std::make_shared<udpm_client_msg<MsgIn,MsgOut,DataT>>(url);
		//else
		if (url.protocol == "udpmr")
			peer_sptr = std::make_shared<udpmr_client_msg<MsgIn,MsgOut,DataT>>(url, url_replay);
		if ( ! peer_sptr || ! peer_sptr->good())
		{
			std::cout << "connect failed to " << url.to_string() << std::endl;
			return nullptr;
		}
		std::cout << "connected " << url.to_string() << std::endl;
		add(peer_sptr);
		return peer_sptr;
	}
	template<typename MsgIn=no_msg_t, typename MsgOut=no_msg_t, typename DataT=int>
	std::shared_ptr<peer_msg<MsgIn,MsgOut,DataT>> connect_msg(const utttil::url url)
	{
		std::shared_ptr<peer_msg<MsgIn,MsgOut,DataT>> peer_sptr;
		if (url.protocol == "tcp")
			peer_sptr = std::make_shared<tcp_socket_msg<MsgIn,MsgOut,DataT>>(url);
		else if (url.protocol == "udpm")
			peer_sptr = std::make_shared<udpm_client_msg<MsgIn,MsgOut,DataT>>(url);
		if ( ! peer_sptr || ! peer_sptr->good())
		{
			std::cout << "connect failed to " << url.to_string() << std::endl;
			return nullptr;
		}
		std::cout << "connected " << url.to_string() << std::endl;
		add(peer_sptr);
		return peer_sptr;
	}

	void loop_accept()
	{
		std::vector<std::shared_ptr<peer>> accept_peers;
		while(go_on)
		{
			while ( ! new_accept_peers.empty())
			{
				accept_peers.push_back(new_accept_peers.front());
				new_accept_peers.pop_front();
			}
			for(int i=accept_peers.size()-1 ; i>=0 ; i--)
			{
				std::shared_ptr<peer> peer_sptr = accept_peers[i];
				auto new_peer_sptr = peer_sptr->accept();
				if ( ! new_peer_sptr)
					continue;
				else if ( ! peer_sptr->good())
				{
					std::cout << "erase accept peer" << std::endl;
					accept_peers.erase(accept_peers.begin()+i);
				}
				std::cout << "accepted" << std::endl;
				add(new_peer_sptr);
			}
			sleep(1);
		}
	}
	template<typename Callback>
	void loop_read(Callback callback)
	{
		std::vector<std::shared_ptr<peer>> read_peers;
		while(go_on)
		{
			while ( ! new_read_peers.empty())
			{
				read_peers.push_back(new_read_peers.front());
				new_read_peers.pop_front();
			}
			for(int i=read_peers.size()-1 ; i>=0 ; i--)
			{
				auto peer_sptr = read_peers[i];
				int count = peer_sptr->read();
				peer_sptr->unpack();
				callback();
				if (count < 0)
				{
					if ( ! peer_sptr->good())
					{
						std::cout << "erase read peer" << std::endl;
						read_peers.erase(read_peers.begin()+i); // TODO swap with to end before erasing
					}
					continue;
				}
			}
		}
	}
	void loop_read()
	{
		loop_read([](){});
	}
	void loop_write()
	{
		std::vector<std::shared_ptr<peer>> write_peers;
		while(go_on)
		{
			while ( ! new_write_peers.empty())
			{
				write_peers.push_back(new_write_peers.front());
				new_write_peers.pop_front();
			}
			for(int i=write_peers.size()-1 ; i>=0 ; i--)
			{
				std::shared_ptr<peer> peer_sptr = write_peers[i];
				peer_sptr->pack();
				int count = peer_sptr->write();
				if (count < 0)
				{
					if ( ! peer_sptr->good())
					{
						std::cout << "erase write peer" << std::endl;
						write_peers.erase(write_peers.begin()+i);
					}
					continue;
				}
			}
		}
	}

	void loop_all()
	{
		std::vector<std::shared_ptr<peer>> accept_peers;
		std::vector<std::shared_ptr<peer>> read_peers;
		std::vector<std::shared_ptr<peer>> write_peers;
		while(go_on)
		{
			while ( ! new_accept_peers.empty())
			{
				accept_peers.push_back(new_accept_peers.front());
				new_accept_peers.pop_front();
			}
			for(int i=accept_peers.size() ; i>0 ; )
			{
				--i;
				std::shared_ptr<peer> peer_sptr = accept_peers[i];
				auto new_peer_sptr = peer_sptr->accept();
				if ( ! new_peer_sptr)
					continue;
				else if ( ! peer_sptr->good())
				{
					std::cout << "erase accept peer" << std::endl;
					accept_peers.erase(accept_peers.begin()+i);
				}
				std::cout << "accepted" << std::endl;
				add(new_peer_sptr);
			}
			
			while ( ! new_write_peers.empty())
			{
				write_peers.push_back(new_write_peers.front());
				new_write_peers.pop_front();
			}
			for(int i=write_peers.size() ; i>0 ; )
			{
				--i;
				std::shared_ptr<peer> peer_sptr = write_peers[i];
				peer_sptr->pack();
				int count = peer_sptr->write();
				if (count < 0)
				{
					if ( ! peer_sptr->good())
					{
						std::cout << "erase write peer" << std::endl;
						write_peers.erase(write_peers.begin()+i);
					}
					continue;
				}
			}

			while ( ! new_read_peers.empty())
			{
				read_peers.push_back(new_read_peers.front());
				new_read_peers.pop_front();
			}
			for(int i=read_peers.size() ; i>0 ; )
			{
				--i;
				auto peer_sptr = read_peers[i];
				int count = peer_sptr->read();
				peer_sptr->unpack();
				if (count < 0)
				{
					if ( ! peer_sptr->good())
					{
						std::cout << "erase read peer" << std::endl;
						read_peers.erase(read_peers.begin()+i); // TODO swap with to end before erasing
					}
					continue;
				}
			}
		}
	}
};

}} // namespace
