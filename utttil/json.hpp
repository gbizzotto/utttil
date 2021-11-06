
#pragma once

#include <map>
#include <vector>
#include <variant>
#include <cwchar>
#include <exception>
#include <string>

namespace utttil {
namespace json {

template<typename STR>
struct dict;
template<typename STR>
struct array;

template<typename STR>
using var = std::variant<dict<STR>,array<STR>,int,float,STR>;

template<typename STR>
struct dict : public std::map<STR,var<STR>>
{
};

template<typename STR>
struct array : public std::vector<var<STR>>
{
};

template<typename STR, typename CHR>
dict<STR> read_dict(const CHR *& c);
template<typename STR, typename CHR>
array<STR> read_array(const CHR *& c);
template<typename STR, typename CHR>
STR read_string(const CHR *& c);

template<typename CHR>
int read_int(const CHR *& c);
template<>
inline int read_int<char>(const char *& c)
{
	int result = atoi(c);
	if (*c == '-')
		c++;
	while (*c >= '0' && *c <= '9')
		c++;
	return result;
}
template<>
inline int read_int<wchar_t>(const wchar_t *& c)
{
	wchar_t * end;
	int result = std::wcstol(c, &end, 10);
	if (*c == '-')
		c++;
	while (*c >= '0' && *c <= '9')
		c++;
	return result;
}

template<typename CHR>
void skip_separators(const CHR *& c)
{
	while (*c == ' ' || *c == '\t' || *c == '\n' || *c == '\r')
		c++;
	if (*c == 0)
		throw std::runtime_error(std::string("bad json at ").append(std::string(c)));
}

template<typename STR, typename CHR>
var<STR> read_var(const CHR *& c)
{
	skip_separators(c);
	if (*c == '{')
		return read_dict<STR>(c);
	else if (*c == '[')
		return read_array<STR>(c);
	else if (*c == '"')
		return read_string<STR>(c);
	else if ((*c >= '0' && *c <= '9') || *c == '-')
		return read_int(c);
	else
		throw std::runtime_error(std::string("bad json at ").append(std::string(c)));
}

template<typename STR, typename CHR>
STR read_string(const CHR *& c)
{
	STR str;
	if (*c != '"')
		throw std::runtime_error(std::string("bad json at ").append(std::string(c)));
	for (c++ ; *c != '"' ; c++)
	{
		if (*c == 0)
			throw std::runtime_error(std::string("bad json at ").append(std::string(c)));
		str.append(1, *c);
	}
	c++; // skip last '"'
	return str;
}

template<typename STR, typename CHR>
dict<STR> read_dict(const CHR *& c)
{
	skip_separators(c);
	if (*c != '{')
		throw std::runtime_error(std::string("bad json at ").append(std::string(c)));
	dict<STR> result;
	c++;
	for (;;)
	{
		skip_separators(c);
		if (*c == '}')
			break;
		if (*c != '"')
			throw std::runtime_error(std::string("bad json at ").append(std::string(c)));
		STR key = read_string<STR>(c);
		skip_separators(c);
		if (*c != ':')
			throw std::runtime_error(std::string("bad json at ").append(std::string(c)));
		c++; // skip ':'
		skip_separators(c);
		result.insert({key, read_var<STR>(c)});
		skip_separators(c);
		if (*c == ',')
			c++;
	}
	c++; // skip '}'
	return result;
}
template<typename STR, typename CHR>
dict<STR> read_dict_(const CHR * c)
{
	return read_dict<STR,CHR>(c);
}

template<typename STR, typename CHR>
array<STR> read_array(const CHR *& c)
{
	skip_separators(c);
	if (*c != '[')
		throw std::runtime_error("bad json");
	array<STR> result;
	c++;
	for (;;)
	{
		skip_separators(c);
		if (*c == ']')
			break;
		result.push_back(read_var<STR>(c));
		skip_separators(c);
		if (*c == ',')
			c++;
	}
	c++; // skip '}'
	return result;
}

template<typename STR>
STR make_indent(int indent)
{
	STR result;
	for (int i=0 ; i<indent ; i++)
		result.append("\t");
	return result;
}

template<typename STR>
STR to_str(const STR & str)
{
	return STR("\"").append(str).append("\"");
}
template<typename STR>
STR to_str(const int v)
{
	return STR(std::to_string(v).c_str());
}
template<typename STR>
STR to_str(const dict<STR> & dict, int indent=0)
{
	STR result;
	result.append("{\n");
	indent++;
	for (auto it=dict.begin(),end=dict.end() ; it!=end ; )
	{
		result.append(make_indent<STR>(indent));
		result.append(to_str(it->first));
		result.append(": ");
		result.append(to_str(it->second, indent+1));
		++it;
		if (it != end)
			result.append(",");
		result.append("\n");
	}
	indent--;
	result.append(make_indent<STR>(indent));
	result.append("}");
	return result;
}
template<typename STR>
STR to_str(const array<STR> & dict, int indent=0)
{
	STR result;
	result.append("[\n");
	indent++;
	for (auto it=dict.begin(),end=dict.end() ; it!=end ; )
	{
		result.append(make_indent<STR>(indent));
		result.append(to_str(*it));
		++it;
		if (it != end)
			result.append(",");
		result.append("\n");
	}
	indent--;
	result.append(make_indent<STR>(indent));
	result.append("]");
	return result;
}

template<typename STR>
STR to_str(const var<STR> & var, int indent=0)
{
	STR result;
	if (std::holds_alternative<int>(var))
		result = to_str<STR>(std::get<int>(var));
	else if (std::holds_alternative<STR>(var))
		result = to_str<STR>(std::get<STR>(var));
	else if (std::holds_alternative<dict<STR>>(var))
		result = to_str(std::get<dict<STR>>(var), indent);
	else if (std::holds_alternative<array<STR>>(var))
		result = to_str(std::get<array<STR>>(var), indent);
	else
		result = "";
	return result;
}

}} // namespace
