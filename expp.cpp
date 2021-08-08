#include "src/expp.hpp"
#include "src/stl.hpp"
#include <erl_nif.h>
using namespace std;


vector<int> vector_times_int(vector<int> v, int i)
{
    for (auto& x : v)
        x *= i;
    return v;
}


vector<char> vector_char_plus_one(vector<char> v)
{
    for (auto& c : v)
        c++;
    return v;
}


vector<int8_t> vector_int8_plus_one(vector<int8_t> v)
{
    for (auto& c : v)
        c++;
    return v;
}


unordered_map<int, int> times2(unordered_map<int, int> m)
{
    for (auto& item : m)
        item.second *= item.first;
    return m;
}


tuple<int, int, int> times4(tuple<int, int, int> m)
{
    return make_tuple(get<0>(m) * 4, get<1>(m) * 4, get<2>(m) * 4);
}


erl_result<int, string> times5(int i)
{
    return Ok(i * 5);
}

variant<int, string> variant_int_and_string(variant<int, string> v)
{
    return std::visit(
        [](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, int>)
                return variant<int, string>(arg * 5);
            else
                return variant<int, string>(arg + arg);
        },
        v);
}


int bool_arguments(bool b)
{
    return b ? 3 : 5;
}


bool bool_returns(int i)
{
    return i < 0;
}


int optional_arguments(optional<int> i)
{
    if (i)
        return *i;
    else
        return -123;
}


optional<int> optional_returns(int i)
{
    if (i < 0)
        return nullopt;
    return i;
}


erl_result<int, int> get_erl_result(int i)
{
    if (i >= 0)
        return Ok(123);
    else
        return Error(-123);
}


erl_result<int, binary> get_erl_result_binary_error(int i)
{
    if (i >= 0)
        return Ok(123);
    else
        return Error("my bad..."_binary);
}


int atom_arguments(atom a)
{
    return a == "foo" ? 1 : -1;
}


atom atom_returns(int i)
{
    return i >= 0 ? "foo"_atom : "bar"_atom;
}


MODULE(
    Elixir.Foo,
    nullptr,
    nullptr,
    nullptr,
    def(vector_times_int, DirtyFlags::NotDirty),
    def(vector_char_plus_one, DirtyFlags::NotDirty),
    def(vector_int8_plus_one, DirtyFlags::NotDirty),
    def(times2, DirtyFlags::NotDirty),
    def(times4, DirtyFlags::NotDirty),
    def(times5, DirtyFlags::NotDirty),
    def(variant_int_and_string, DirtyFlags::NotDirty),
    def(bool_arguments, DirtyFlags::NotDirty),
    def(bool_returns, DirtyFlags::NotDirty),
    def(optional_arguments, DirtyFlags::NotDirty),
    def(optional_returns, DirtyFlags::NotDirty),
    def(get_erl_result, DirtyFlags::NotDirty),
    def(get_erl_result_binary_error, DirtyFlags::NotDirty),
    def(atom_arguments, DirtyFlags::NotDirty),
    def(atom_returns, DirtyFlags::NotDirty), )
