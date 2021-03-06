#include "src/expp.hpp"
#include "src/stl.hpp"
#include <erl_nif.h>
using namespace std;


vector<int> times(vector<int> v, int i)
{
    for (auto& x : v)
        x *= i;
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

variant<int, string> handle_variant(variant<int, string> v)
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


MODULE(
    Elixir.Foo,
    def(times, "times", DirtyFlags::NotDirty),
    def(times2, "times2", DirtyFlags::NotDirty),
    def(times4, "times4", DirtyFlags::NotDirty),
    def(times5, "times5", DirtyFlags::NotDirty),
    def(handle_variant, "handle_variant", DirtyFlags::NotDirty), )
