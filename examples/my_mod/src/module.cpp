#include "expp.hpp"
#include <erl_nif.h>

int add(int a, int b)
{
    return a + b;
}

MODULE(
    Elixir.MyMod,
    def(add, "add")
)
