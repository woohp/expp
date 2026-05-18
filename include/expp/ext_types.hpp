#pragma once
#include "atom.hpp"
#include "casts.hpp"
#include <erl_nif.h>
#include <stdexcept>
#include <tuple>
#include <variant>


namespace expp
{
struct erl_error_base : std::exception
{
    virtual ERL_NIF_TERM get_term(ErlNifEnv* env) const = 0;
};


// this exception is automatically converted to {:error, <error_value>}
template <std::copy_constructible T>
struct erl_error : erl_error_base
{
    T error_value;

    constexpr explicit erl_error(const T& error_value) :
        error_value(error_value)
    {}

    ERL_NIF_TERM get_term(ErlNifEnv* env) const
    {
        using error_type = std::tuple<atom, std::decay_t<T>>;
        return type_cast<error_type>::to_term(env, error_type("error"_atom, error_value));
    }
};


namespace exceptions
{
inline ERL_NIF_TERM raise_error_with_message(ErlNifEnv* env, const char* module_name, std::string_view message)
{
    ERL_NIF_TERM keys[3] = {
        enif_make_atom(env, "__struct__"), enif_make_atom(env, "__exception__"), enif_make_atom(env, "message")
    };
    ERL_NIF_TERM values[3] = {
        enif_make_atom(env, module_name),
        enif_make_atom(env, "true"),
        type_cast<std::string_view>::to_term(env, message)
    };

    ERL_NIF_TERM map;
    if (!enif_make_map_from_arrays(env, keys, values, 3, &map))
    {
        return enif_raise_exception(env, type_cast<std::string_view>::to_term(env, message));
    }

    return enif_raise_exception(env, map);
}

inline ERL_NIF_TERM raise_argument_error(ErlNifEnv* env, std::string_view message)
{
    return raise_error_with_message(env, "Elixir.ArgumentError", message);
}

inline ERL_NIF_TERM raise_runtime_error(ErlNifEnv* env, std::string_view message)
{
    return raise_error_with_message(env, "Elixir.RuntimeError", message);
}
}  // namespace exceptions
}  // namespace expp
