
#pragma once

#include <string>
#include <sstream>
#include <map>
#include <algorithm>

namespace utttil {

template<typename STR>
struct url_
{
	STR protocol;
	STR login;
	STR password;
	STR host;
	STR port;
	STR location;
	std::map<STR, STR> args;

	url_(STR str)
	{
		auto pos = str.find("://");
		if (pos != STR::npos) {
			protocol = str.substr(0, pos);
			str = str.substr(pos+3);
		} else {
			protocol = "file";
		}

		pos = str.find('?');
		if (pos != STR::npos)
		{
			STR argstr = str.substr(pos+1);

			std::basic_istringstream<typename STR::value_type> iss(argstr);
			STR arg;
			while (std::getline(iss, arg, '&'))
			{
				auto eq_pos = arg.find('=');
				STR argname = arg.substr(0,eq_pos);
				if (eq_pos != STR::npos)
					args[argname] = arg.substr(eq_pos+1);
				else
					args[argname] = "";
			}

			str = str.substr(0, pos);
		}
		
		if (protocol == "file")
		{
			location = str;
		}
		else
		{
			pos = str.find('/');
			if (pos != STR::npos) {
				location = str.substr(pos);
				str = str.substr(0, pos);
			}

			pos = str.find('@');
			if (pos != STR::npos) {
				STR credentials = str.substr(0, pos);
				auto pos_cred = credentials.find(':');
				if (pos_cred != STR::npos) {
					login = credentials.substr(0, pos_cred);
					password = credentials.substr(pos_cred+1);
				} else {
					login = credentials;
				}
				str = str.substr(pos+1);
			}

			pos = str.find(':');
			if (pos != STR::npos) {
				host = str.substr(0,pos);
				port = str.substr(pos+1);
			} else {
				host = str;
			}
		}
	}
};

using url = url_<std::string>;

template<typename STR>
std::ostream & operator<<(std::ostream & out, const url_<STR> & u)
{
	out << "protocol = " << u.protocol << std::endl
	    << "host     = " << u.host     << std::endl
	    << "port     = " << u.port     << std::endl
	    << "location = " << u.location << std::endl
	    ;
	for (const auto & p : u.args)
		out << p.first << " = " << p.second;
	return out;

}

} // namespace
