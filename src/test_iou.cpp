
#include <signal.h>
#include "utttil/iou.hpp"

std::atomic_bool go_on = true;

void sigint_handler(int signo)
{
	go_on = false;
}

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

int main()
{
    signal(SIGINT, sigint_handler);

    const std::string sent_by_cli = "You keep using that word. I don't think it means what you think it means.";
    const std::string sent_by_srv = "My name is Inigo Montoya. You killed my father. Prepare to die.";

    std::string rcvd_by_srv;
    std::string rcvd_by_cli_welcome;

    // server

	utttil::iou::context srv_ctx;
	srv_ctx.run();
	auto srv = srv_ctx.bind(8000);
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
	auto cli = cli_ctx.connect("127.0.0.1", 8000);
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

	return success ? 0 : 1;
}