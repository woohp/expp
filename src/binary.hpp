#pragma once
#include "type_cast_fwd.hpp"
#include <algorithm>
#include <erl_nif.h>
#include <new>


namespace expp
{
class binary : public ErlNifBinary
{
private:
    ERL_NIF_TERM _term = 0;

    friend struct type_cast<binary>;

    binary& operator=(const binary&) = default;

public:
    binary()
    {
        this->size = 0;
        this->data = nullptr;
    }

    explicit binary(size_t size)
    {
        if (!enif_alloc_binary(size, this))
            throw std::bad_alloc { };
    }

    template <size_t N>
    explicit binary(const char (&str)[N])
    {
        if (!enif_alloc_binary(N - 1, this))
            throw std::bad_alloc { };
        std::copy_n(str, N - 1, this->data);
    }

    binary(binary&& other)
    {
        // No old data to release in a freshly constructed object
        static_cast<ErlNifBinary&>(*this) = static_cast<const ErlNifBinary&>(other);
        _term = other._term;

        other.data = nullptr;
        other.size = 0;
        other._term = 0;
    }

    binary(const binary& other) = delete;

    template <typename T>
        requires((std::is_integral_v<T> && sizeof(T) == 1) || std::is_same_v<T, std::byte>)
    static binary from_bytes(const T* data, size_t size)
    {
        binary b { size };
        std::copy_n(data, size, b.data);
        return b;
    }

    ~binary()
    {
        if (!this->_term && this->data)
        {
            enif_release_binary(this);
            this->size = 0;
            this->data = nullptr;
        }
    }

    binary& operator=(binary&& other)
    {
        if (this != &other)
        {
            // Release our current binary data if we own it
            if (!_term && data)
                enif_release_binary(this);

            // Transfer all fields from other
            static_cast<ErlNifBinary&>(*this) = static_cast<const ErlNifBinary&>(other);
            _term = other._term;

            other.data = nullptr;
            other.size = 0;
            other._term = 0;
        }
        return *this;
    }
};


inline binary operator""_binary(const char* s, std::size_t len)
{
    binary binary_info { len };  // constructor checks allocation and throws on failure
    std::copy_n(s, len, binary_info.data);
    return binary_info;
}
}
