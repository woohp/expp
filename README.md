# expp — Erlang/Elixir NIF Library

Header-only C++23 library for writing native Erlang NIFs. Zero dependencies beyond the Erlang VM headers and a C++23 compiler.

```cpp
#include "expp.hpp"

int add(int a, int b) { return a + b; }

MODULE(
    Elixir.MyMod,
    def(add, expp::DirtyFlags::NotDirty)
)
```

```elixir
defmodule MyMod do
  use Expp, ext: "./expp"

  def add(_a, _b), do: :erlang.nif_error(:not_loaded)
end
```

## Quickstart

Create a new project and add the dependencies:

```elixir
# mix.exs
def deps do
  [
    {:expp, github: "woohp/expp", runtime: false},
    {:elixir_make, "~> 0.6", runtime: false}
  ]
end
```

Configure `elixir_make` as the build tool and pass the expp include path:

```elixir
# mix.exs
def project do
  [
    app: :my_nif,
    version: "0.1.0",
    compilers: [:elixir_make] ++ Mix.compilers(),
    make_env: fn -> %{"EXPP_INCLUDE_DIR" => Expp.include_dir()} end,
    make_targets: ["priv/my_nif.so"],
    deps: deps()
  ]
end
```

Write your NIF in `src/my_nif.cpp`:

```cpp
#include "expp.hpp"

int add(int a, int b)
{
    return a + b;
}

MODULE(
    Elixir.MyNif,
    nullptr,
    nullptr,
    nullptr,
    def(add, expp::DirtyFlags::NotDirty))
```

Create a `Makefile` that builds the shared library:

```makefile
ERTS_INCLUDE_DIR ?= $(ERL_EI_INCLUDE_DIR)

priv:
	@mkdir -p priv

priv/my_nif.so: priv src/my_nif.cpp
	$(CXX) -I$(ERTS_INCLUDE_DIR) -I$(EXPP_INCLUDE_DIR) -std=c++23 -fvisibility=hidden -shared -undefined dynamic_lookup -o $@ src/my_nif.cpp

clean:
	$(RM) priv/my_nif.so
```

Write the Elixir module in `lib/my_nif.ex`:

```elixir
defmodule MyNif do
  use Expp, ext: "./priv/my_nif"

  def add(_a, _b), do: :erlang.nif_error(:not_loaded)
end
```

Fetch dependencies, build, and test:

```sh
mix deps.get
mix compile
mix run -e "IO.puts(MyNif.add(2, 3))"
# → 5
```

The shared library is compiled to `priv/my_nif.so` during `mix compile` and loaded at runtime by `Expp`.

> **Symbol visibility.** The `-fvisibility=hidden` flag prevents symbol clashes when multiple NIF libraries use expp in the same VM. Without it, loading two such libraries will fail.

## Type Conversions

Types convert automatically between C++ and Erlang/Elixir:

| C++                              | Elixir                    |
|----------------------------------|---------------------------|
| `int8_t`–`uint64_t`              | integer                   |
| `float` / `double`               | float                     |
| `bool`                           | `true` / `false`          |
| `std::string`                    | binary (copied)           |
| `std::string_view`               | binary (borrowed, no copy)|
| `expp::binary`                   | binary (ref-counted)      |
| `expp::atom`                     | atom                      |
| `std::vector<T>`                 | list (or binary for byte types) |
| `std::map<K,V>` / `std::unordered_map<K,V>` | map          |
| `std::multimap<K,V>` / `std::unordered_multimap<K,V>` | list of `{k,v}` tuples |
| `std::pair<X,Y>`                 | 2-tuple                   |
| `std::tuple<T...>`               | tuple                     |
| `std::optional<T>`               | value or `nil`            |
| `std::expected<T,E>`             | `{:ok, value}` or `{:error, err}` |
| `std::variant<T...>`             | value of whichever variant matches |
| `std::vector<std::byte>` / `std::vector<int8_t>` / `std::vector<char>` | binary |
| `expp::resource<T>`              | reference                 |
| `expp::term`                     | raw `ERL_NIF_TERM` passthrough |

Nested containers (e.g. `vector<vector<int>>`, `map<string, vector<int>>`) work recursively.

### `expp::term` — Raw Passthrough

When you need to handle types the library doesn't convert or want to pass an Erlang term through uninterpreted, use `expp::term`:

```cpp
term term_identity(term t)
{
    return t;
}
```

The value is left untouched on both sides of the boundary — no conversion is attempted.

---

## Atoms

Use `expp::atom` as a parameter or return type to receive/produce Elixir atoms. The `""_atom` literal syntax is a shorthand for constructing atom values:

```cpp
atom atom_returns(int i)
{
    return i >= 0 ? "ok"_atom : "error"_atom;
}

int atom_arguments(atom a)
{
    return a == "foo" ? 1 : -1;
}
```

Comparison works against `std::string_view` and other `atom` values.

---

## NIF Registration

Use `def(fn)` or `def(fn, "elixir_name")` inside the `MODULE(...)` macro. Arity is derived automatically from the C++ function signature via `function_traits` — you never specify it manually.

```cpp
MODULE(
    Elixir.MyMod,
    load,                          // optional load callback
    nullptr,                       // upgrade callback (or nullptr)
    nullptr,                       // unload callback (or nullptr)
    def(add, DirtyFlags::NotDirty),
    def(dirty_cpu_fn, DirtyFlags::DirtyCpu),
    def(dirty_io_fn, DirtyFlags::DirtyIO),
    def(fn, "custom_name", DirtyFlags::NotDirty), )
```

By default the NIF is named after the C++ function. Pass a string as the second argument to override (`def(fn, "elixir_name", ...)`).

The `load` function initialises resource types. If you use coroutines, `yielding_resource_t` must also be initialised here:

```cpp
int load(ErlNifEnv* env, void**, ERL_NIF_TERM)
{
    resource<MyResource>::init(env, "MyResource");
    yielding_resource_t::init(env, "yielding_generator");
    return 0;
}
```

---

## Error Handling

| C++ exception                      | Elixir result                                  |
|------------------------------------|------------------------------------------------|
| `std::invalid_argument`            | `ArgumentError` raised                         |
| `std::runtime_error`               | `RuntimeError` raised                          |
| `erl_error<T>(value)`              | `{:error, value}` tuple returned               |
| `...` (unknown)                    | `RuntimeError` raised                          |

```cpp
int divide(int a, int b)
{
    if (b == 0)
        throw erl_error<std::string>("division by zero");
    return a / b;
}
```

---

## Dirty NIFs

```cpp
def(long_computation, DirtyFlags::DirtyCpu)
def(blocking_io,     DirtyFlags::DirtyIO)
def(fast_op,         DirtyFlags::NotDirty)
```

---

## Resources

Long-lived C++ objects passed as Elixir references:

```cpp
struct Counter { int value; };

resource<Counter> make_counter(int start)
{
    return resource<Counter>::alloc(start);
}

int read_counter(resource<Counter> c)
{
    return c.get().value;
}
```

---

## Coroutines (Yielding NIFs)

Functions that execute incrementally across multiple scheduler reductions, preventing VM starvation:

```cpp
expp::yielding<int> process_items(int n)
{
    expp::yielding_timer timer;
    for (int i = 0; i < n; i++)
    {
        // do some work...
        if (timer.times_up())
        {
            co_yield std::nullopt;    // suspend, reschedule
            timer.reset();
        }
    }
    co_yield 42;                      // final result
}
```

Restrictions: no reference, `binary`, or `string_view` arguments (their backing storage may be invalidated across resumptions).
