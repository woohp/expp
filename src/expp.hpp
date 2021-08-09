#pragma once
#include "casts.hpp"
#include "ext_types.hpp"
#include "generator.hpp"
#include "resource.hpp"
#include <erl_nif.h>
#include <optional>
#include <utility>


template <typename T>
struct is_generator
{
    static const bool value = false;
};


template <typename T>
struct is_generator<cppcoro::generator<std::optional<T>>>
{
    static const bool value = true;
};


template <typename T>
inline constexpr bool is_generator_v = is_generator<T>::value;


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
        return fn(type_cast<std::decay_t<Args>>::load(env, argv[I])...);
    }

    template <func_type fn>
    constexpr static R apply(ErlNifEnv* env, const ERL_NIF_TERM argv[])
    {
        return apply_impl<fn>(env, argv, std::make_index_sequence<nargs> {});
    }
};


using generator_resource_t = resource<cppcoro::generator<std::optional<int>>>;


template <typename GeneratorType>
ERL_NIF_TERM coroutine_step(ErlNifEnv* env, int, const ERL_NIF_TERM argv[])
{
    auto coroutine_resource = type_cast<generator_resource_t>::load(env, argv[0]);
    auto& coro = coroutine_resource.get<GeneratorType>();
    auto it = std::begin(coro);
    auto& out = *it;

    if (out)
        return type_cast<std::decay_t<decltype(*out)>>::handle(env, *out);
    else
        return enif_schedule_nif(env, "coroutine_step", 0, coroutine_step<GeneratorType>, 1, argv);
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
        if constexpr (is_generator_v<return_type>)
        {
            void* buf = enif_alloc_resource(generator_resource_t::resource_type, sizeof(ret));
            new (buf) decltype(ret) { std::move(ret) };
            ERL_NIF_TERM out[] = { enif_make_resource(env, buf) };
            return enif_schedule_nif(env, "coroutine_step", 0, coroutine_step<return_type>, 1, out);
        }
        else
        {
            return type_cast<std::decay_t<decltype(ret)>>::handle(env, std::move(ret));
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
        auto reason = type_cast<std::string>::handle(env, e.what());
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
constexpr ErlNifFunc def_impl(const char* name)
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
