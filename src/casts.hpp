#pragma once
#include "atom.hpp"
#include "binary.hpp"
#include "resource.hpp"
#include "type_cast_fwd.hpp"
#include <algorithm>
#include <concepts>
#include <cstdint>
#include <erl_nif.h>
#include <expected>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <tuple>
#include <variant>


namespace expp
{
using namespace std::literals::string_view_literals;


template <typename T>
concept type_castable = requires(T t) {
    { type_cast<T>::to_term(nullptr, t) } -> std::same_as<ERL_NIF_TERM>;
    { type_cast<T>::from_term(nullptr, static_cast<ERL_NIF_TERM>(0)) } -> std::convertible_to<T>;
};


struct term
{
    ERL_NIF_TERM value;

    operator ERL_NIF_TERM() const
    {
        return value;
    }
};


template <>
struct type_cast<term>
{
    static term from_term(ErlNifEnv*, ERL_NIF_TERM t)
    {
        return term { t };
    }

    static ERL_NIF_TERM to_term(ErlNifEnv*, term t)
    {
        return t.value;
    }
};


template <std::integral T>
struct type_cast<T>
{
    static T from_term(ErlNifEnv* env, ERL_NIF_TERM term)
    {
        if constexpr (std::is_signed_v<T>)
        {
            if constexpr (sizeof(T) < 8)
            {
                int i;
                if (!enif_get_int(env, term, &i))
                    throw std::invalid_argument("invalid int");
                return static_cast<T>(i);
            }
            else
            {
                ErlNifSInt64 i;
                if (!enif_get_int64(env, term, &i))
                    throw std::invalid_argument("invalid int64");
                return static_cast<T>(i);
            }
        }
        else
        {
            if constexpr (sizeof(T) < 8)
            {
                unsigned int i;
                if (!enif_get_uint(env, term, &i))
                    throw std::invalid_argument("invalid uint");
                return static_cast<T>(i);
            }
            else
            {
                ErlNifUInt64 i;
                if (!enif_get_uint64(env, term, &i))
                    throw std::invalid_argument("invalid uint64");
                return static_cast<T>(i);
            }
        }
    }

    static ERL_NIF_TERM to_term(ErlNifEnv* env, T i) noexcept
    {
        if constexpr (std::is_signed_v<T>)
        {
            if constexpr (sizeof(T) < 8)
                return enif_make_int(env, i);
            else
                return enif_make_int64(env, static_cast<ErlNifSInt64>(i));
        }
        else
        {
            if constexpr (sizeof(T) < 8)
                return enif_make_uint(env, i);
            else
                return enif_make_uint64(env, static_cast<ErlNifUInt64>(i));
        }
    }
};


template <std::floating_point T>
struct type_cast<T>
{
    static T from_term(ErlNifEnv* env, ERL_NIF_TERM term)
    {
        double d;
        if (!enif_get_double(env, term, &d))
            throw std::invalid_argument("invalid double");
        return static_cast<T>(d);
    }

    static ERL_NIF_TERM to_term(ErlNifEnv* env, T d) noexcept
    {
        return enif_make_double(env, static_cast<double>(d));
    }
};


// Note: from_term always copies from the binary data into a new std::string.
// For read-only access without copying, prefer std::string_view.
template <>
struct type_cast<std::string>
{
    static std::string from_term(ErlNifEnv* env, ERL_NIF_TERM term)
    {
        ErlNifBinary binary_info;
        if (!enif_inspect_binary(env, term, &binary_info))
            throw std::invalid_argument("invalid string");
        return std::string(reinterpret_cast<const char*>(binary_info.data), binary_info.size);
    }

    static ERL_NIF_TERM to_term(ErlNifEnv* env, const std::string& s)
    {
        ErlNifBinary binary_info;
        if (!enif_alloc_binary(s.size(), &binary_info))
            throw std::bad_alloc { };
        std::copy_n(s.data(), s.size(), binary_info.data);
        return enif_make_binary(env, &binary_info);
    }
};


template <>
struct type_cast<std::string_view>
{
    static std::string_view from_term(ErlNifEnv* env, ERL_NIF_TERM term)
    {
        ErlNifBinary binary_info;
        if (!enif_inspect_binary(env, term, &binary_info))
            throw std::invalid_argument("invalid string");
        return std::string_view(reinterpret_cast<const char*>(binary_info.data), binary_info.size);
    }

    static ERL_NIF_TERM to_term(ErlNifEnv* env, const std::string_view s)
    {
        ErlNifBinary binary_info;
        if (!enif_alloc_binary(s.size(), &binary_info))
            throw std::bad_alloc { };
        std::copy_n(s.data(), s.size(), binary_info.data);
        return enif_make_binary(env, &binary_info);
    }
};


template <>
struct type_cast<binary>
{
    static binary from_term(ErlNifEnv* env, ERL_NIF_TERM term)
    {
        binary b;
        if (!enif_inspect_binary(env, term, &b))
            throw std::invalid_argument("invalid binary");
        b._term = term;
        return b;
    }

    static ERL_NIF_TERM to_term(ErlNifEnv* env, const binary& b) noexcept
    {
        if (b._term)
            return b._term;

        auto b_ = const_cast<binary*>(&b);
        b_->_term = enif_make_binary(env, reinterpret_cast<ErlNifBinary*>(b_));

        return b_->_term;
    }
};


template <>
struct type_cast<atom>
{
    static atom from_term(ErlNifEnv* env, ERL_NIF_TERM term)
    {
        unsigned len;
        if (!enif_get_atom_length(env, term, &len, ERL_NIF_LATIN1))
            throw std::invalid_argument("invalid atom");
        std::string s(len, ' ');

        if (enif_get_atom(env, term, &s[0], len + 1, ERL_NIF_LATIN1) != int(len + 1))
            throw std::invalid_argument("invalid atom");

        return atom { s };
    }

    static ERL_NIF_TERM to_term(ErlNifEnv* env, const atom& a) noexcept
    {
        return enif_make_atom_len(env, a.name.data(), a.name.length());
    }

    static ERL_NIF_TERM to_term(ErlNifEnv* env, const std::string_view& s) noexcept
    {
        return enif_make_atom_len(env, s.data(), s.length());
    }
};


template <>
struct type_cast<bool>
{
    static bool from_term(ErlNifEnv* env, ERL_NIF_TERM term)
    {
        char buf[8];
        std::size_t bytes_read = enif_get_atom(env, term, buf, 8, ERL_NIF_LATIN1);
        if (bytes_read == 0)
            throw std::invalid_argument("not boolean");

        std::string_view atom_str(buf, bytes_read - 1);
        if (atom_str == "true"sv)
            return true;
        else if (atom_str == "false"sv)
            return false;
        else
            throw std::invalid_argument("not boolean");
    }

    static ERL_NIF_TERM to_term(ErlNifEnv* env, bool b) noexcept
    {
        // enif_make_atom is internally interned by the VM, so calling it each
        // time is cheap and avoids caching ERL_NIF_TERMs across environments.
        if (b)
            return enif_make_atom(env, "true");
        else
            return enif_make_atom(env, "false");
    }
};


template <typename X, typename Y>
struct type_cast<std::pair<X, Y>>
{
    constexpr static std::pair<X, Y> from_term(ErlNifEnv* env, ERL_NIF_TERM term)
    {
        const ERL_NIF_TERM* tup_array = nullptr;
        int arity;
        if (!enif_get_tuple(env, term, &arity, &tup_array))
            throw std::invalid_argument("invalid pair");
        if (arity != 2)
            throw std::invalid_argument("invalid pair");
        return std::pair<X, Y>(type_cast<X>::from_term(env, tup_array[0]), type_cast<Y>::from_term(env, tup_array[1]));
    }

    constexpr static ERL_NIF_TERM to_term(ErlNifEnv* env, const std::pair<X, Y>& item) noexcept
    {
        return enif_make_tuple2(env, type_cast<X>::to_term(env, item.first), type_cast<Y>::to_term(env, item.second));
    }
};


template <typename... Args>
struct type_cast<std::tuple<Args...>>
{
private:
    typedef std::tuple<Args...> tuple_type;

    template <std::size_t... I>
    constexpr static tuple_type from_term_impl(ErlNifEnv* env, const ERL_NIF_TERM* tup_array, std::index_sequence<I...>)
    {
        return tuple_type(type_cast<std::decay_t<Args>>::from_term(env, tup_array[I])...);
    }

    template <std::size_t... I>
    constexpr static ERL_NIF_TERM
    to_term_impl(ErlNifEnv* env, const tuple_type& items, std::index_sequence<I...>) noexcept
    {
        return enif_make_tuple(
            env, std::tuple_size_v<tuple_type>, type_cast<std::decay_t<Args>>::to_term(env, std::get<I>(items))...);
    }

public:
    static tuple_type from_term(ErlNifEnv* env, ERL_NIF_TERM term)
    {
        const ERL_NIF_TERM* tup_array;
        int arity;
        if (!enif_get_tuple(env, term, &arity, &tup_array))
            throw std::invalid_argument("invalid tuple");
        if (arity != static_cast<int>(sizeof...(Args)))
            throw std::invalid_argument("invalid tuple arity");
        return from_term_impl(env, tup_array, std::index_sequence_for<Args...> {});
    }

    static ERL_NIF_TERM to_term(ErlNifEnv* env, const tuple_type& items) noexcept
    {
        return to_term_impl(env, items, std::index_sequence_for<Args...> { });
    }
};


template <typename... Args>
struct type_cast<std::variant<Args...>>
{
private:
    typedef std::variant<Args...> variant_type;

    template <int I, typename T, typename... Rest>
    constexpr static variant_type from_term_impl(ErlNifEnv* env, ERL_NIF_TERM term)
    {
        try
        {
            return variant_type(std::in_place_index<I>, type_cast<T>::from_term(env, term));
        }
        catch (const std::invalid_argument&)
        {
            if constexpr (sizeof...(Rest) == 0)
                throw std::invalid_argument("invalid argument");
            else
                return type_cast<variant_type>::from_term_impl<I + 1, Rest...>(env, term);
        }
    }

public:
    constexpr static variant_type from_term(ErlNifEnv* env, ERL_NIF_TERM term)
    {
        return type_cast<std::variant<Args...>>::from_term_impl<0, Args...>(env, term);
    }

    constexpr static ERL_NIF_TERM to_term(ErlNifEnv* env, const variant_type& item) noexcept
    {
        return std::visit(
            [env, &item](auto&& arg) {
                using T = std::decay_t<decltype(arg)>;
                return type_cast<T>::to_term(env, std::get<T>(item));
            },
            item);
    }
};


template <typename T, typename E>
struct type_cast<std::expected<T, E>>
{
private:
    typedef std::expected<T, E> expected_type;

public:
    static ERL_NIF_TERM to_term(ErlNifEnv* env, const expected_type& result) noexcept
    {
        if (result.has_value())
            return enif_make_tuple2(env, enif_make_atom(env, "ok"), type_cast<T>::to_term(env, *result));
        else
            return enif_make_tuple2(env, enif_make_atom(env, "error"), type_cast<E>::to_term(env, result.error()));
    };
};


template <typename T>
struct type_cast<std::optional<T>>
{
    static std::optional<T> from_term(ErlNifEnv* env, ERL_NIF_TERM term)
    {
        static_assert(!std::is_same_v<T, atom>, "std::optional cannot wrap an atom");

        if (enif_is_atom(env, term))
        {
            char buf[8];
            if (enif_get_atom(env, term, buf, 8, ERL_NIF_LATIN1) != 4)
                throw std::invalid_argument("not nil");
            if (std::string_view(buf, 3) != "nil")
                throw std::invalid_argument("not nil");
            return std::nullopt;
        }
        else
        {
            return type_cast<T>::from_term(env, term);
        }
    }

    static ERL_NIF_TERM to_term(ErlNifEnv* env, const std::optional<T>& item)
    {
        if (item)
            return type_cast<T>::to_term(env, *item);
        else
            return enif_make_atom(env, "nil");
    }
};


template <typename T>
struct type_cast<resource<T>>
{
    static resource<T> from_term(ErlNifEnv* env, ERL_NIF_TERM term)
    {
        return resource<T> { env, term };
    }

    static ERL_NIF_TERM to_term(ErlNifEnv* env, const resource<T>& res)
    {
        // Just create the term — the resource<T> destructor will call
        // enif_release_resource when the object goes out of scope,
        // leaving the term as the sole owner of the NIF resource.
        return enif_make_resource(env, res.objp);
    }
};
}
