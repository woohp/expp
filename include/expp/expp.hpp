#pragma once
#include "casts.hpp"
#include "ext_types.hpp"
#include "generator.hpp"
#include "gleam.hpp"
#include "resource.hpp"
#include "yielding.hpp"
#include <algorithm>
#include <chrono>
#include <erl_nif.h>
#include <memory>
#include <optional>
#include <utility>


namespace expp
{
template <typename T>
struct function_traits;

template <typename R, typename... Args, bool IsNoexcept>
struct function_traits<R (*)(Args...) noexcept(IsNoexcept)>
{
    using func_type = R(Args...) noexcept(IsNoexcept);
    using return_type = R;
    static constexpr size_t nargs = sizeof...(Args);

    template <func_type fn, std::size_t... I>
    constexpr static R apply_impl(
        [[maybe_unused]] ErlNifEnv* env, [[maybe_unused]] const ERL_NIF_TERM argv[], std::index_sequence<I...>)
    {
        return fn(type_cast<std::decay_t<Args>>::from_term(env, argv[I])...);
    }

    template <func_type fn>
    constexpr static R apply(ErlNifEnv* env, const ERL_NIF_TERM argv[])
    {
        return apply_impl<fn>(env, argv, std::make_index_sequence<nargs>{});
    }

    constexpr static bool any_args_by_reference()
    {
        return (... || std::is_reference_v<Args>);
    }

    constexpr static bool all_args_yield_safe()
    {
        return (... && expp::is_yield_safe<Args>());
    }
};


enum class DirtyFlags
{
    NotDirty = 0,
    DirtyCpu = ERL_NIF_DIRTY_JOB_CPU_BOUND,
    DirtyIO = ERL_NIF_DIRTY_JOB_IO_BOUND,
};


// The coroutine whose step is running on this thread, so schedule_on can reach it.
inline thread_local yielding_resource_base* running_coroutine = nullptr;


// Suspends a yielding NIF like `co_yield std::nullopt`, and runs its following steps on the
// given scheduler, e.g. to move an indivisible, long call to a dirty scheduler and back.
// Only valid inside a yielding NIF: `co_yield expp::schedule_on(DirtyFlags::DirtyCpu);`
inline std::nullopt_t schedule_on(DirtyFlags flags)
{
    running_coroutine->dirty_flags = static_cast<int>(flags);
    return std::nullopt;
}


// Runs one step of a coroutine, returning its result, or nullopt if it suspended.
inline std::optional<ERL_NIF_TERM> run_step(ErlNifEnv* env, yielding_resource_base& coroutine)
{
    const bool on_normal_scheduler = coroutine.dirty_flags == static_cast<int>(DirtyFlags::NotDirty);
    const auto start_time = std::chrono::steady_clock::now();

    running_coroutine = &coroutine;
    const std::optional<ERL_NIF_TERM> result = coroutine.step(env);
    running_coroutine = nullptr;

    // Report the share of a normal scheduler's ~1 ms timeslice that the step used.
    if (on_normal_scheduler)
    {
        const auto elapsed = std::chrono::steady_clock::now() - start_time;
        const auto percent = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count() / 10;
        enif_consume_timeslice(env, static_cast<int>(std::clamp<decltype(percent)>(percent, 1, 100)));
    }
    return result;
}


inline ERL_NIF_TERM coroutine_step(ErlNifEnv* env, int, const ERL_NIF_TERM argv[])
{
    auto coroutine_resource = type_cast<yielding_resource_t>::from_term(env, argv[0]);
    auto& impl_ptr = coroutine_resource.get();

    if (auto step_result = run_step(env, *impl_ptr))
        return *step_result;
    else if (impl_ptr->is_exhausted())
        return exceptions::raise_runtime_error(env, "yielding NIF ended without final result");
    else
        return enif_schedule_nif(env, "coroutine_step", impl_ptr->dirty_flags, coroutine_step, 1, argv);
}


template <auto fn, DirtyFlags dirty_flags>
constexpr ERL_NIF_TERM wrapper(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[])
{
    using func_traits = function_traits<decltype(fn)>;

    if (argc != func_traits::nargs)
        return enif_make_badarg(env);

    try
    {
        using return_type = typename func_traits::return_type;

        if constexpr (std::is_void_v<return_type>)
        {
            func_traits::template apply<fn>(env, argv);
            return detail::static_atom<"ok">(env);
        }
        else if constexpr (is_yielding_v<return_type>)
        {
            auto ret = func_traits::template apply<fn>(env, argv);

            // Generator functions can be resumed after suspension, so every
            // argument must be fully owned (not borrowed from the Erlang heap).
            // References are invalid because the original stack frame is gone.
            static_assert(
                !func_traits::any_args_by_reference(), "generator functions cannot have pass-by-reference arguments");
            static_assert(
                func_traits::all_args_yield_safe(),
                "generator function arguments must be fully owned across suspension; "
                "string_view, binary, term, and containers wrapping them "
                "are not allowed. Specialize expp::is_yield_persistent<T> = std::true_type "
                "for fully-owned custom decoded types.");

            auto impl = std::make_unique<yielding_resource_impl<return_type>>(std::move(ret));
            impl->dirty_flags = static_cast<int>(dirty_flags);

            if (auto step_output = run_step(env, *impl))
                return *step_output;
            else if (impl->is_exhausted())
                return exceptions::raise_runtime_error(env, "yielding NIF must contain at least one co_yield");
            else
            {
                const int next_dirty_flags = impl->dirty_flags;
                auto res = yielding_resource_t::alloc(std::move(impl));
                ERL_NIF_TERM resource_term = type_cast<yielding_resource_t>::to_term(env, res);
                ERL_NIF_TERM argv[] = {resource_term};
                return enif_schedule_nif(env, "coroutine_step", next_dirty_flags, coroutine_step, 1, argv);
            }
        }
        else
        {
            auto ret = func_traits::template apply<fn>(env, argv);
            return type_cast<std::decay_t<decltype(ret)>>::to_term(env, std::move(ret));
        }
    }
    catch (...)
    {
        return exceptions::from_current_exception(env);
    }
}


template <auto fn, DirtyFlags dirty_flag>
consteval ErlNifFunc def_impl(const char* name)
{
    ErlNifFunc entry = {
        name,
        function_traits<decltype(fn)>::nargs,
        wrapper<fn, dirty_flag>,
        static_cast<int>(dirty_flag),
    };
    return entry;
}


template <auto callback, typename... Args>
int guarded_callback(Args... args) noexcept
{
    try
    {
        return callback(args...);
    }
    catch (const std::exception& e)
    {
        enif_fprintf(stderr, "expp: NIF load/upgrade callback failed: %s\n", e.what());
    }
    catch (...)
    {
        enif_fprintf(stderr, "expp: NIF load/upgrade callback failed with an unknown exception\n");
    }
    return -1;
}


// Wraps a load or upgrade callback so an exception (e.g. from resource<T>::init) fails
// the load instead of escaping into the VM. A null callback (nullptr, NULL, or a null
// function pointer) stays null, since a null upgrade callback tells the VM that upgrades
// are unsupported.
template <auto callback>
consteval auto guard()
{
    if constexpr (callback == decltype(callback){})
        return nullptr;
    else
        return []<typename... Args>(int (*)(Args...)) { return &guarded_callback<callback, Args...>; }(callback);
}
}  // namespace expp


/*
macro overloading trick:
https://stackoverflow.com/questions/11761703/overloading-macro-on-number-of-arguments
We want to be able to write:

    def(add, DirtyFlags::NotDirty)                // named after the function
    def(add_impl, "add", DirtyFlags::NotDirty)    // explicit name
*/
#define DEF2(fn, dirty_flag) expp::def_impl<fn, dirty_flag>(#fn)
#define DEF3(fn, name, dirty_flag) expp::def_impl<fn, dirty_flag>(name)
#define GET_MACRO(_1, _2, _3, NAME, ...) NAME
#define def(...) GET_MACRO(__VA_ARGS__, DEF3, DEF2, UNUSED)(__VA_ARGS__)


#define MODULE(NAME, LOAD, UPGRADE, UNLOAD, ...)                                                                       \
    ErlNifFunc _nif_funcs[] = {__VA_ARGS__};                                                                           \
    ERL_NIF_INIT(NAME, _nif_funcs, expp::guard<LOAD>(), nullptr, expp::guard<UPGRADE>(), UNLOAD)
