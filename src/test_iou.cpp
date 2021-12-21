
#include <signal.h>
#include "utttil/iou.hpp"

std::vector<char> vec_from_str(std::string s)
{
	std::vector<char> v;
	for(const char a : s)
		v.push_back(a);
	return v;
}
std::string str_from_vec(std::vector<char> & v)
{
	std::string s;
	for(const char a : v)
		s.push_back(a);
	return s;
}

bool test_data()
{
    const std::string sent_by_cli = "You keep using that word. I don't think it means what you think it means.";
    const std::string sent_by_srv = "My name is Inigo Montoya. You killed my father. Prepare to die.";

    utttil::url url("tcp://localhost:8000");

    std::string rcvd_by_srv;
    std::string rcvd_by_cli_welcome;

    // server

	utttil::iou::context srv_ctx;
	srv_ctx.run();
	auto srv = srv_ctx.bind(url);
	if ( ! srv)
	{
		std::cerr << "Bind failed" << std::endl;
		return -1;
	}

	srv->on_accept = [&](utttil::iou::peer * p)
		{
			p->post_write(vec_from_str(sent_by_srv));
		};
	srv->on_data = [&](utttil::iou::peer * p)
		{
			rcvd_by_srv += str_from_vec(p->inbox.front());

			std::cout << "srv " << p->inbox.front().size() << " " << str_from_vec(p->inbox.front()) << std::endl;
			p->inbox.pop_front();
		};
	srv->on_close = [](utttil::iou::peer * p)
		{
			std::cout << "closed on server side" << std::endl;
		};


	// client

	utttil::iou::context cli_ctx;
	cli_ctx.run();
	auto cli = cli_ctx.connect(url);
	if ( ! cli)
	{
		std::cerr << "Connect failed" << std::endl;
		return -1;
	}

	cli->on_data = [&](utttil::iou::peer * p)
		{
			rcvd_by_cli_welcome += str_from_vec(p->inbox.front());

			std::cout << "cli " << p->inbox.front().size() << " " << str_from_vec(p->inbox.front()) << std::endl;
			p->inbox.pop_front();
		};
	cli->on_close = [](utttil::iou::peer * p)
		{
			std::cout << "closed on client side" << std::endl;
		};

	cli->post_write(vec_from_str(sent_by_cli));
	

	std::this_thread::sleep_for(std::chrono::milliseconds(10));

	std::cout << "rcvd_by_srv: "         << rcvd_by_srv         << "->" << (rcvd_by_srv        ==sent_by_cli) << std::endl;
	std::cout << "rcvd_by_cli_welcome: " << rcvd_by_cli_welcome << "->" << (rcvd_by_cli_welcome==sent_by_srv) << std::endl;

	bool success = true
		&& rcvd_by_srv         == sent_by_cli
		&& rcvd_by_cli_welcome == sent_by_srv
		;

	return success;
}

struct message
{
	std::string str;
	int i;

	template<typename Serializer>
	void serialize(Serializer && s) const
	{
		s << str
		  << i
		;
	}
	template<typename Deserializer>
	void deserialize(Deserializer && s)
	{
		s >> str
		  >> i
		;
	}
};
inline bool operator==(const message & left, const message & right)
{
	return left.str == right.str
		&& left.i   == right.i
		;
}

bool test_messages()
{
	return true;

    const message sent_by_cli{"Inconceivable! As you wish.", 1234};
    const message sent_by_srv{"Have fun storming the casle!", 4321};

    message rcvd_by_srv;
    message rcvd_by_cli_welcome;

    utttil::url url("tcp://localhost:8000");

    // server

	utttil::iou::context srv_ctx;
	srv_ctx.run();
	auto srv = srv_ctx.bind_srlz<message,message>(url);
	if ( ! srv)
	{
		std::cerr << "Bind failed" << std::endl;
		return -1;
	}

	srv->on_accept = [&](utttil::iou::peer * p)
		{
			utttil::iou::msg_peer<message,message> * p2 = (utttil::iou::msg_peer<message,message> *)p;
			p2->post_send(std::make_unique<message>(sent_by_srv));
		};
	srv->on_message = [&](utttil::iou::msg_peer<message,message> * p)
		{
			rcvd_by_srv = *p->inbox_msg.front();

			std::cout << "srv " << p->inbox_msg.front()->str << " " << p->inbox_msg.front()->i << std::endl;
			p->inbox_msg.pop_front();
		};
	srv->on_close = [](utttil::iou::peer * p)
		{
			std::cout << "closed on server side" << std::endl;
		};


	// client

	utttil::iou::context cli_ctx;
	cli_ctx.run();
	auto cli = cli_ctx.connect_srlz<message,message>(url);
	if ( ! cli)
	{
		std::cerr << "Connect failed" << std::endl;
		return -1;
	}

	cli->on_message = [&](utttil::iou::msg_peer<message,message> * p)
		{
			rcvd_by_cli_welcome = *p->inbox_msg.front();

			std::cout << "cli " << p->inbox_msg.front()->str << " " << p->inbox_msg.front()->i << std::endl;
			p->inbox_msg.pop_front();
		};
	cli->on_close = [](utttil::iou::peer * p)
		{
			std::cout << "closed on client side" << std::endl;
		};

	cli->post_send(std::make_unique<message>(sent_by_cli));
	

	std::this_thread::sleep_for(std::chrono::milliseconds(10));

	std::cout << "rcvd_by_srv: "         << rcvd_by_srv        .str << " " << rcvd_by_srv        .i << "->" << (rcvd_by_srv        ==sent_by_cli) << std::endl;
	std::cout << "rcvd_by_cli_welcome: " << rcvd_by_cli_welcome.str << " " << rcvd_by_cli_welcome.i << "->" << (rcvd_by_cli_welcome==sent_by_srv) << std::endl;

	bool success = true
		&& rcvd_by_srv         == sent_by_cli
		&& rcvd_by_cli_welcome == sent_by_srv
		;

	return success;
}

int main()
{
	bool success = true
		&& test_data()
		&& test_messages()
		;
	return success ? 0 : 1;
}