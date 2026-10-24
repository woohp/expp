#pragma once
#include <concepts>
#include <erl_nif.h>
#include <new>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>


namespace expp
{
// A reference-counted handle to a NIF resource, like std::shared_ptr but sharing the
// VM's count: each handle holds one reference, so handles can be copied, stored in
// other objects, and kept across calls. T lives in the resource memory, so returning
// the same object again yields the same Erlang reference.
template <typename T>
    requires std::destructible<T>
class resource
{
    // The resource memory holds an optional, so if T's constructor throws in
    // alloc, the destructor callback finds it empty and skips ~T().
    using slot = std::optional<T>;

    slot* objp;

    friend struct type_cast<resource<T>>;

    // Adopts a reference the caller already holds.
    explicit resource(void* objp) :
        objp(static_cast<slot*>(objp))
    {}

public:
    typedef T type;

    resource(const resource<T>& other) :
        objp(other.objp)
    {
        if (objp)
            enif_keep_resource(objp);
    }

    resource(resource<T>&& other) noexcept :
        objp(std::exchange(other.objp, nullptr))
    {}

    resource<T>& operator=(resource<T> other) noexcept
    {
        std::swap(objp, other.objp);
        return *this;
    }

    ~resource()
    {
        if (objp)
            enif_release_resource(objp);
    }

    T& get() const
    {
        return **objp;
    }

    T& operator*() const
    {
        return get();
    }

    T* operator->() const
    {
        return &get();
    }

    template <typename... Args>
        requires std::constructible_from<T, Args...>
    static resource<T> alloc(Args&&... args)
    {
        if (!resource<T>::resource_type)
            throw std::runtime_error(
                "NIF resource type not initialized; call resource<T>::init(env, name) in the load callback "
                "(yielding NIFs need yielding_resource_t::init)");

        void* buf = enif_alloc_resource(resource<T>::resource_type, sizeof(slot));
        if (!buf)
            throw std::bad_alloc{};

        // Owning the resource first releases it if the constructor throws.
        resource<T> res{new (buf) slot};
        res.objp->emplace(std::forward<Args>(args)...);
        return res;
    }

    static void init(ErlNifEnv* env, const char* name)
    {
        resource<T>::resource_type =
            enif_open_resource_type(env, nullptr, name, resource<T>::destructor, ERL_NIF_RT_CREATE, nullptr);
        if (!resource<T>::resource_type)
            throw std::runtime_error(std::string("failed to open NIF resource type: ") + name);
    }

    static void destructor(ErlNifEnv*, void* objp)
    {
        static_cast<slot*>(objp)->~slot();
    }

    static ErlNifResourceType* resource_type;
};


template <typename T>
    requires std::destructible<T>
ErlNifResourceType* resource<T>::resource_type = nullptr;
}  // namespace expp
