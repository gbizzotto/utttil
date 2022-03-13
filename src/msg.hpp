
struct msg
{
	int a;
	std::string s;

	template<typename Serializer>
	void serialize(Serializer && ss) const
	{
		ss << a
		  << s
		  ;
	}
	template<typename Deserializer>
	void deserialize(Deserializer && ss)
	{
		ss >> a
		  >> s
		  ;
	}
};
bool operator==(const msg & left, const msg & right)
{
	return left.a == right.a && left.s == right.s;
}
