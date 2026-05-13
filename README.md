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
  use Expp, ext: "./expp.so"

  def add(_a, _b), do: "NIF library not loaded"
end
```

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
term term_identity(term t) {
    return t;
}
```

The value is left untouched on both sides of the boundary — no conversion is attempted.

---

## Atoms

Use `expp::atom` as a parameter or return type to receive/produce Elixir atoms. The `""_atom` literal syntax is a shorthand for constructing atom values:

```cpp
atom atom_returns(int i) {
    return i >= 0 ? "ok"_atom : "error"_atom;
}

int atom_arguments(atom a) {
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
int load(ErlNifEnv* env, void**, ERL_NIF_TERM) {
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
int divide(int a, int b) {
    if (b == 0) throw erl_error<std::string>("division by zero");
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

resource<Counter> make_counter(int start) {
    return resource<Counter>::alloc(start);
}

int read_counter(resource<Counter> c) {
    return c.get().value;
}
```

---

## Coroutines (Yielding NIFs)

Functions that execute incrementally across multiple scheduler reductions, preventing VM starvation:

```cpp
expp::yielding<int> process_items(int n) {
    expp::yielding_timer timer;
    for (int i = 0; i < n; i++) {
        // do some work...
        if (timer.times_up()) {
            co_yield std::nullopt;    // suspend, reschedule
            timer.reset();
        }
    }
    co_yield 42;                      // final result
}
```

Restrictions: no reference, `binary`, or `string_view` arguments (their backing storage may be invalidated across resumptions).

---

## Elixir Side

```elixir
defmodule MyMod do
  use Expp, ext: "./expp.so"

  # Fallback implementations when NIF is not loaded
  def add(_a, _b), do: "NIF library not loaded"
end
```

The `Expp` module handles `@on_load` and `:erlang.load_nif` automatically.

---

## Build

```sh
make              # Build expp.so
mix test          # Run tests
mix format        # Format Elixir code
```

Custom Makefile (e.g. with a C++ compiler and `-I$(ERLANG_PATH)`):

```make
CFLAGS = -std=c++23 -fPIC -I$(shell erl -eval 'io:format("~s", [lists:concat([code:root_dir(), "/erts-", erlang:system_info(version), "/include"])])' -s init stop -noshell)
LDFLAGS = -shared

my_nif.so: my_nif.cpp expp.hpp
 $(CXX) $(CFLAGS) $(LDFLAGS) -o $@ my_nif.cpp
```

On macOS add `-undefined dynamic_lookup` to `LDFLAGS`.

---

## Requirements

- C++23 compiler (coroutines, concepts, `std::expected`)
- Erlang/OTP headers (`erl_nif.h`)
- Elixir ≥ 1.15 (optional, for the Elixir wrapper)

---

## Example

See [`examples/my_mod/`](examples/my_mod) for a complete end-to-end project.
