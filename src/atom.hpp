#pragma once
#include "type_cast_fwd.hpp"
#include <string>


namespace expp
{
struct atom
{
private:
    atom(const char* name, std::size_t len)
        : name(name, len)
    {}

    explicit atom(std::string name)
        : name(std::move(name))
    {}

    friend atom operator""_atom(const char* s, std::size_t len);

    friend struct type_cast<atom>;

public:
    bool operator==(const std::string_view sv) const
    {
        return this->name == sv;
    }

    bool operator!=(const std::string_view sv) const
    {
        return this->name != sv;
    }

    bool operator==(const atom& other) const
    {
        return this->name == other.name;
    }

    bool operator!=(const atom& other) const
    {
        return this->name != other.name;
    }

    std::string name;
};


inline atom operator""_atom(const char* s, std::size_t len)
{
    return atom{s, len};
}
}  // namespace expp
