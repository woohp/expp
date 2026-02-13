#pragma once
#include "casts.hpp"
#include "ext_types.hpp"
#include "generator.hpp"
#include "resource.hpp"
#include "yielding.hpp"
#include <erl_nif.h>
#include <memory>
#include <optional>
#include <utility>


template <typename T>
struct function_traits;

template <typename R, typename... Args, bool IsNoexcept>
struct function_traits<R (*)(Args...) noexcept(IsNoexcept)>
{
    using func_type = R(Args...) noexcept(IsNoexcept);
    using return_type = R;
    static constexpr size_t nargs = sizeof...(Args);

    template <func_type fn, std::size_t... I>
    constexpr static R apply_impl(ErlNifEnv* env, const ERL_NIF_TERM argv[], std::index_sequence<I...>)
    {
        return fn(type_cast<std::decay_t<Args>>::from_term(env, argv[I])...);
    }

    template <func_type fn>
    constexpr static R apply(ErlNifEnv* env, const ERL_NIF_TERM argv[])
    {
        return apply_impl<fn>(env, argv, std::make_index_sequence<nargs> {});
    }

    constexpr static bool any_args_by_reference()
    {
        return (... || std::is_reference_v<Args>);
    }

    template <typename U>
    constexpr static bool any_args_has_type()
    {
        return (... || std::is_same_v<Args, U>);
    }
};


inline ERL_NIF_TERM coroutine_step(ErlNifEnv* env, int, const ERL_NIF_TERM argv[])
{
    auto coroutine_resource = type_cast<yielding_resource_t>::from_term(env, argv[0]);
    auto& impl_ptr = coroutine_resource.get();

    if (ERL_NIF_TERM step_result = impl_ptr->step(env); step_result)
        return step_result;
    else
        return enif_schedule_nif(env, "coroutine_step", 0, coroutine_step, 1, argv);
}


template <auto fn>
constexpr ERL_NIF_TERM wrapper(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[])
{
    using func_traits = function_traits<decltype(fn)>;

    if (argc != func_traits::nargs)
        return enif_make_badarg(env);

    try
    {
        auto ret = func_traits::template apply<fn>(env, argv);

        using return_type = typename func_traits::return_type;  // aka GeneratorType
        if constexpr (is_yielding_v<return_type>)
        {
            // Do some type-checking to make sure we don't run into trouble:
            // Because generator functions can be resumed, they cannot take
            // 1. arguments by reference (the original stack is gone when it's resumed)
            // 2. arguments of type binary (the data pointer might be invalidated when it's resumed)
            static_assert(
                !func_traits::any_args_by_reference(), "generator functions cannot have pass-by-reference arguments");
            static_assert(
                !func_traits::template any_args_has_type<binary>(),
                "generator functions cannot have arguments of type binary");

            // Wrap the generator in a type-erased impl
            auto impl = std::make_unique<yielding_resource_impl<return_type>>(std::move(ret));

            // Try to step the generator one time
            if (auto step_output = impl->step(env); step_output)
                return step_output;
            else
            {
                // Allocate a resource for the generator and schedule it for later execution
                auto res = yielding_resource_t::alloc(std::move(impl));
                ERL_NIF_TERM resource_term = type_cast<yielding_resource_t>::to_term(env, res);
                ERL_NIF_TERM out[] = { resource_term };
                return enif_schedule_nif(env, "coroutine_step", 0, coroutine_step, 1, out);
            }
        }
        else
        {
            return type_cast<std::decay_t<decltype(ret)>>::to_term(env, std::move(ret));
        }
    }
    catch (const std::invalid_argument& e)
    {
        return enif_make_badarg(env);
    }
    catch (const erl_error_base& e)
    {
        return e.get_term(env);
    }
    catch (const std::exception& e)
    {
        auto reason = type_cast<std::string>::to_term(env, e.what());
        return enif_raise_exception(env, reason);
    }
}


enum class DirtyFlags
{
    NotDirty = 0,
    DirtyCpu = ERL_NIF_DIRTY_JOB_CPU_BOUND,
    DirtyIO = ERL_NIF_DIRTY_JOB_IO_BOUND,
};


template <auto fn, DirtyFlags dirty_flag>
consteval ErlNifFunc def_impl(const char* name)
{
    ErlNifFunc entry = {
        name,
        function_traits<decltype(fn)>::nargs,
        wrapper<fn>,
        static_cast<int>(dirty_flag),
    };
    return entry;
}


/*
macro overloading trick:
https://stackoverflow.com/questions/11761703/overloading-macro-on-number-of-arguments
We want to be able to write:

    def(add, "add)
    def(add)  // defaults to using the same name as the function
*/
#define DEF2(fn, dirty_flag) def_impl<fn, dirty_flag>(#fn)
#define DEF3(fn, name, dirty_flag) def_impl<fn, dirty_flag>(name)
#define GET_MACRO(_1, _2, _3, NAME, ...) NAME
#define def(...) GET_MACRO(__VA_ARGS__, DEF3, DEF2, UNUSED)(__VA_ARGS__)


#define MODULE(NAME, LOAD, UPGRADE, UNLOAD, ...)                                                                       \
    ErlNifFunc _nif_funcs[] = { __VA_ARGS__ };                                                                         \
    ERL_NIF_INIT(NAME, _nif_funcs, LOAD, nullptr, UPGRADE, UNLOAD)
