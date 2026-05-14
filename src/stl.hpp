#pragma once
#include "casts.hpp"
#include <map>
#include <new>
#include <stdexcept>
#include <unordered_map>
#include <vector>


namespace expp
{
template <typename T>
concept InnerType = (std::is_move_constructible_v<T> || std::is_copy_constructible_v<T>) &&
                    (type_castable<T> || std::is_same_v<T, std::byte>);


template <class F>
class scope_exit
{
    F f;

public:
    explicit scope_exit(F&& f)
        : f(std::forward<F>(f))
    {}

    scope_exit(scope_exit&& other) = delete;

    ~scope_exit()
    {
        f();
    }

    scope_exit(const scope_exit&) = delete;
    scope_exit& operator=(const scope_exit&) = delete;
};

template <class F>
scope_exit(F) -> scope_exit<F>;

template <InnerType T>
struct type_cast<std::vector<T>>
{
private:
    typedef std::decay_t<T> item_type;

public:
    constexpr static std::vector<T> from_term(ErlNifEnv* env, const ERL_NIF_TERM term)
    {
        if constexpr ((std::is_integral_v<T> && sizeof(T) == 1) || std::is_same_v<T, std::byte>)
        {
            ErlNifBinary binary_info;
            if (!enif_inspect_binary(env, term, &binary_info))
                throw std::invalid_argument("invalid string");
            auto begin = reinterpret_cast<const T*>(binary_info.data);
            auto end = begin + binary_info.size;
            return std::vector<T>(begin, end);
        }
        else
        {
            unsigned len = 0;
            if (!enif_get_list_length(env, term, &len))
                throw std::invalid_argument("invalid vector");

            std::vector<T> items;
            items.reserve(len);
            ERL_NIF_TERM list_term = term;
            for (unsigned i = 0; i < len; i++)
            {
                ERL_NIF_TERM head, tail;
                enif_get_list_cell(env, list_term, &head, &tail);
                items.push_back(type_cast<item_type>::from_term(env, head));
                list_term = tail;
            }

            return items;
        }
    }

    static ERL_NIF_TERM to_term(ErlNifEnv* env, const std::vector<T>& items)
    {
        if constexpr ((std::is_integral_v<T> && sizeof(T) == 1) || std::is_same_v<T, std::byte>)
        {
            ErlNifBinary binary_info;
            if (!enif_alloc_binary(items.size(), &binary_info))
                throw std::bad_alloc{};
            std::copy_n(reinterpret_cast<const unsigned char*>(items.data()), items.size(), binary_info.data);
            return enif_make_binary(env, &binary_info);
        }
        else
        {
            ERL_NIF_TERM list = enif_make_list(env, 0);
            for (auto it = items.rbegin(); it != items.rend(); ++it)
                list = enif_make_list_cell(env, type_cast<item_type>::to_term(env, *it), list);
            return list;
        }
    }
};


template <InnerType K, InnerType V>
struct type_cast<std::unordered_map<K, V>>
{
private:
    typedef std::decay_t<K> key_type;
    typedef std::decay_t<V> value_type;
    typedef std::unordered_map<key_type, value_type> map_type;

public:
    constexpr static map_type from_term(ErlNifEnv* env, const ERL_NIF_TERM term)
    {
        map_type _map;
        std::size_t size;
        if (!enif_get_map_size(env, term, &size))
            throw std::invalid_argument("invalid map");
        _map.reserve(size);

        ErlNifMapIterator iter;
        if (!enif_map_iterator_create(env, term, &iter, ERL_NIF_MAP_ITERATOR_FIRST))
            throw std::invalid_argument("invalid map");

        auto guard = scope_exit([env, &iter]() { enif_map_iterator_destroy(env, &iter); });

        ERL_NIF_TERM key, value;
        while (enif_map_iterator_get_pair(env, &iter, &key, &value))
        {
            _map.emplace(type_cast<key_type>::from_term(env, key), type_cast<value_type>::from_term(env, value));
            enif_map_iterator_next(env, &iter);
        }

        return _map;
    }

    constexpr static ERL_NIF_TERM to_term(ErlNifEnv* env, const map_type& _map)
    {
        std::vector<ERL_NIF_TERM> keys;
        std::vector<ERL_NIF_TERM> values;
        keys.reserve(_map.size());
        values.reserve(_map.size());

        for (const auto& item : _map)
        {
            keys.push_back(type_cast<key_type>::to_term(env, item.first));
            values.push_back(type_cast<value_type>::to_term(env, item.second));
        }

        ERL_NIF_TERM map_term;
        if (!enif_make_map_from_arrays(env, keys.data(), values.data(), keys.size(), &map_term))
            throw std::invalid_argument("duplicate keys in map");
        return map_term;
    }
};


template <InnerType K, InnerType V>
struct type_cast<std::map<K, V>>
{
private:
    typedef std::decay_t<K> key_type;
    typedef std::decay_t<V> value_type;
    typedef std::map<key_type, value_type> map_type;

public:
    constexpr static map_type from_term(ErlNifEnv* env, const ERL_NIF_TERM term)
    {
        map_type _map;
        std::size_t size;
        if (!enif_get_map_size(env, term, &size))
            throw std::invalid_argument("invalid map");

        ErlNifMapIterator iter;
        if (!enif_map_iterator_create(env, term, &iter, ERL_NIF_MAP_ITERATOR_FIRST))
            throw std::invalid_argument("invalid map");

        auto guard = scope_exit([env, &iter]() { enif_map_iterator_destroy(env, &iter); });

        ERL_NIF_TERM key, value;
        while (enif_map_iterator_get_pair(env, &iter, &key, &value))
        {
            _map.emplace(type_cast<key_type>::from_term(env, key), type_cast<value_type>::from_term(env, value));
            enif_map_iterator_next(env, &iter);
        }

        return _map;
    }

    constexpr static ERL_NIF_TERM to_term(ErlNifEnv* env, const map_type& _map)
    {
        std::vector<ERL_NIF_TERM> keys;
        std::vector<ERL_NIF_TERM> values;
        keys.reserve(_map.size());
        values.reserve(_map.size());

        for (const auto& item : _map)
        {
            keys.push_back(type_cast<key_type>::to_term(env, item.first));
            values.push_back(type_cast<value_type>::to_term(env, item.second));
        }

        ERL_NIF_TERM map_term;
        if (!enif_make_map_from_arrays(env, keys.data(), values.data(), keys.size(), &map_term))
            throw std::invalid_argument("duplicate keys in map");
        return map_term;
    }
};


namespace detail
{

template <typename Map>
Map decode_multimap(ErlNifEnv* env, ERL_NIF_TERM term, const char* label)
{
    unsigned len = 0;
    if (!enif_get_list_length(env, term, &len))
        throw std::invalid_argument(std::string("invalid ") + label);

    Map _map;
    if constexpr (requires { _map.reserve(len); })
        _map.reserve(len);

    ERL_NIF_TERM list_term = term;
    for (unsigned i = 0; i < len; i++)
    {
        ERL_NIF_TERM head, tail;
        if (!enif_get_list_cell(env, list_term, &head, &tail))
            throw std::invalid_argument(std::string("invalid ") + label);
        using pair_type = std::pair<typename Map::key_type, typename Map::mapped_type>;
        _map.emplace(type_cast<pair_type>::from_term(env, head));
        list_term = tail;
    }

    return _map;
}

template <typename Map>
ERL_NIF_TERM encode_multimap(ErlNifEnv* env, const Map& _map)
{
    using pair_type = std::pair<typename Map::key_type, typename Map::mapped_type>;
    std::vector<ERL_NIF_TERM> terms;
    terms.reserve(_map.size());
    for (const auto& item : _map)
        terms.push_back(type_cast<pair_type>::to_term(env, pair_type(item.first, item.second)));
    return enif_make_list_from_array(env, terms.data(), static_cast<unsigned>(terms.size()));
}

}  // namespace detail


template <InnerType K, InnerType V>
struct type_cast<std::multimap<K, V>>
{
    using map_type = std::multimap<std::decay_t<K>, std::decay_t<V>>;

    static map_type from_term(ErlNifEnv* env, ERL_NIF_TERM term)
    {
        return detail::decode_multimap<map_type>(env, term, "multimap");
    }

    static ERL_NIF_TERM to_term(ErlNifEnv* env, const map_type& _map)
    {
        return detail::encode_multimap(env, _map);
    }
};


template <InnerType K, InnerType V>
struct type_cast<std::unordered_multimap<K, V>>
{
    using map_type = std::unordered_multimap<std::decay_t<K>, std::decay_t<V>>;

    static map_type from_term(ErlNifEnv* env, ERL_NIF_TERM term)
    {
        return detail::decode_multimap<map_type>(env, term, "unordered_multimap");
    }

    static ERL_NIF_TERM to_term(ErlNifEnv* env, const map_type& _map)
    {
        return detail::encode_multimap(env, _map);
    }
};
}  // namespace expp
