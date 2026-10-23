#pragma once
#include "type_cast_fwd.hpp"
#include <algorithm>
#include <erl_nif.h>
#include <string>
#include <string_view>

#if ERL_NIF_MAJOR_VERSION < 2 || (ERL_NIF_MAJOR_VERSION == 2 && ERL_NIF_MINOR_VERSION < 17)
#error "expp requires NIF version 2.17 (OTP 26) or later for UTF-8 atoms"
#endif


namespace expp
{
namespace detail
{
// Allows string literals to be used as non-type template parameters,
// e.g. `static_atom<"ok">(env)` or `gleam::make_case<"some_tag">(...)`.
template <std::size_t N>
struct fixed_string
{
    char value[N];

    consteval fixed_string(const char (&str)[N])
    {
        std::copy_n(str, N, value);
    }

    constexpr std::string_view view() const
    {
        return {value, N - 1};
    }
};


// Atoms live in the VM-wide atom table and are never garbage collected, so
// their terms are valid in every env and can be created once and reused.
template <fixed_string Name>
ERL_NIF_TERM static_atom(ErlNifEnv* env)
{
    static const ERL_NIF_TERM atom = enif_make_atom_len(env, Name.value, Name.view().size());
    return atom;
}


template <fixed_string Name>
bool is_atom(ErlNifEnv* env, ERL_NIF_TERM term)
{
    return enif_is_identical(term, static_atom<Name>(env));
}
}  // namespace detail


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
