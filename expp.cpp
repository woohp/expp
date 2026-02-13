#include "src/expp.hpp"
#include "src/stl.hpp"
#include <erl_nif.h>

using namespace std;
using namespace expp;


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


std::expected<int, string> times5(int i)
{
    return i * 5;
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


string_view stringview_identity(string_view s)
{
    return s;
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


std::expected<int, int> get_expected(int i)
{
    if (i >= 0)
        return 123;
    else
        return std::unexpected(-123);
}


std::expected<int, std::string_view> get_expected_stringview_error(int i)
{
    if (i >= 0)
        return 123;
    else
        return std::unexpected("my bad...");
}


int atom_arguments(atom a)
{
    return a == "foo" ? 1 : -1;
}


atom atom_returns(int i)
{
    return i >= 0 ? "foo"_atom : "bar"_atom;
}


yielding<pair<int, int>> simple_coroutine(int n)
{
    for (int i = 0; i < n; i++)
    {
        if (i < n - 1)
            co_yield nullopt;
        else
            co_yield make_pair(i, i * i);
    }
}


vector<vector<int>> nested_vector(vector<vector<int>> v)
{
    for (auto& inner : v)
        for (auto& x : inner)
            x *= 2;
    return v;
}


std::map<string, int> ordered_map_test(std::map<string, int> m)
{
    return m;
}


std::map<string, vector<int>> complex_nested_map(std::map<string, vector<int>> m)
{
    for (auto& [k, v] : m)
        for (auto& x : v)
            x *= 2;
    return m;
}


vector<std::byte> byte_vector_test(vector<std::byte> v)
{
    for (auto& b : v)
        b = std::byte { static_cast<unsigned char>(static_cast<unsigned char>(b) + 1) };
    return v;
}


struct MyResource
{
    int value;
};


resource<MyResource> make_resource(int i)
{
    return resource<MyResource>::alloc(i);
}


int use_resource(resource<MyResource> res)
{
    return res.get().value;
}


int throw_error(int i)
{
    if (i == 0)
        throw erl_error<string>("some error");
    if (i == 1)
        throw erl_error<int>(42);
    return i;
}


int dirty_cpu_test(int i)
{
    return i * 10;
}


binary binary_identity(binary b)
{
    return b;
}


term term_identity(term t)
{
    return t;
}


int load(ErlNifEnv* env, void**, ERL_NIF_TERM)
{
    yielding_resource_t::init(env, "yielding_generator");
    resource<MyResource>::init(env, "MyResource");
    return 0;
}


MODULE(
    Elixir.MyMod,
    load,
    nullptr,
    nullptr,
    def(vector_times_int, DirtyFlags::NotDirty),
    def(vector_char_plus_one, DirtyFlags::NotDirty),
    def(vector_int8_plus_one, DirtyFlags::NotDirty),
    def(times2, DirtyFlags::NotDirty),
    def(times4, DirtyFlags::NotDirty),
    def(times5, DirtyFlags::NotDirty),
    def(stringview_identity, DirtyFlags::NotDirty),
    def(variant_int_and_string, DirtyFlags::NotDirty),
    def(bool_arguments, DirtyFlags::NotDirty),
    def(bool_returns, DirtyFlags::NotDirty),
    def(optional_arguments, DirtyFlags::NotDirty),
    def(optional_returns, DirtyFlags::NotDirty),
    def(get_expected, DirtyFlags::NotDirty),
    def(get_expected_stringview_error, DirtyFlags::NotDirty),
    def(atom_arguments, DirtyFlags::NotDirty),
    def(atom_returns, DirtyFlags::NotDirty),
    def(simple_coroutine, DirtyFlags::NotDirty),
    def(nested_vector, DirtyFlags::NotDirty),
    def(ordered_map_test, DirtyFlags::NotDirty),
    def(complex_nested_map, DirtyFlags::NotDirty),
    def(byte_vector_test, DirtyFlags::NotDirty),
    def(make_resource, DirtyFlags::NotDirty),
    def(use_resource, DirtyFlags::NotDirty),
    def(throw_error, DirtyFlags::NotDirty),
    def(dirty_cpu_test, DirtyFlags::DirtyCpu),
    def(binary_identity, DirtyFlags::NotDirty),
    def(term_identity, DirtyFlags::NotDirty), )
