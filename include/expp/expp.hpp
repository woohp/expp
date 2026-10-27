#pragma once
#include "casts.hpp"
#include "ext_types.hpp"
#include "generator.hpp"
#include "gleam.hpp"
#include "process.hpp"
#include "resource.hpp"
#include "stl.hpp"
#include "yielding.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <erl_nif.h>
#include <memory>
#include <meta>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>


namespace expp
{
template <typename T>
struct function_traits;


namespace detail
{
template <typename... Args>
inline constexpr bool first_is_env = false;

template <typename First, typename... Rest>
inline constexpr bool first_is_env<First, Rest...> = std::same_as<First, ErlNifEnv*>;
}  // namespace detail


// A NIF function may take the caller's ErlNifEnv* as its first parameter, which the
// wrapper passes in and which doesn't count toward the NIF's arity.
template <typename R, typename... Args, bool IsNoexcept>
struct function_traits<R (*)(Args...) noexcept(IsNoexcept)>
{
    using func_type = R(Args...) noexcept(IsNoexcept);
    using return_type = R;
    using decayed_args = std::tuple<std::decay_t<Args>...>;
    static constexpr bool takes_env = detail::first_is_env<Args...>;
    static constexpr size_t nargs = sizeof...(Args) - takes_env;

    template <func_type fn, std::size_t... I>
    constexpr static R apply_impl(
        [[maybe_unused]] ErlNifEnv* env, [[maybe_unused]] const ERL_NIF_TERM argv[], std::index_sequence<I...>)
    {
        if constexpr (takes_env)
            return fn(env, type_cast<std::tuple_element_t<I + 1, decayed_args>>::from_term(env, argv[I])...);
        else
            return fn(type_cast<std::tuple_element_t<I, decayed_args>>::from_term(env, argv[I])...);
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


// Annotation exporting a function from nif_entry, on the given scheduler, under its own name or
// another one:
//
//     [[=nif::not_dirty]] int add(int a, int b);
//     [[=nif::dirty_cpu.named("add")]] int add_impl(int a, int b);
//
// Repeat it to export one function under several names.
struct nif
{
    DirtyFlags dirty_flags;
    const char* name = nullptr;

    static const nif not_dirty;
    static const nif dirty_cpu;
    static const nif dirty_io;

    // An annotation can't point into a string literal, so the name is copied into static storage.
    consteval nif named(std::string_view new_name) const
    {
        return {dirty_flags, std::define_static_string(new_name)};
    }
};

inline constexpr nif nif::not_dirty{DirtyFlags::NotDirty};
inline constexpr nif nif::dirty_cpu{DirtyFlags::DirtyCpu};
inline constexpr nif nif::dirty_io{DirtyFlags::DirtyIO};


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
                "string_view, binary, term, ErlNifEnv*, pointers, and containers wrapping them "
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


namespace detail
{
// nif_entry's load callback: opens the resource types, then runs the user's callback, if any
// (load may be nullptr, NULL, or a null function pointer). An exception fails the load
// instead of escaping into the VM.
template <auto user_load>
int on_load(ErlNifEnv* env, void** priv_data, ERL_NIF_TERM load_info) noexcept
{
    try
    {
        open_resource_types(env);
        if constexpr (user_load == decltype(user_load){})
            return 0;
        else
            return user_load(env, priv_data, load_info);
    }
    catch (const std::exception& e)
    {
        enif_fprintf(stderr, "expp: NIF load callback failed: %s\n", e.what());
    }
    catch (...)
    {
        enif_fprintf(stderr, "expp: NIF load callback failed with an unknown exception\n");
    }
    return -1;
}


struct nif_decl
{
    std::meta::info fn;
    nif annotation;
    const char* name;
};


// The NIF's arity, which doesn't count a leading ErlNifEnv*, matching function_traits::nargs.
consteval std::size_t nif_arity(std::meta::info fn)
{
    const auto params = std::meta::parameters_of(fn);
    const bool takes_env = !params.empty() && std::meta::dealias(std::meta::type_of(params[0])) == ^^ErlNifEnv*;
    return params.size() - takes_env;
}


consteval std::vector<nif_decl> nif_decls(std::meta::info ns)
{
    std::vector<nif_decl> decls;
    for (auto member : std::meta::members_of(ns, std::meta::access_context::current()))
    {
        if (!std::meta::is_function(member))
            continue;
        for (auto annotation : std::meta::annotations_of_with_type(member, ^^nif))
        {
            const auto a = std::meta::extract<nif>(annotation);
            const auto name = a.name ? a.name : std::define_static_string(std::meta::identifier_of(member));
            for (const auto& d : decls)
                if (std::string_view(d.name) == name && nif_arity(d.fn) == nif_arity(member))
                    throw std::invalid_argument(std::string("NIF exported twice with the same arity: ") + name);
            decls.push_back({member, a, name});
        }
    }
    return decls;
}


template <std::meta::info ns>
inline auto nif_funcs = [] {
    static constexpr auto decls = std::define_static_array(nif_decls(ns));
    std::array<ErlNifFunc, decls.size()> funcs;
    std::size_t i = 0;
    template for (constexpr nif_decl d : decls)
    {
        constexpr auto fn = &[:d.fn:];
        constexpr auto dirty_flags = d.annotation.dirty_flags;
        funcs[i++] = {d.name, nif_arity(d.fn), wrapper<fn, dirty_flags>, static_cast<int>(dirty_flags)};
    }
    return funcs;
}();
}  // namespace detail


// Returns the entry the VM loads a NIF library through, exporting every function directly in
// namespace ns annotated with [[=nif::...]]. Return it from the library's nif_init:
//
//     extern "C" [[gnu::visibility("default")]] ErlNifEntry* nif_init()
//     {
//         return expp::nif_entry<^^my_nifs, load, unload>("Elixir.MyMod");
//     }
template <std::meta::info ns, auto load = nullptr, auto unload = nullptr>
ErlNifEntry* nif_entry(const char* module_name)
{
    auto& funcs = detail::nif_funcs<ns>;
    static ErlNifEntry entry = {
        .major = ERL_NIF_MAJOR_VERSION,
        .minor = ERL_NIF_MINOR_VERSION,
        .name = module_name,
        .num_of_funcs = static_cast<int>(funcs.size()),
        .funcs = funcs.data(),
        .load = detail::on_load<load>,
        .reload = nullptr,
        .upgrade = nullptr,
        .unload = unload,
        .vm_variant = ERL_NIF_VM_VARIANT,
        .options = 1,
        .sizeof_ErlNifResourceTypeInit = sizeof(ErlNifResourceTypeInit),
        .min_erts = ERL_NIF_MIN_ERTS_VERSION,
    };
    return &entry;
}
}  // namespace expp
