defmodule MyModTest do
  use ExUnit.Case

  doctest MyMod

  test "list (vector) times int" do
    assert MyMod.vector_times_int([1, 2, 3], 2) == [2, 4, 6]
  end

  test "map keys times values" do
    assert MyMod.times2(%{2 => 3, 4 => 5, 6 => 8}) == %{2 => 6, 4 => 20, 6 => 48}
  end

  test "multiplies tuple values by 4" do
    assert MyMod.times4({2, 3, 5}) == {8, 12, 20}
  end

  test "multiplies by 5 and returns with :ok" do
    assert MyMod.times5(7) == {:ok, 35}
  end

  test "handles variant" do
    assert MyMod.variant_int_and_string(5) == 25
    assert MyMod.variant_int_and_string("5") == "55"

    assert_raise ArgumentError, fn ->
      MyMod.variant_int_and_string(5.0)
    end
  end

  test "string_view to convert to/from binaries" do
    assert MyMod.stringview_identity("123xyz") == "123xyz"
  end

  test "vector<char> should convert to/from binaries" do
    assert MyMod.vector_char_plus_one("23467") == "34578"
  end

  test "vector<int8> should convert to/from binaries" do
    assert MyMod.vector_int8_plus_one("23467") == "34578"
  end

  test "bool arguments" do
    assert MyMod.bool_arguments(true) == 3
    assert MyMod.bool_arguments(false) == 5

    assert_raise ArgumentError, fn ->
      MyMod.bool_arguments(1)
    end

    assert_raise ArgumentError, fn ->
      MyMod.bool_arguments(:tru)
    end

    assert_raise ArgumentError, fn ->
      MyMod.bool_arguments(:truetrue)
    end
  end

  test "bool returns" do
    assert MyMod.bool_returns(-5) == true
    assert MyMod.bool_returns(5) == false
  end

  test "optional arguments" do
    assert MyMod.optional_arguments(5) == 5
    assert MyMod.optional_arguments(nil) == -123

    assert_raise ArgumentError, fn ->
      MyMod.optional_arguments(:nill)
    end

    assert_raise ArgumentError, fn ->
      MyMod.optional_arguments(false)
    end
  end

  test "optional returns" do
    assert MyMod.optional_returns(5) == 5
    assert is_nil(MyMod.optional_returns(-123))
  end

  test "erl_result returns" do
    assert MyMod.get_erl_result(3) == {:ok, 123}
    assert MyMod.get_erl_result(-5) == {:error, -123}
  end

  test "erl_result returns binary error message" do
    assert MyMod.get_erl_result_binary_error(-5) == {:error, "my bad..."}
  end

  test "test atom arguments" do
    assert MyMod.atom_arguments(:foo) == 1
    assert MyMod.atom_arguments(:bar) == -1
  end

  test "test atom returns" do
    assert MyMod.atom_returns(123) == :foo
    assert MyMod.atom_returns(-123) == :bar
  end

  test "coroutines" do
    assert MyMod.simple_coroutine(5) == {4, 16}
  end
end
