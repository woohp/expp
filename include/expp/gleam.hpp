#pragma once
#include "casts.hpp"
#include <optional>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

namespace expp::gleam
{
// Gleam's `Option(T)` term shape: `none | {some, value}`.
// This intentionally differs from `std::optional<T>`, which expp keeps as
// the existing BEAM/Elixir-friendly `nil | value` representation.
template <typename T>
struct option
{
    using value_type = T;

    std::optional<T> value;

    option() = default;

    option(std::nullopt_t) :
        value(std::nullopt)
    {}

    option(T value) :
        value(std::move(value))
    {}

    option(std::optional<T> value) :
        value(std::move(value))
    {}

    explicit operator bool() const noexcept
    {
        return value.has_value();
    }

    T& operator*() &
    {
        return *value;
    }

    const T& operator*() const&
    {
        return *value;
    }
};

// A Gleam custom-type constructor case encoded as a tagged tuple:
// `{tag, arg1, arg2, ...}`. For example, `case_<"image", int, int>{640, 480}`
// encodes as `{image, 640, 480}`.
//
// Use this type when declaring a C++ variant that can return multiple Gleam
// constructors:
//
//     using result = std::variant<
//         gleam::case_<"found", int, std::string>,
//         gleam::case_<"missing", int>>;
//
// Prefer constructing values with `gleam::make_case<"found">(...)`.
template <detail::fixed_string Tag, typename... Args>
struct case_
{
    std::tuple<Args...> values;

    explicit case_(Args... args) :
        values(std::move(args)...)
    {}

    static constexpr std::string_view tag()
    {
        return Tag.view();
    }
};

// Convenience factory for Gleam constructor cases. Like `std::make_tuple`, this
// deduces and decays payload types so callers can pass literals, references,
// and temporaries while storing an owned value that can safely be converted to a
// BEAM term.
template <detail::fixed_string Tag, typename... Args>
case_<Tag, std::decay_t<Args>...> make_case(Args&&... args)
{
    return case_<Tag, std::decay_t<Args>...>(std::forward<Args>(args)...);
}
}  // namespace expp::gleam

namespace expp
{
// Converts between `expp::gleam::option<T>` and Gleam's stdlib Option encoding.
template <typename T>
struct type_cast<gleam::option<T>>
{
    static gleam::option<T> from_term(ErlNifEnv* env, ERL_NIF_TERM term)
    {
        if (detail::is_atom<"none">(env, term))
            return std::nullopt;

        const ERL_NIF_TERM* tuple = nullptr;
        int arity = 0;
        if (!enif_get_tuple(env, term, &arity, &tuple) || arity != 2)
            throw std::invalid_argument("expected Gleam Option none or {some, value}, got: " + format_term(term));

        if (!detail::is_atom<"some">(env, tuple[0]))
            throw std::invalid_argument("expected Gleam Option {some, value}, got: " + format_term(term));

        return gleam::option<T>{type_cast<T>::from_term(env, tuple[1])};
    }

    static ERL_NIF_TERM to_term(ErlNifEnv* env, const gleam::option<T>& item)
    {
        if (!item.value)
            return detail::static_atom<"none">(env);
        return enif_make_tuple2(env, detail::static_atom<"some">(env), type_cast<T>::to_term(env, *item.value));
    }
};

// Converts a Gleam constructor case to its BEAM tuple representation. Decoding
// is intentionally not implemented yet; use `expp::term` or a custom
// `type_cast` specialization if a NIF needs to accept arbitrary Gleam ADTs.
template <detail::fixed_string Tag, typename... Args>
struct type_cast<gleam::case_<Tag, Args...>>
{
private:
    using case_type = gleam::case_<Tag, Args...>;

    template <std::size_t... I>
    static ERL_NIF_TERM to_term_impl(ErlNifEnv* env, const case_type& item, std::index_sequence<I...>)
    {
        return enif_make_tuple(
            env,
            sizeof...(Args) + 1,
            detail::static_atom<Tag>(env),
            type_cast<std::tuple_element_t<I, std::tuple<Args...>>>::to_term(env, std::get<I>(item.values))...);
    }

public:
    static ERL_NIF_TERM to_term(ErlNifEnv* env, const case_type& item)
    {
        return to_term_impl(env, item, std::index_sequence_for<Args...>{});
    }
};
}  // namespace expp
