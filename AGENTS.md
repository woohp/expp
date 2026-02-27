# expp - Erlang/Elixir NIF Library

A header-only C++17+ library for creating NIFs for Erlang/Elixir.

## Architecture

This project combines two languages:

- **C++**: Core library implementation in `expp.hpp` and NIF functions in `expp.cpp`
- **Elixir**: Elixir wrapper modules in `lib/` and tests in `test/`

The build system compiles C++ into a dynamic library (`expp.so`) loaded by Elixir modules.

## Build & Compile

### Build the C++ NIF library

```bash
make              # Build expp.so
make clean        # Clean build artifacts
make format       # Format both C++ and Elixir code
```

### Compile and start Elixir environment

```bash
mix compile               # Compile Elixir code (requires expp.so)
mix clean                 # Clean Elixir build
mix shell                 # Start interactive Elixir shell
```

## Testing

### Run all tests

```bash
mix test
```

### Run a single test by line number

```bash
mix test test/my_mod_test.exs:6   # Run test at line 6
```

### Run a single test by name

```bash
mix test --filter vector_times_int
mix test --only vector_times_int
```

### Run tests with traces

```bash
mix test --trace
```

### Run specific test module

```bash
mix test test/my_mod_test.exs
```

### Run doctests only

```bash
mix test --only doctest
```

## Code Formatting

### Format C++ code

```bash
clang-format -i src/*.hpp expp.cpp
```

The `.clang-format` file uses WebKit-based style with:

- 120 character line limit
- 4-space indentation
- Braces on new line for functions/classes/namespaces
- No namespace indentation
- Binpacking disabled for arguments/parameters

### Format Elixir code

```bash
mix format
```

The `.formatter.exs` configures:

- 120 character line length
- Applies to `*.ex`, `*.exs`, and `lib/**/*.{ex,exs}`

## Code Style Guidelines

### C++ Style

#### Imports and Includes

- Place standard library includes first (alphabetically sorted)
- Then local headers (relative paths)
- Use `#pragma once` instead of include guards

```cpp
#include <algorithm>
#include <coroutine>
#include <optional>
#include <erl_nif.h>
#include "expp.hpp"
#include "casts.hpp"
```

#### Type Handling

- Prefer modern C++ types: `std::optional`, `std::expected`, `std::variant`
- Use `constexpr` and `consteval` appropriately for compile-time computation
- Use `noexcept` specifiers on NIF wrapper functions
- Prefer move semantics with `std::move` where applicable

#### Naming Conventions

- Functions: `camelCase` (e.g., `vector_times_int`, `simple_coroutine`)
- Types: `camelCase` with first word capitalized for structs/classes
- Namespaces: `snake_case` (e.g., `expp`, `expp::exceptions`)
- Module macros: `PascalCase` (e.g., `Elixir.MyMod`)

#### Error Handling

- Use `throw erl_error<T>(message)` for type-safe Erlang errors
- Use `throw std::invalid_argument` for argument validation failures
- Use `throw std::runtime_error` for runtime errors that should raise exceptions
- The wrapper catches all exceptions and converts them to Erlang errors

```cpp
int throw_error(int i) {
    if (i == 0)
        throw erl_error<string>("some error");
    if (i == 1)
        throw erl_error<int>(42);
    return i;
}
```

#### Function Definition Pattern

- Define standalone functions first with type signatures matching Erlang expectations
- Use the `MODULE` macro to register NIFs with the Erlang VM:

```cpp
int add(int a, int b) {
    return a + b;
}

MODULE(
    Elixir.MyMod,
    def(add, DirtyFlags::NotDirty),
    def(dirty_function, DirtyFlags::DirtyCpu), )
```

- Use `DirtyFlags::DirtyCpu` for CPU-bound operations
- Use `DirtyFlags::DirtyIO` for I/O-bound operations
- Use `DirtyFlags::NotDirty` for quick operations

#### Generator/Coroutine Functions

- Use `yielding<T>` return type for generator functions
- Cannot take reference arguments or binary arguments
- Uses `co_yield` to produce values

```cpp
yielding<pair<int, int>> simple_coroutine(int n) {
    for (int i = 0; i < n; i++) {
        if (i < n - 1)
            co_yield nullopt;
        else
            co_yield make_pair(i, i * i);
    }
}
```

### Elixir Style

#### Module Structure

- Use `Expp` macro with path to compiled library
- Provide fallback implementation for when NIF not loaded
- Document modules thoroughly

```elixir
defmodule MyMod do
  use Expp, ext: "./expp"

  def vector_times_int(_v, _i), do: "NIF library not loaded"
  def times2(_m), do: "NIF library not loaded"
end
```

#### Test Style

- Use `ExUnit.Case` for test modules
- Include `doctest` for doctests
- Test argument validation with `assert_raise`
- Test error returns in the format `{:error, message}`
- Test `{:ok, value}` tuple returns for successful operations

#### Atoms and Patterns

- Use string literals for atoms in C++: `"foo"_atom`
- Elixir atoms use prefix `:`: `:foo`, `:ok`, `:error`

## File Structure

```
expp/
├── expp.hpp          # Combined bundle header (main library)
├── expp.cpp          # NIF function implementations
├── src/              # Source headers for the library
│   ├── expp.hpp      # Core module definitions
│   ├── casts.hpp     # Type casting between Erlang and C++
│   ├── atom.hpp      # Atom type support
│   ├── binary.hpp    # Binary type support
│   ├── resource.hpp  # Resource management
│   ├── yielding.hpp  # Generator/coroutine support
│   └── ...           # Additional type support
├── lib/              # Elixir module wrappers
│   ├── expp.ex       # Expp module (use directive provider)
│   └── my_mod.ex     # Example module
├── test/             # Elixir tests
│   ├── test_helper.exs
│   └── my_mod_test.exs
├── Makefile          # Build configuration
├── mix.exs           # Elixir project configuration
├── .clang-format     # C++ formatter configuration
└── .formatter.exs    # Elixir formatter configuration
```

## Common Patterns

### Creating a new NIF module

1. Create `src/new_module.cpp` with C++ functions
2. Use the `MODULE` macro to register functions
3. Create `lib/new_module.ex` with `use Expp, ext: "./path"`
4. Add NIF loading in module's `init/0` function
5. Add build rules to Makefile
6. Write tests in `test/new_module_test.exs`

### Resource Types

Use the `resource<T>` template for managing C++ objects that outlive NIF calls:

```cpp
struct MyResource {
    int value;
};

resource<MyResource> make_resource(int i) {
    return resource<MyResource>::alloc(i);
}
```

Initialize in the `load` function:

```cpp
int load(ErlNifEnv* env, void**, ERL_NIF_TERM) {
    resource<MyResource>::init(env, "MyResource");
    return 0;
}
```

### Type Conversions

The library handles automatic conversions:

- `vector<T>` ↔ Elixir lists
- `map<K, V>` / `unordered_map<K, V>` ↔ Elixir maps
- `string` / `binary` ↔ Elixir binaries
- `std::tuple<T...>` ↔ Elixir tuples
- `std::optional<T>` ↔ `nil` or value
- `std::expected<T, E>` ↔ `{:ok, value}` or `{:error, error}`
- `std::variant<T...>` ↔ values of variant types
- Custom structs via `type_cast` implementation

