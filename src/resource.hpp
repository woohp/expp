#pragma once
#include <erl_nif.h>
#include <new>
#include <stdexcept>
#include <string>


namespace expp
{
template <typename T, typename... Args>
inline constexpr bool is_brace_constructible_v = requires { T { std::declval<Args>()... }; };


template <typename T>
    requires std::destructible<T>
class resource
{
    ErlNifEnv* env;
    ERL_NIF_TERM term;
    void* objp;
    bool owns_;

    friend struct type_cast<resource<T>>;

    resource(ErlNifEnv* env, ERL_NIF_TERM term)
        : env(env)
        , term(term)
        , objp(nullptr)
        , owns_(false)
    { }

    resource(T* objp)
        : env(nullptr)
        , term(0)
        , objp(objp)
        , owns_(true)
    { }

public:
    typedef T type;

    resource(const resource<T>&) = delete;

    resource(resource<T>&& other)
        : env(other.env)
        , term(other.term)
        , objp(other.objp)
        , owns_(other.owns_)
    {
        other.objp = nullptr;
        other.owns_ = false;
    }

    ~resource()
    {
        if (owns_ && objp)
            enif_release_resource(objp);
    }

    template <typename U = T>
    U& get()
    {
        if (owns_ && objp)
            return *reinterpret_cast<U*>(objp);
        if (!enif_get_resource(env, term, resource<T>::resource_type, &this->objp))
            throw std::invalid_argument("invalid resource");
        return *reinterpret_cast<U*>(this->objp);
    }

    template <typename... Args>
        requires(is_brace_constructible_v<T, Args...>)
    static resource<T> alloc(Args&&... args)
    {
        void* buf = enif_alloc_resource(resource<T>::resource_type, sizeof(T));
        if (!buf)
            throw std::bad_alloc { };

        struct alloc_guard
        {
            void* ptr;
            ~alloc_guard()
            {
                if (ptr)
                    enif_release_resource(ptr);
            }
            void dismiss()
            {
                ptr = nullptr;
            }
        } guard { buf };

        new (buf) T { std::forward<Args>(args)... };
        guard.dismiss();
        return resource<T> { static_cast<T*>(buf) };
    }

    static void init(ErlNifEnv* env, const char* name)
    {
        resource<T>::resource_type
            = enif_open_resource_type(env, nullptr, name, resource<T>::destructor, ERL_NIF_RT_CREATE, nullptr);
        if (!resource<T>::resource_type)
            throw std::runtime_error(std::string("failed to open NIF resource type: ") + name);
    }

    static void destructor(ErlNifEnv*, void* objp)
    {
        reinterpret_cast<T*>(objp)->~T();
    }

    static ErlNifResourceType* resource_type;
};


template <typename T>
    requires std::destructible<T>
ErlNifResourceType* resource<T>::resource_type = nullptr;
}
