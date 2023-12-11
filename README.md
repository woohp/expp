# expp

Header-only library that allows easy creation of NIFs for erlang and elixir using c++17 and above.

Alpha status, can change at any time, please use at your own risk.


## Example

In the file `src/module.cpp`

```cpp
#include "src/expp.hpp"
#include <erl_nif.h>

int add(int a, int b)
{
    return a + b;
}

MODULE(
    Elixir.MyMod,
    def(add, "add")
)
```

In the file `lib/my_mod.ex`

```elixir
defmodule MyMod do
  @moduledoc """
  Documentation for `MyMod`.
  """

  @on_load :init

  app = Mix.Project.config()[:app]

  def init do
    base_path =
      case :code.priv_dir(unquote(app)) do
        {:error, :bad_name} -> ~c"priv"
        dir -> dir
      end

    path = :filename.join(base_path, ~c"my_mod")
    :ok = :erlang.load_nif(path, 0)
  end

  def add(_a, _b) do
    "NIF library not loaded"
  end
end
```

For `mix.exs`

```elixir
defmodule Mix.Tasks.Compile.MyMod do
  def run(_) do
    if !File.exists?("priv") do
      File.mkdir!("priv")
    end

    {result, _error_code} = System.cmd("make", ["priv/my_mod.so"], stderr_to_stdout: true)
    IO.binwrite(result)
    :ok
  end
end

defmodule MyMod.MixProject do
  use Mix.Project

  @version "0.1.0"

  def project do
    [
      app: :my_mod,
      version: @version,
      elixir: "~> 1.15",
      start_permanent: Mix.env() == :prod,
      compilers: [:my_mod, :elixir, :app],
      deps: deps()
    ]
  end

  ...
end
```

And finally, the Makefile

```make
MIX = mix
CFLAGS = -g -O3 -ansi -pedantic -Wall -Wextra -Wno-unused-parameter -std=c++17 -march=native

ERLANG_PATH = $(shell erl -eval 'io:format("~s", [lists:concat([code:root_dir(), "/erts-", erlang:system_info(version), "/include"])])' -s init stop -noshell)
CFLAGS += -I$(ERLANG_PATH)

ifneq ($(OS), Windows_NT)
    CFLAGS += -fPIC

    ifeq ($(shell uname), Darwin)
        LDFLAGS += -dynamiclib -undefined dynamic_lookup
    endif
endif

.PHONY: all my_mod clean

all: my_mod

my_mod:
	$(MIX) compile

priv/my_mod.so: src/module.cpp
	$(CXX) $(CFLAGS) -shared $(LDFLAGS) -o $@ src/module.cpp

clean:
	$(MIX) clean
	$(RM) priv/my_mod.so
```

Please refer to the [full example](examples/my_mod) for the full code.
