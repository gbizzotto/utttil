
#pragma once

#include "absl/container/flat_hash_map.h"
#include "unique_int.hpp"
#include "pool.hpp"

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

	template<typename T, typename Tag>       V & get (const unique_int<T,Tag> & i)       { return get (i.value()); }
	template<typename T, typename Tag> const V * find(const unique_int<T,Tag> & i) const { return find(i.value()); }
	template<typename T, typename Tag>       V * find(const unique_int<T,Tag> & i)       { return find(i.value()); }
};

template<typename K, typename V>
struct seqdict_pool
{
	using   seq_type = K;
	using value_type = V;
	using inserted_t = bool;
	using pool_idx_t = uint32_t;

	seq_type next_seq = 0;
	utttil::fixed_pool<V> pool;
	absl::flat_hash_map<seq_type,pool_idx_t> key_map;

	seqdict_pool(uint32_t size)
		: pool(size)
	{}

	seq_type next_key() const { return next_seq; }
	size_t size() const { return pool.size(); }
	
	template<typename...Args>
	std::tuple<seq_type,inserted_t> push_back(Args...args)
	{
		value_type * inserted_v = pool.alloc(args...);
		if ( ! inserted_v)
			return {0,false};
		seq_type inserted_seq = next_seq++;
		pool_idx_t pool_index = pool.index_of(inserted_v);
		key_map.insert({inserted_seq, pool_index});
		return {inserted_seq, true};
	}
	bool contains(K k)
	{
		auto it = key_map.find(k);
		return (it != key_map.end());
	}
	V * find(K k)
	{
		auto it = key_map.find(k);
		if (it == key_map.end())
			return nullptr;
		return &pool.element_at(it->second);
	}
	bool erase(K k)
	{
		auto it = key_map.find(k);
		if (it == key_map.end())
			return false;
		V & v = pool.element_at(it->second);
		key_map.erase(it);
		pool.free(&v);
		return true;
	}
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

	template<typename T, typename Tag>       V & get (const unique_int<T,Tag> & i)       { return get (i.value()); }
	template<typename T, typename Tag> const V * find(const unique_int<T,Tag> & i) const { return find(i.value()); }
	template<typename T, typename Tag>       V * find(const unique_int<T,Tag> & i)       { return find(i.value()); }
};


} // namespace
