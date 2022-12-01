
#include "headers.hpp"

namespace utttil {

template<typename T>
struct registered
{
	static inline std::vector<registered*> instances;

	registered()
	{
		instances.insert(std::lower_bound(instances.begin(), instances.end(), this), this);
	}
	~registered()
	{
		auto it = std::lower_bound(instances.begin(), instances.end(), this);
		if (it != instances.end() && *it == this)
			instances.erase(it);
	}
	static inline bool is_instance(const registered * r)
	{
		auto it = std::lower_bound(instances.begin(), instances.end(), r);
		return it != instances.end() && *it == r;
	}
	inline bool is_registered() const
	{
		auto it = std::lower_bound(instances.begin(), instances.end(), this);
		return it != instances.end() && *it == this;
	}
};

} // namespace