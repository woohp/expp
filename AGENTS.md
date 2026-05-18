# expp - BEAM NIF Library

Header-only C++23 library for creating Erlang/Elixir/Gleam NIFs.

## Architecture

- **Core C++ headers** live in `src/*.hpp`.
- **Bundled public header** is root `expp.hpp`, generated from `src/*.hpp` by `./scripts/bundle.sh`.
- **Example/test NIF** lives in `expp.cpp` and builds to `expp.so`.
- **Elixir wrapper/tests** live in `lib/` and `test/`.
- **Gleam interop** uses an Erlang loader module plus Gleam `@external` declarations.

Do not edit root `expp.hpp` directly. Edit `src/*.hpp`, then regenerate the bundle.

## Common Commands

```bash
make              # Build expp.so
make clean        # Remove expp.so
make bundle       # Generates bundled root expp.hpp from individual src files
make format       # clang-format src/*.hpp/expp.cpp, mix format
mix compile       # Compile Elixir project
mix test          # Run all tests
mix test test/my_mod_test.exs:6
mix test --filter vector_times_int
```

When done with code changes, run the following, and fix any issues that might come up.

```bash
make format
make bundle
make clean && make
mix test
```

## C++ Guidelines

- Use C++23.
- Prefer modern standard types: `std::optional`, `std::expected`, `std::variant`, `std::tuple`, `std::vector`.
- Preserve existing style; NIF/example function names are mostly `snake_case`.
- Keep standard/library includes first, then local headers.
- Use `#pragma once` in headers.
- Use `DirtyFlags::DirtyCpu` for CPU-bound work, `DirtyFlags::DirtyIO` for blocking I/O, and `DirtyFlags::NotDirty` for fast calls.
- Throw `std::invalid_argument` for bad arguments and `std::runtime_error` for runtime failures; the wrapper converts them to BEAM exceptions.
- Throw `erl_error<T>(value)` to return `{:error, value}` / `{error, Value}` instead of raising.

Basic NIF shape:

```cpp
#include "expp.hpp"

using namespace expp;

int add(int a, int b)
{
    return a + b;
}

MODULE(
    Elixir.MyMod,
    nullptr,
    nullptr,
    nullptr,
    def(add, DirtyFlags::NotDirty))
```

For Erlang/Gleam loaders, use a normal Erlang module atom instead of an Elixir module name:

```cpp
MODULE(my_nif_ffi, nullptr, nullptr, nullptr, def(add, DirtyFlags::NotDirty))
```

## Type Conversion Notes

Conversions are BEAM-term based and work across Erlang, Elixir, and Gleam surface syntax.

- integers ↔ `int8_t` through `uint64_t` with bounds checks
- floats ↔ `float` / `double`
- `true` / `false` atoms ↔ `bool`
- binaries ↔ `std::string`, `std::string_view`, `expp::binary`
- lists ↔ `std::vector<T>` except byte vectors, which encode as binaries
- maps ↔ `std::map<K,V>` / `std::unordered_map<K,V>`
- tuples ↔ `std::pair<X,Y>` / `std::tuple<T...>`
- `nil | value` ↔ `std::optional<T>`
- `{ok, value} | {error, reason}` ↔ `std::expected<T,E>`
- resources ↔ `expp::resource<T>`
- raw terms ↔ `expp::term`

## Gleam Interop

- `src/gleam.hpp` contains Gleam-specific term shapes and is included by `src/expp.hpp` / bundled `expp.hpp`.
- Use `expp::gleam::option<T>` for Gleam `Option(T)`: `none | {some, value}`.
- Keep `std::optional<T>` as the existing `nil | value` shape.
- Use `std::expected<T,E>` for Gleam `Result(T,E)`: `{ok, value} | {error, reason}`.
- Use `expp::gleam::make_case<"tag">(...)` to construct Gleam custom-type constructor tuples like `{tag, ...}`.
- Use `expp::gleam::case_<"tag", ...>` when declaring a `std::variant` that can return multiple Gleam constructors.
- Do not add permissive bool/int coercions. Gleam `Bool` maps to C++ `bool`, not `int`.
- Gleam examples should use an Erlang loader module with `code:priv_dir/1` and `@external(erlang, "loader_module", "nif_name")`.

## Elixir Interop

- Use `use Expp, ext: "./path/to/nif"` in Elixir modules.
- Provide fallback functions that call `:erlang.nif_error(:not_loaded)`.
- Tests use `ExUnit.Case`, `doctest`, `assert_raise` for invalid arguments, and `{:ok, value}` / `{:error, reason}` assertions for expected-like returns.

## Resources

Use `resource<T>` for C++ objects that outlive one NIF call:

```cpp
struct MyResource
{
    int value;
};

resource<MyResource> make_resource(int i)
{
    return resource<MyResource>::alloc(i);
}

int load(ErlNifEnv* env, void**, ERL_NIF_TERM)
{
    resource<MyResource>::init(env, "MyResource");
    return 0;
}
```

## Coroutines

- Use `yielding<T>` return type and `co_yield`.
- Generator arguments must be fully owned; no references, borrowed binaries, `std::string_view`, raw terms, or non-persistent resources unless explicitly marked yield-persistent.
