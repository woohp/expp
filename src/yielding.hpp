#pragma once
#include "casts.hpp"
#include "ext_types.hpp"
#include "generator.hpp"
#include "resource.hpp"
#include <chrono>
#include <memory>


namespace expp
{
// A yielding type is a generator that returns an optional of the underlying type.
// If it yields nullopt, then the next nif execution will be scheduled, otherwise, that thing is returned to the caller.
template <typename T>
using yielding = cppcoro::generator<std::optional<T>>;


template <typename T>
struct is_yielding : std::false_type
{ };


template <typename T>
struct is_yielding<cppcoro::generator<std::optional<T>>> : std::true_type
{ };


template <typename T>
inline constexpr bool is_yielding_v = is_yielding<T>::value;


// a simple timer for knowing when to yield back to the erlang runtime
struct yielding_timer
{
    std::chrono::time_point<std::chrono::steady_clock> start_time;

    yielding_timer()
    {
        this->reset();
    }

    void reset()
    {
        this->start_time = std::chrono::steady_clock::now();
    }

    bool times_up() const
    {
        using namespace std;
        return chrono::duration_cast<chrono::microseconds>(chrono::steady_clock::now() - start_time).count() >= 990;
    }
};


// Type-erased base for yielding coroutine resources, enabling virtual dispatch
// instead of type-punning through resource<yielding<int>>.
struct yielding_resource_base
{
    // Dirty flag used when rescheduling continuations. Stored here so that
    // coroutine_step can propagate the same scheduler class as the initial call.
    int dirty_flags = 0;

    virtual ~yielding_resource_base() = default;
    virtual ERL_NIF_TERM step(ErlNifEnv* env) = 0;
};


template <typename GeneratorType>
struct yielding_resource_impl : yielding_resource_base
{
    GeneratorType coro;

    explicit yielding_resource_impl(GeneratorType&& c)
        : coro(std::move(c))
    { }

    ERL_NIF_TERM step(ErlNifEnv* env) override
    {
        try
        {
            if (const auto& out = *std::begin(coro); out)
            {
                return type_cast<std::decay_t<decltype(*out)>>::to_term(env, *out);
            }
            else
                return 0;  // indicates that it needs to be scheduled for another step
        }
        catch (const erl_error_base& e)
        {
            return e.get_term(env);
        }
        catch (const std::exception& e)
        {
            return exceptions::raise_runtime_error(env, e.what());
        }
    }
};


using yielding_resource_t = resource<std::unique_ptr<yielding_resource_base>>;
}
