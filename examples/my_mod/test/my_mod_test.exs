defmodule MyModTest do
  use ExUnit.Case
  doctest MyMod

  test "greets the world" do
    assert MyMod.add(1, 2) == 3
  end
end
