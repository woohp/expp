#pragma once
#include "casts.hpp"
#include <map>
#include <new>
#include <stdexcept>
#include <unordered_map>
#include <vector>


namespace expp
{
// Vectors of these encode as binaries. bool is a 1-byte integral type too, but encodes as true/false.
template <typename T>
inline constexpr bool is_byte_v =
    (std::is_integral_v<T> && sizeof(T) == 1 && !std::is_same_v<T, bool>) || std::is_same_v<T, std::byte>;


template <class F>
class scope_exit
{
    F f;

public:
    explicit scope_exit(F&& f) :
        f(std::forward<F>(f))
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

// Like the map casts, unconstrained: each member uses only the element cast it needs, so
// containers of encode-only types such as resource_binary can still be returned.
template <typename T, typename Alloc>
struct type_cast<std::vector<T, Alloc>>
{
private:
    typedef std::decay_t<T> item_type;
    typedef std::vector<T, Alloc> vector_type;

public:
    constexpr static vector_type from_term(ErlNifEnv* env, const ERL_NIF_TERM term)
    {
        if constexpr (is_byte_v<T>)
        {
            ErlNifBinary binary_info;
            if (!enif_inspect_binary(env, term, &binary_info))
                throw std::invalid_argument("expected a binary, got: " + format_term(term));
            auto begin = reinterpret_cast<const T*>(binary_info.data);
            auto end = begin + binary_info.size;
            return vector_type(begin, end);
        }
        else
        {
            unsigned len = 0;
            if (!enif_get_list_length(env, term, &len))
                throw std::invalid_argument("expected a list, got: " + format_term(term));

            vector_type items;
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

    static ERL_NIF_TERM to_term(ErlNifEnv* env, const vector_type& items)
    {
        if constexpr (is_byte_v<T>)
        {
            return detail::make_new_binary(env, items.data(), items.size());
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


namespace detail
{

template <typename Map>
Map decode_map(ErlNifEnv* env, ERL_NIF_TERM term)
{
    ErlNifMapIterator iter;
    if (!enif_map_iterator_create(env, term, &iter, ERL_NIF_MAP_ITERATOR_FIRST))
        throw std::invalid_argument("expected a map, got: " + format_term(term));

    auto guard = scope_exit([env, &iter]() { enif_map_iterator_destroy(env, &iter); });

    Map _map;
    if constexpr (requires { _map.reserve(0); })
    {
        std::size_t size;
        enif_get_map_size(env, term, &size);
        _map.reserve(size);
    }

    ERL_NIF_TERM key, value;
    while (enif_map_iterator_get_pair(env, &iter, &key, &value))
    {
        _map.emplace(
            type_cast<typename Map::key_type>::from_term(env, key),
            type_cast<typename Map::mapped_type>::from_term(env, value));
        enif_map_iterator_next(env, &iter);
    }

    return _map;
}

template <typename Map>
ERL_NIF_TERM encode_map(ErlNifEnv* env, const Map& _map)
{
    std::vector<ERL_NIF_TERM> keys;
    std::vector<ERL_NIF_TERM> values;
    keys.reserve(_map.size());
    values.reserve(_map.size());

    for (const auto& item : _map)
    {
        keys.push_back(type_cast<typename Map::key_type>::to_term(env, item.first));
        values.push_back(type_cast<typename Map::mapped_type>::to_term(env, item.second));
    }

    ERL_NIF_TERM map_term;
    if (!enif_make_map_from_arrays(env, keys.data(), values.data(), keys.size(), &map_term))
        throw std::invalid_argument("duplicate keys in map");
    return map_term;
}

template <typename Map>
Map decode_multimap(ErlNifEnv* env, ERL_NIF_TERM term)
{
    unsigned len = 0;
    if (!enif_get_list_length(env, term, &len))
        throw std::invalid_argument("expected a list of {key, value} tuples, got: " + format_term(term));

    Map _map;
    if constexpr (requires { _map.reserve(len); })
        _map.reserve(len);

    ERL_NIF_TERM list_term = term;
    for (unsigned i = 0; i < len; i++)
    {
        ERL_NIF_TERM head, tail;
        enif_get_list_cell(env, list_term, &head, &tail);
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


template <typename K, typename V, typename Hash, typename Eq, typename Alloc>
struct type_cast<std::unordered_map<K, V, Hash, Eq, Alloc>>
{
    using map_type = std::unordered_map<K, V, Hash, Eq, Alloc>;

    static map_type from_term(ErlNifEnv* env, ERL_NIF_TERM term)
    {
        return detail::decode_map<map_type>(env, term);
    }

    static ERL_NIF_TERM to_term(ErlNifEnv* env, const map_type& _map)
    {
        return detail::encode_map(env, _map);
    }
};


template <typename K, typename V, typename Compare, typename Alloc>
struct type_cast<std::map<K, V, Compare, Alloc>>
{
    using map_type = std::map<K, V, Compare, Alloc>;

    static map_type from_term(ErlNifEnv* env, ERL_NIF_TERM term)
    {
        return detail::decode_map<map_type>(env, term);
    }

    static ERL_NIF_TERM to_term(ErlNifEnv* env, const map_type& _map)
    {
        return detail::encode_map(env, _map);
    }
};


template <typename K, typename V, typename Compare, typename Alloc>
struct type_cast<std::multimap<K, V, Compare, Alloc>>
{
    using map_type = std::multimap<K, V, Compare, Alloc>;

    static map_type from_term(ErlNifEnv* env, ERL_NIF_TERM term)
    {
        return detail::decode_multimap<map_type>(env, term);
    }

    static ERL_NIF_TERM to_term(ErlNifEnv* env, const map_type& _map)
    {
        return detail::encode_multimap(env, _map);
    }
};


template <typename K, typename V, typename Hash, typename Eq, typename Alloc>
struct type_cast<std::unordered_multimap<K, V, Hash, Eq, Alloc>>
{
    using map_type = std::unordered_multimap<K, V, Hash, Eq, Alloc>;

    static map_type from_term(ErlNifEnv* env, ERL_NIF_TERM term)
    {
        return detail::decode_multimap<map_type>(env, term);
    }

    static ERL_NIF_TERM to_term(ErlNifEnv* env, const map_type& _map)
    {
        return detail::encode_multimap(env, _map);
    }
};
}  // namespace expp
