
#include "ws_server_socket.hpp"

namespace utttil {
namespace io {

template<typename CustomData=int>
struct ws_server : interface<CustomData>
{
	using ConnectionData = CustomData;
	using Connection     = ws_server_socket<CustomData>;
	using ConnectionSPTR = typename Connection::ConnectionSPTR;

	tcp::acceptor acceptor;

	ws_server(int port, std::shared_ptr<boost::asio::io_context> context = nullptr)
		: interface<CustomData>(context)
		, acceptor(*this->io_context, tcp::endpoint(tcp::v4(), port))
	{
		acceptor.set_option(boost::asio::socket_base::reuse_address(true));
	}
	virtual bool is_null_io() const override { return false; }
	
	virtual void close() override
	{
		if (acceptor.is_open())
			acceptor.close();
	}
	
	void listen()
	{
		acceptor.listen(boost::asio::socket_base::max_listen_connections);
		async_accept();
	}

	void async_accept()
	{
		auto new_connection_sptr = std::make_shared<Connection>(this->io_context);
		acceptor.async_accept(new_connection_sptr->get_tcp_socket(), [new_connection_sptr,this](const boost::system::error_code & error)
			{
				if ( ! error)
				{
					new_connection_sptr->on_connect = this->on_connect;
					new_connection_sptr->on_close   = this->on_close  ;
					new_connection_sptr->on_message = this->on_message;
					this->on_connect(new_connection_sptr);
					new_connection_sptr->init();
					async_accept();
				}
			});
	}
};

template<typename CustomData=int>
std::shared_ptr<ws_server<CustomData>> make_ws_server(int port, std::shared_ptr<boost::asio::io_context> context = nullptr)
{
	return std::make_shared<ws_server<CustomData>>(port, context);
}

}} // namespace
