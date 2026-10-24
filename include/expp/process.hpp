#pragma once
#include "casts.hpp"
#include <erl_nif.h>
#include <memory>
#include <new>
#include <stdexcept>
#include <type_traits>


namespace expp
{
template <>
struct type_cast<ErlNifPid>
{
    static ErlNifPid from_term(ErlNifEnv* env, ERL_NIF_TERM term)
    {
        ErlNifPid pid;
        if (!enif_is_pid(env, term))
            throw std::invalid_argument("expected a pid, got: " + format_term(term));
        if (!enif_get_local_pid(env, term, &pid))
            throw std::invalid_argument("expected a local pid, got a remote one: " + format_term(term));
        return pid;
    }

    static ERL_NIF_TERM to_term(ErlNifEnv* env, const ErlNifPid& pid) noexcept
    {
// enif_make_pid is a macro that casts to a const-qualified type.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
        return enif_make_pid(env, &pid);
#pragma GCC diagnostic pop
    }
};


// Sends msg to a local process, returning false if it isn't alive. Pass the env of the
// calling NIF, or nullptr when sending from a thread the VM didn't create.
template <typename T>
bool send(ErlNifEnv* caller_env, const ErlNifPid& to, const T& msg)
{
    const std::unique_ptr<ErlNifEnv, decltype(&enif_free_env)> msg_env{enif_alloc_env(), enif_free_env};
    if (!msg_env)
        throw std::bad_alloc{};
    const ERL_NIF_TERM term = type_cast<std::decay_t<T>>::to_term(msg_env.get(), msg);
    return enif_send(caller_env, &to, msg_env.get(), term);
}
}  // namespace expp
