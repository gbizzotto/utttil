
#include <iostream>
#include "utttil/url.hpp"
#include "utttil/assert.hpp"

bool test_protocol()
{
	{
		const char *urls[] =
			{
				"/tmp/ok",
				"file:///tmp/ok",
				"file://user@/tmp/ok",
				"file://user:pass@/tmp/ok",
				"file://ok",
				"file://ok?a=1",
				"file://~/a/b/?a=1",
				"~/a/b/?a=1",
			};
		for (const char * u : urls)
		{
			utttil::url url(u);
			ASSERT_MSG_ACT(url.protocol, ==, "file", u, return false); 
		}
	}
	{
		const char *urls[] =
			{
				"tcp://localhost",
				"tcp://127.0.0.1",
				"tcp://127.0.0.1/",
				"tcp://127.0.0.1/a",
				"tcp://127.0.0.1/a/b/",
				"tcp://127.0.0.1:1234/a/b/",
				"tcp://127.0.0.1:1234/a/b/?a=1&b=2",
				"tcp://u@127.0.0.1:1234/a/b/?a=1&b=2",
				"tcp://u:p@127.0.0.1:1234/a/b/?a=1&b=2",
				"tcp://u:p@127.0.0.1/a/b/?a=1&b=2",
				"tcp://u:p@127.0.0.1?a=1&b=2",
				"tcp://127.0.0.1?a=1&b=2",
			};
		for (const char * u : urls)
		{
			utttil::url url(u);
			ASSERT_MSG_ACT(url.protocol, ==, "tcp", u, return false); 
		}
	}
	return true;
}

bool test_login()
{
	{
		const char *urls[] =
			{
				"tcp://user@127.0.0.1:1234/a/b/?a=1&b=2",
				"tcp://user:p@127.0.0.1:1234/a/b/?a=1&b=2",
				"tcp://user:p@127.0.0.1/a/b/?a=1&b=2",
				"tcp://user:p@127.0.0.1?a=1&b=2",
				"tcp://user:p@localhost",
				"tcp://user@localhost",
			};
		for (const char * u : urls)
		{
			utttil::url url(u);
			ASSERT_MSG_ACT(url.login, ==, "user", u, return false); 
		}
	}
	{
		const char *urls[] =
			{
				"/tmp/ok",
				"file:///tmp/ok",
				"file://ok",
				"file://ok?a=1",
				"file://~/a/b/?a=1",
				"file://user@/tmp/ok",
				"file://user:pass@/tmp/ok",
				"~/a/b/?a=1",
				"tcp://localhost",
				"tcp://127.0.0.1",
				"tcp://127.0.0.1/",
				"tcp://127.0.0.1/a",
				"tcp://127.0.0.1/a/b/",
				"tcp://127.0.0.1:1234/a/b/",
				"tcp://127.0.0.1?a=1&b=2",
			};
		for (const char * u : urls)
		{
			utttil::url url(u);
			ASSERT_ACT(url.login, ==, "", return false); 
		}
	}
	return true;
}
bool test_password()
{
	{
		const char *urls[] =
			{
				"tcp://user:p@127.0.0.1:1234/a/b/?a=1&b=2",
				"tcp://user:p@127.0.0.1/a/b/?a=1&b=2",
				"tcp://user:p@127.0.0.1?a=1&b=2",
				"tcp://user:p@localhost",
			};
		for (const char * u : urls)
		{
			utttil::url url(u);
			ASSERT_MSG_ACT(url.password, ==, "p", u, return false); 
		}
	}
	{
		const char *urls[] =
			{
				"tcp://user@127.0.0.1:1234/a/b/?a=1&b=2",
				"tcp://user@localhost",
				"/tmp/ok",
				"file:///tmp/ok",
				"file://ok",
				"file://ok?a=1",
				"file://~/a/b/?a=1",
				"file://user@/tmp/ok",
				"file://user:pass@/tmp/ok",
				"~/a/b/?a=1",
				"tcp://localhost",
				"tcp://127.0.0.1",
				"tcp://127.0.0.1/",
				"tcp://127.0.0.1/a",
				"tcp://127.0.0.1/a/b/",
				"tcp://127.0.0.1:1234/a/b/",
				"tcp://127.0.0.1?a=1&b=2",
			};
		for (const char * u : urls)
		{
			utttil::url url(u);
			ASSERT_MSG_ACT(url.password, ==, "", u, return false); 
		}
	}
	return true;
}

bool test_host()
{
	{
		const char *urls[] =
			{
				"/tmp/ok",
				"file:///tmp/ok",
				"file://user@/tmp/ok",
				"file://user:pass@/tmp/ok",
				"file://ok",
				"file://ok?a=1",
				"file://~/a/b/?a=1",
				"~/a/b/?a=1",
			};
		for (const char * u : urls)
		{
			utttil::url url(u);
			ASSERT_MSG_ACT(url.host, ==, "", u, return false); 
		}
	}
	{
		const char *urls[] =
			{
				"tcp://127.0.0.1",
				"tcp://127.0.0.1/",
				"tcp://127.0.0.1/a",
				"tcp://127.0.0.1/a/b/",
				"tcp://127.0.0.1:1234/a/b/",
				"tcp://127.0.0.1:1234/a/b/?a=1&b=2",
				"tcp://u@127.0.0.1:1234/a/b/?a=1&b=2",
				"tcp://u:p@127.0.0.1:1234/a/b/?a=1&b=2",
				"tcp://u:p@127.0.0.1/a/b/?a=1&b=2",
				"tcp://u:p@127.0.0.1?a=1&b=2",
				"tcp://127.0.0.1?a=1&b=2",
			};
		for (const char * u : urls)
		{
			utttil::url url(u);
			ASSERT_MSG_ACT(url.host, ==, "127.0.0.1", u, return false); 
		}
	}
	return true;
}

bool test_port()
{
	{
		const char *urls[] =
			{
				"/tmp/ok",
				"file:///tmp/ok",
				"file://user@/tmp/ok",
				"file://user:pass@/tmp/ok",
				"file://ok",
				"file://ok?a=1",
				"file://~/a/b/?a=1",
				"~/a/b/?a=1",
				"tcp://127.0.0.1",
				"tcp://127.0.0.1/",
				"tcp://127.0.0.1/a",
				"tcp://127.0.0.1/a/b/",
				"tcp://u:p@127.0.0.1/a/b/?a=1&b=2",
				"tcp://u:p@127.0.0.1?a=1&b=2",
				"tcp://127.0.0.1?a=1&b=2",
			};
		for (const char * u : urls)
		{
			utttil::url url(u);
			ASSERT_MSG_ACT(url.port, ==, "", u, return false); 
		}
	}
	{
		const char *urls[] =
			{
				"tcp://127.0.0.1:1234/a/b/",
				"tcp://127.0.0.1:1234/a/b/?a=1&b=2",
				"tcp://u@127.0.0.1:1234/a/b/?a=1&b=2",
				"tcp://u:p@127.0.0.1:1234/a/b/?a=1&b=2",
				"tcp://127.0.0.1:1234",
				"tcp://localhost:1234",
			};
		for (const char * u : urls)
		{
			utttil::url url(u);
			ASSERT_MSG_ACT(url.port, ==, "1234", u, return false); 
		}
	}
	return true;
}

bool test_location()
{
	{
		const char *urls[] =
			{
				"/tmp/ok",
				"file:///tmp/ok",
			};
		for (const char * u : urls)
		{
			utttil::url url(u);
			ASSERT_MSG_ACT(url.location, ==, "/tmp/ok", u, return false); 
		}
	}
	{
		const char *urls[] =
			{
				"a/b/",
				"a/b/?a=1&b=2",
				"file://a/b/",
				"file://a/b/?a=1&b=2",
			};
		for (const char * u : urls)
		{
			utttil::url url(u);
			ASSERT_MSG_ACT(url.location, ==, "a/b/", u, return false); 
		}
	}
	{
		const char *urls[] =
			{
				"/a/b/",
				"/a/b/?a=1&b=2",
				"file:///a/b/",
				"file:///a/b/?a=1&b=2",
				"tcp://127.0.0.1/a/b/",
				"tcp://127.0.0.1:1234/a/b/",
				"tcp://127.0.0.1:1234/a/b/?a=1&b=2",
				"tcp://u@127.0.0.1:1234/a/b/?a=1&b=2",
				"tcp://u:p@127.0.0.1:1234/a/b/?a=1&b=2",
			};
		for (const char * u : urls)
		{
			utttil::url url(u);
			ASSERT_MSG_ACT(url.location, ==, "/a/b/", u, return false); 
		}
	}
	{
		const char *urls[] =
			{
				"tcp://127.0.0.1",
				"tcp://u:p@127.0.0.1?a=1&b=2",
				"tcp://127.0.0.1?a=1&b=2",
				"tcp://127.0.0.1:1234",
				"tcp://localhost:1234",
			};
		for (const char * u : urls)
		{
			utttil::url url(u);
			ASSERT_MSG_ACT(url.location, ==, "", u, return false); 
		}
	}
	{
		const char *urls[] =
			{
				"tcp://127.0.0.1/",
			};
		for (const char * u : urls)
		{
			utttil::url url(u);
			ASSERT_MSG_ACT(url.location, ==, "/", u, return false); 
		}
	}
	return true;
}

bool test_args()
{
	{
		const char *urls[] =
			{
				"/tmp/ok",
				"file:///tmp/ok",
				"file://user@/tmp/ok",
				"file://user:pass@/tmp/ok",
				"file://ok",
				"tcp://127.0.0.1",
				"tcp://127.0.0.1/",
				"tcp://127.0.0.1/a",
				"tcp://127.0.0.1/a/b/",
				"tcp://127.0.0.1:1234/a/b/",
				"tcp://127.0.0.1:1234",
				"tcp://localhost:1234",
			};
		for (const char * u : urls)
		{
			utttil::url url(u);
			ASSERT_MSG_ACT(url.args.size(), ==, 0, u, return false); 
		}
	}
	{
		const char *urls[] =
			{
				"file://ok?a=1&b=2",
				"file://~/a/b/?a=1&b=2",
				"~/a/b/?a=1&b=2",
				"tcp://u:p@127.0.0.1/a/b/?a=1&b=2",
				"tcp://u:p@127.0.0.1?a=1&b=2",
				"tcp://127.0.0.1?a=1&b=2",
				"tcp://127.0.0.1:1234/a/b/?a=1&b=2",
				"tcp://u@127.0.0.1:1234/a/b/?a=1&b=2",
				"tcp://u:p@127.0.0.1:1234/a/b/?a=1&b=2",
			};
		for (const char * u : urls)
		{
			utttil::url url(u);
			ASSERT_MSG_ACT(url.args.size(), ==, 2, u, return false); 
			ASSERT_MSG_ACT(url.args["a"], ==, "1", u, return false); 
			ASSERT_MSG_ACT(url.args["b"], ==, "2", u, return false); 
		}
	}
	return true;
}

int main()
{
	bool success = test_protocol()
		&& test_login()
		&& test_password()
		&& test_host()
		&& test_port()
		&& test_location()
		&& test_args()
		;

	return success?0:1;
}