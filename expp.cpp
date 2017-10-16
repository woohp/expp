#include <erl_nif.h>
#include "src/expp.hpp"
#include "src/stl.hpp"
using namespace std;


vector<int> times(vector<int> v, int i)
{
    for (auto& x: v)
        x *= i;
    return v;
}


unordered_map<int, int> times2(unordered_map<int, int> m)
{
    for (auto& item: m)
        item.second *= item.first;
    return m;
}


vector<int> times3(vector<int> m)
{
    for (auto& item: m)
        item *= 3;
    return m;
}


tuple<int, int, int> times4(tuple<int, int, int> m)
{
    return make_tuple(get<0>(m) * 4, get<1>(m) * 4, get<2>(m) * 4);
}


atom catcat(atom a)
{
    return atom(a.name + a.name);
}


ELIXIR_MODULE(Foo,
    def(times, "times"),
    def(times2, "times2"),
    def(times3, "times3"),
    def(times4, "times4"),
    def(catcat, "catcat")
)
