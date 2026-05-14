#pragma once
#include "casts.hpp"
#include "ext_types.hpp"
#include "generator.hpp"
#include "resource.hpp"
#include <chrono>
#include <memory>
#include <type_traits>


namespace expp
{
// Customization point: users must specialize this to true_type for custom
// decoded types whose internal data is fully owned (not borrowed from the
// Erlang heap). The default is false_type (fail-closed) so unknown types
// are conservatively rejected in yielding NIF arguments.
template <typename T>
struct is_yield_persistent : std::false_type
{};


namespace detail
{
template <typename T>
struct is_resource_impl : std::false_type
{};

template <typename T>
struct is_resource_impl<resource<T>> : std::true_type
{};
}  // namespace detail


// consteval check: true if T owns its data and is safe to persist
// across a yielding NIF suspension. The default is false (fail-closed);
// only known-safe types (scalars, fully-owned containers of scalars,
// and user types with is_yield_persistent<T> = true_type) pass.
// Containers are decomposed generically via tuple_size, variant_size,
// and value_type rather than enumerating every template.
template <typename T>
consteval bool is_yield_safe()
{
    using U = std::remove_cvref_t<T>;

    // Known unsafe types that borrow from the Erlang heap
    if constexpr (std::same_as<U, std::string_view> || std::same_as<U, binary> || std::same_as<U, term>)
        return false;

    // resource<T> is tied to a specific NIF call environment
    if constexpr (detail::is_resource_impl<U>::value)
        return false;

    // Container with value_type (vector, optional, array, map, etc.): check element type.
    // This is checked before tuple_size so that homogeneous containers (e.g. array<T,N>)
    // don't redundantly iterate every element.
    if constexpr (requires { typename U::value_type; })
        return is_yield_safe<typename U::value_type>();

    // Tuple-like (pair, tuple): check each element individually
    if constexpr (requires { std::tuple_size<U>::value; })
    {
        return []<std::size_t... I>(std::index_sequence<I...>) {
            return (is_yield_safe<std::tuple_element_t<I, U>>() && ...);
        }(std::make_index_sequence<std::tuple_size_v<U>>());
    }

    // Variant: check each alternative
    if constexpr (requires { std::variant_size<U>::value; })
    {
        return []<std::size_t... I>(std::index_sequence<I...>) {
            return (is_yield_safe<std::variant_alternative_t<I, U>>() && ...);
        }(std::make_index_sequence<std::variant_size_v<U>>());
    }

    // Scalars (int, float, bool, enums, pointers) are always owned by value
    // and safe to persist across suspension.
    if constexpr (std::is_scalar_v<U>)
        return true;

    // Fall through to the user customization point (default false — fail-closed).
    return is_yield_persistent<U>::value;
}


// A yielding type is a generator that returns an optional of the underlying type.
// If it yields nullopt, then the next nif execution will be scheduled, otherwise, that thing is returned to the caller.
template <typename T>
using yielding = cppcoro::generator<std::optional<T>>;


template <typename T>
struct is_yielding : std::false_type
{};


template <typename T>
struct is_yielding<cppcoro::generator<std::optional<T>>> : std::true_type
{};


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
    std::optional<typename GeneratorType::iterator> it;

    explicit yielding_resource_impl(GeneratorType&& c)
        : coro(std::move(c))
    {}

    ERL_NIF_TERM step(ErlNifEnv* env) override
    {
        try
        {
            if (!it)
                it = coro.begin();
            else
                ++(*it);

            if (*it == coro.end())
                return exceptions::raise_runtime_error(env, "yielding NIF ended without final result");

            const auto& out = **it;
            if (out)
                return type_cast<std::decay_t<decltype(*out)>>::to_term(env, *out);
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
}  // namespace expp
