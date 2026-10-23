#pragma once
#include "type_cast_fwd.hpp"
#include <erl_nif.h>
#include <string>

#if ERL_NIF_MAJOR_VERSION < 2 || (ERL_NIF_MAJOR_VERSION == 2 && ERL_NIF_MINOR_VERSION < 17)
#error "expp requires NIF version 2.17 (OTP 26) or later for UTF-8 atoms"
#endif


namespace expp
{
struct atom
{
private:
    atom(const char* name, std::size_t len) :
        name(name, len)
    {}

    explicit atom(std::string name) :
        name(std::move(name))
    {}

    friend atom operator""_atom(const char* s, std::size_t len);

    friend struct type_cast<atom>;

public:
    bool operator==(const std::string_view sv) const
    {
        return this->name == sv;
    }

    bool operator==(const atom& other) const
    {
        return this->name == other.name;
    }

    std::string name;
};


inline atom operator""_atom(const char* s, std::size_t len)
{
    return atom{s, len};
}
}  // namespace expp
