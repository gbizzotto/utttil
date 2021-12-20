
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
    std::string rcvd_by_cli_copy;

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
	srv->on_message = [&](utttil::iou::peer * p)
		{
			rcvd_by_srv = str_from_vec(p->inbox.front());

			for (auto c : p->inbox.front())
				std::cout << c;
			std::cout << std::endl;
			p->post_write(std::move(p->inbox.front()));
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

	cli->on_message = [&](utttil::iou::peer * p)
		{
			if (rcvd_by_cli_welcome.empty())
				rcvd_by_cli_welcome = str_from_vec(p->inbox.front());
			else
				rcvd_by_cli_copy = str_from_vec(p->inbox.front());

			for (auto c : p->inbox.front())
				std::cout << c;
			std::cout << std::endl;
			p->inbox.pop_front();
		};
	cli->on_close = [](utttil::iou::peer * p)
		{
			std::cout << "closed on client side" << std::endl;
		};

	cli->post_write(vec_from_str(sent_by_cli));
	

	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	return true
		&& rcvd_by_srv         == sent_by_cli
		&& rcvd_by_cli_welcome == sent_by_srv
		&& rcvd_by_cli_copy    == sent_by_cli
		;
}