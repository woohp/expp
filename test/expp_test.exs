defmodule FooTest do
  use ExUnit.Case
  doctest Foo

  test "list times number" do
    assert Foo.times([1, 2, 3], 2) == [2, 4, 6]
  end

  test "map keys times values" do
    assert Foo.times2(%{2 => 3, 4 => 5, 6 => 8}) == %{2 => 6, 4 => 20, 6 => 48}
  end

  test "multiplies tuple values by 4" do
    assert Foo.times4({2, 3, 5}) == {8, 12, 20}
  end

  test "multiplies by 5 and returns with :ok" do
    assert Foo.times5(7) == {:ok, 35}
  end

  test "handles variant" do
    assert Foo.handle_variant(5) == 25
    assert Foo.handle_variant("5") == "55"
  end
end
