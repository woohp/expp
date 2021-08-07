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
    assert Foo.variant_int_and_string(5) == 25
    assert Foo.variant_int_and_string("5") == "55"

    assert_raise ArgumentError, fn ->
      Foo.variant_int_and_string(5.0)
    end
  end

  test "vector<char> should convert to/from binaries" do
    assert Foo.vector_char_plus_one("23467") == "34578"
  end

  test "vector<int8> should convert to/from binaries" do
    assert Foo.vector_int8_plus_one("23467") == "34578"
  end

  test "bool arguments" do
    assert Foo.bool_arguments(true) == 3
    assert Foo.bool_arguments(false) == 5

    assert_raise ArgumentError, fn ->
      Foo.bool_arguments(1)
    end

    assert_raise ArgumentError, fn ->
      Foo.bool_arguments(:tru)
    end

    assert_raise ArgumentError, fn ->
      Foo.bool_arguments(:truetrue)
    end
  end

  test "bool returns" do
    assert Foo.bool_returns(-5) == true
    assert Foo.bool_returns(5) == false
  end

  test "optional arguments" do
    assert Foo.optional_arguments(5) == 5
    assert Foo.optional_arguments(nil) == -123

    assert_raise ArgumentError, fn ->
      Foo.optional_arguments(:nill)
    end

    assert_raise ArgumentError, fn ->
      Foo.optional_arguments(false)
    end
  end

  test "optional returns" do
    assert Foo.optional_returns(5) == 5
    assert is_nil(Foo.optional_returns(-123))
  end
end
