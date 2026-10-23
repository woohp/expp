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
#include <limits>
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
        return term{t};
    }

    static ERL_NIF_TERM to_term(ErlNifEnv*, term t)
    {
        return t.value;
    }
};


inline std::string format_term(ERL_NIF_TERM term)
{
    char buffer[256];
    int would_have_written = enif_snprintf(buffer, sizeof(buffer), "%T", term);
    return std::string{buffer, std::min(static_cast<std::size_t>(would_have_written), sizeof(buffer) - 1)};
}


template <std::integral T>
struct type_cast<T>
{
    using wide_type = std::conditional_t<std::is_signed_v<T>, ErlNifSInt64, ErlNifUInt64>;

    static T from_term(ErlNifEnv* env, ERL_NIF_TERM term)
    {
        wide_type i;
        bool ok;
        if constexpr (std::is_signed_v<T>)
            ok = enif_get_int64(env, term, &i);
        else
            ok = enif_get_uint64(env, term, &i);
        if (ok && i >= std::numeric_limits<T>::min() && i <= std::numeric_limits<T>::max())
            return static_cast<T>(i);
        if (enif_term_type(env, term) != ERL_NIF_TERM_TYPE_INTEGER)
            throw std::invalid_argument("expected an integer, got: " + format_term(term));
        throw std::invalid_argument(
            "integer out of range [" + std::to_string(std::numeric_limits<T>::min()) + ", " +
            std::to_string(std::numeric_limits<T>::max()) + "], got: " + format_term(term));
    }

    static ERL_NIF_TERM to_term(ErlNifEnv* env, T i) noexcept
    {
        if constexpr (std::is_signed_v<T>)
            return enif_make_int64(env, i);
        else
            return enif_make_uint64(env, i);
    }
};


template <std::floating_point T>
struct type_cast<T>
{
    static T from_term(ErlNifEnv* env, ERL_NIF_TERM term)
    {
        double d;
        if (!enif_get_double(env, term, &d))
            throw std::invalid_argument("expected a float, got: " + format_term(term));
        if (d < std::numeric_limits<T>::lowest() || d > std::numeric_limits<T>::max())
            throw std::invalid_argument("floating point argument out of bounds...");
        return static_cast<T>(d);
    }

    static ERL_NIF_TERM to_term(ErlNifEnv* env, T d) noexcept
    {
        return enif_make_double(env, static_cast<double>(d));
    }
};


template <>
struct type_cast<std::string_view>
{
    static std::string_view from_term(ErlNifEnv* env, ERL_NIF_TERM term)
    {
        ErlNifBinary binary_info;
        if (!enif_inspect_binary(env, term, &binary_info))
            throw std::invalid_argument("expected a binary, got: " + format_term(term));
        return std::string_view(reinterpret_cast<const char*>(binary_info.data), binary_info.size);
    }

    static ERL_NIF_TERM to_term(ErlNifEnv* env, const std::string_view s)
    {
        ErlNifBinary binary_info;
        if (!enif_alloc_binary(s.size(), &binary_info))
            throw std::bad_alloc{};
        std::copy_n(s.data(), s.size(), binary_info.data);
        return enif_make_binary(env, &binary_info);
    }
};


// Note: from_term always copies from the binary data into a new string.
// For read-only access without copying, prefer std::string_view.
template <typename Alloc>
struct type_cast<std::basic_string<char, std::char_traits<char>, Alloc>>
{
    using string_type = std::basic_string<char, std::char_traits<char>, Alloc>;

    static string_type from_term(ErlNifEnv* env, ERL_NIF_TERM term)
    {
        return string_type(type_cast<std::string_view>::from_term(env, term));
    }

    static ERL_NIF_TERM to_term(ErlNifEnv* env, const string_type& s)
    {
        return type_cast<std::string_view>::to_term(env, s);
    }
};


template <>
struct type_cast<binary>
{
    static binary from_term(ErlNifEnv* env, ERL_NIF_TERM term)
    {
        binary b;
        if (!enif_inspect_binary(env, term, &b))
            throw std::invalid_argument("expected a binary, got: " + format_term(term));
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
        if (!enif_get_atom_length(env, term, &len, ERL_NIF_UTF8))
            throw std::invalid_argument("expected an atom, got: " + format_term(term));
        std::string s(len, '\0');

        if (enif_get_atom(env, term, s.data(), len + 1, ERL_NIF_UTF8) != int(len + 1))
            throw std::invalid_argument("expected an atom, got: " + format_term(term));

        return atom{s};
    }

    static ERL_NIF_TERM to_term(ErlNifEnv* env, const atom& a)
    {
        ERL_NIF_TERM term;
        if (!enif_make_new_atom_len(env, a.name.data(), a.name.length(), &term, ERL_NIF_UTF8))
            throw std::runtime_error("invalid atom name (not UTF-8 or longer than 255 characters): " + a.name);
        return term;
    }
};


template <>
struct type_cast<bool>
{
    static bool from_term(ErlNifEnv* env, ERL_NIF_TERM term)
    {
        if (detail::is_atom<"true">(env, term))
            return true;
        if (detail::is_atom<"false">(env, term))
            return false;
        throw std::invalid_argument("expected a boolean (true/false), got: " + format_term(term));
    }

    static ERL_NIF_TERM to_term(ErlNifEnv* env, bool b) noexcept
    {
        if (b)
            return detail::static_atom<"true">(env);
        else
            return detail::static_atom<"false">(env);
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
            throw std::invalid_argument("expected a tuple, got: " + format_term(term));
        if (arity != 2)
            throw std::invalid_argument("expected a 2-element tuple, got: " + format_term(term));
        return std::pair<X, Y>(type_cast<X>::from_term(env, tup_array[0]), type_cast<Y>::from_term(env, tup_array[1]));
    }

    constexpr static ERL_NIF_TERM to_term(ErlNifEnv* env, const std::pair<X, Y>& item)
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
    constexpr static ERL_NIF_TERM to_term_impl(ErlNifEnv* env, const tuple_type& items, std::index_sequence<I...>)
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
            throw std::invalid_argument("expected a tuple, got: " + format_term(term));
        if (arity != static_cast<int>(sizeof...(Args)))
            throw std::invalid_argument(
                "expected tuple arity " + std::to_string(sizeof...(Args)) + ", got " + std::to_string(arity) + ": " +
                format_term(term));
        return from_term_impl(env, tup_array, std::index_sequence_for<Args...>{});
    }

    static ERL_NIF_TERM to_term(ErlNifEnv* env, const tuple_type& items)
    {
        return to_term_impl(env, items, std::index_sequence_for<Args...>{});
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
                throw std::invalid_argument("did not match any variant: " + format_term(term));
            else
                return type_cast<variant_type>::from_term_impl<I + 1, Rest...>(env, term);
        }
    }

public:
    constexpr static variant_type from_term(ErlNifEnv* env, ERL_NIF_TERM term)
    {
        return type_cast<std::variant<Args...>>::from_term_impl<0, Args...>(env, term);
    }

    constexpr static ERL_NIF_TERM to_term(ErlNifEnv* env, const variant_type& item)
    {
        return std::visit(
            [env](const auto& arg) {
                using T = std::decay_t<decltype(arg)>;
                return type_cast<T>::to_term(env, arg);
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
    static expected_type from_term(ErlNifEnv* env, ERL_NIF_TERM term)
    {
        const ERL_NIF_TERM* tuple = nullptr;
        int arity = 0;
        if (!enif_get_tuple(env, term, &arity, &tuple) || arity != 2)
            throw std::invalid_argument("expected {ok, value} or {error, reason}, got: " + format_term(term));

        if (detail::is_atom<"ok">(env, tuple[0]))
            return type_cast<T>::from_term(env, tuple[1]);
        if (detail::is_atom<"error">(env, tuple[0]))
            return std::unexpected(type_cast<E>::from_term(env, tuple[1]));

        throw std::invalid_argument("expected {ok, value} or {error, reason}, got: " + format_term(term));
    }

    static ERL_NIF_TERM to_term(ErlNifEnv* env, const expected_type& result)
    {
        if (result.has_value())
            return enif_make_tuple2(env, detail::static_atom<"ok">(env), type_cast<T>::to_term(env, *result));
        else
            return enif_make_tuple2(env, detail::static_atom<"error">(env), type_cast<E>::to_term(env, result.error()));
    };
};


template <typename T>
struct type_cast<std::optional<T>>
{
    static std::optional<T> from_term(ErlNifEnv* env, ERL_NIF_TERM term)
    {
        static_assert(!std::is_same_v<T, atom>, "std::optional cannot wrap an atom");

        if (detail::is_atom<"nil">(env, term))
            return std::nullopt;
        return type_cast<T>::from_term(env, term);
    }

    static ERL_NIF_TERM to_term(ErlNifEnv* env, const std::optional<T>& item)
    {
        if (item)
            return type_cast<T>::to_term(env, *item);
        else
            return detail::static_atom<"nil">(env);
    }
};


template <typename T>
struct type_cast<resource<T>>
{
    static resource<T> from_term(ErlNifEnv* env, ERL_NIF_TERM term)
    {
        return resource<T>{env, term};
    }

    static ERL_NIF_TERM to_term(ErlNifEnv* env, const resource<T>& res)
    {
        // Just create the term — the resource<T> destructor will call
        // enif_release_resource when the object goes out of scope,
        // leaving the term as the sole owner of the NIF resource.
        return enif_make_resource(env, res.objp);
    }
};
}  // namespace expp
