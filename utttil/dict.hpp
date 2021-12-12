
#pragma once

#include "absl/container/flat_hash_map.h"
#include "unique_int.hpp"

namespace utttil {

template<typename K, typename V>
struct Dict : public std::map<K,V>
{
	using SelfType = Dict;
	using Super = std::map<K,V>;

	V & get(const K & k)
	{
		return (*this)[k];
	}

	const V * find(const K & k) const
	{
		auto it = Super::find(k);
		if (it == Super::end())
			return nullptr;
		return &it->second;
	}
	V * find(const K & k)
	{
		return const_cast<V*>(const_cast<const SelfType*>(this)->find(k));
	}
};

template<typename K, typename V>
struct DequeDict : public std::deque<V>
{
	using SelfType = DequeDict;
	using Super = std::deque<V>;
	using value_type = V;

	V & get(const K & k)
	{
		if (k >= Super::size())
			Super::resize(k+1);
		return Super::operator[](k);
	}

	const V * find(const K & k) const
	{
		if (k < 0 || k >= Super::size())
			return nullptr;
		return & Super::operator[](k);
	}
	V * find(const K & k)
	{
		return const_cast<V*>(const_cast<const SelfType*>(this)->find(k));
	}

	template<typename T, typename Tag>       V & get (const unique_int<T,Tag> & i)       { return get (i.value()-1); }
	template<typename T, typename Tag> const V * find(const unique_int<T,Tag> & i) const { return find(i.value()-1); }
	template<typename T, typename Tag>       V * find(const unique_int<T,Tag> & i)       { return find(i.value()-1); }
};

template<typename K, typename V>
struct AbslPtrDict : public absl::flat_hash_map<K,V*>
{
	using SelfType = AbslPtrDict;
	using Super = absl::flat_hash_map<K,V*>;

	V & get(const K & k)
	{
		auto emplace_pair = this->try_emplace(k, nullptr);
		if (std::get<1>(emplace_pair))
			emplace_pair.first->second = new V();
		return *emplace_pair.first->second;
	}
	const V * find(const K & k) const
	{
		auto it = Super::find(k);
		if (it == Super::end())
			return nullptr;
		return it->second;
	}
	V * find(const K & k)
	{
		return const_cast<V*>(const_cast<const SelfType*>(this)->find(k));
	}
};

template<typename K, typename V>
struct AbslDict : public absl::flat_hash_map<K,V>
{
	using SelfType = AbslDict;
	using Super = absl::flat_hash_map<K,V>;

	V & get(const K & k)
	{
		auto emplace_pair = this->try_emplace(k, V());
		return emplace_pair.first->second;
	}
	const V * find(const K & k) const
	{
		auto it = Super::find(k);
		if (it == Super::end())
			return nullptr;
		return &it->second;
	}
	V * find(const K & k)
	{
		return const_cast<V*>(const_cast<const SelfType*>(this)->find(k));
	}
	std::tuple<V&,bool> try_insert(const K & k, const V & v)
	{
		auto emplace_pair = this->try_emplace(k, v);
		return {emplace_pair.first->second, emplace_pair.second};
	}
};


} // namespace
