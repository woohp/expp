#pragma once
#include <concepts>
#include <erl_nif.h>
#include <memory>
#include <new>
#include <optional>
#include <stdexcept>
#include <string>
#include <typeinfo>
#include <utility>
#include <vector>


namespace expp
{
namespace detail
{
struct resource_registration
{
    ErlNifResourceType** type;
    const char* name;
    ErlNifResourceDtor* destructor;
};


// Per-library state is hidden: GCC otherwise unifies inline and template statics across
// all loaded libraries (STB_GNU_UNIQUE), so two NIF libraries would share registrations
// and resource types.
[[gnu::visibility("hidden")]] inline std::vector<resource_registration>& resource_registrations()
{
    static std::vector<resource_registration> registrations;
    return registrations;
}


// Opens every registered resource type; MODULE's load callback calls this.
inline void open_resource_types(ErlNifEnv* env)
{
    for (const auto& [type, name, destructor] : resource_registrations())
    {
        *type = enif_open_resource_type(env, nullptr, name, destructor, ERL_NIF_RT_CREATE, nullptr);
        if (!*type)
            throw std::runtime_error(std::string("failed to open NIF resource type: ") + name);
    }
}
}  // namespace detail


// A reference-counted handle to a NIF resource, like std::shared_ptr but sharing the
// VM's count: each handle holds one reference, so handles can be copied, stored in
// other objects, and kept across calls. T lives in the resource memory, so returning
// the same object again yields the same Erlang reference. T may be incomplete where a
// handle is only declared, copied, or released, e.g. inside T's own definition.
template <typename T>
class resource
{
    // The resource memory holds an optional, so if T's constructor throws in
    // alloc, the destructor callback finds it empty and skips ~T().
    using slot = std::optional<T>;

    // The resource memory, which holds a slot. Kept as void* so passing it to enif_*
    // functions doesn't trigger argument-dependent lookup on std::optional<T>, which
    // would require T to be complete.
    void* objp;

    friend struct type_cast<resource<T>>;

    // Adopts a reference the caller already holds.
    explicit resource(void* objp) :
        objp(objp)
    {}

public:
    typedef T type;

    resource(const resource<T>& other) noexcept :
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

    T& get() const noexcept
    {
        return **static_cast<slot*>(objp);
    }

    T& operator*() const noexcept
    {
        return get();
    }

    T* operator->() const noexcept
    {
        return std::addressof(get());
    }

    template <typename... Args>
        requires std::constructible_from<T, Args...>
    static resource<T> alloc(Args&&... args)
    {
        // Naming registered instantiates it, which registers T for MODULE's load callback.
        (void)resource<T>::registered;
        if (!resource<T>::resource_type)
            throw std::runtime_error("NIF resource type not opened; define the NIF module with MODULE");

        void* buf = enif_alloc_resource(resource<T>::resource_type, sizeof(slot));
        if (!buf)
            throw std::bad_alloc{};

        // Owning the resource first releases it if the constructor throws.
        resource<T> res{new (buf) slot};
        static_cast<slot*>(res.objp)->emplace(std::forward<Args>(args)...);
        return res;
    }

    static void destructor(ErlNifEnv*, void* objp)
    {
        static_cast<slot*>(objp)->~slot();
    }

    // Hidden, like resource_registrations.
    [[gnu::visibility("hidden")]] static inline ErlNifResourceType* resource_type = nullptr;

private:
    // Initialized when the library is loaded, before its load callback runs. The mangled
    // name is unique per type.
    [[gnu::visibility("hidden")]] static inline const bool registered =
        (detail::resource_registrations().push_back({&resource_type, typeid(T).name(), destructor}), true);
};
}  // namespace expp
