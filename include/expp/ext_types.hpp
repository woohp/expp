#pragma once
#include "casts.hpp"
#include <erl_nif.h>
#include <stdexcept>


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
        return enif_make_tuple2(
            env, detail::static_atom<"error">(env), type_cast<std::decay_t<T>>::to_term(env, error_value));
    }
};


namespace exceptions
{
inline ERL_NIF_TERM raise_error_with_message(ErlNifEnv* env, const char* module_name, std::string_view message)
{
    ERL_NIF_TERM keys[3] = {
        detail::static_atom<"__struct__">(env),
        detail::static_atom<"__exception__">(env),
        detail::static_atom<"message">(env),
    };
    ERL_NIF_TERM values[3] = {
        enif_make_atom(env, module_name),
        detail::static_atom<"true">(env),
        type_cast<std::string_view>::to_term(env, message)
    };

    ERL_NIF_TERM map;
    enif_make_map_from_arrays(env, keys, values, 3, &map);
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

// Converts the exception being handled into the NIF's result; call it from a catch block.
inline ERL_NIF_TERM from_current_exception(ErlNifEnv* env)
{
    try
    {
        throw;
    }
    catch (const std::invalid_argument& e)
    {
        return raise_argument_error(env, e.what());
    }
    catch (const erl_error_base& e)
    {
        return e.get_term(env);
    }
    catch (const std::exception& e)
    {
        return raise_runtime_error(env, e.what());
    }
    catch (...)
    {
        return raise_runtime_error(env, "unknown exception");
    }
}
}  // namespace exceptions
}  // namespace expp
