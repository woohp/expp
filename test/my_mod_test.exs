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
      MyMod.bool_arguments("true")
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

  test "expected returns" do
    assert MyMod.get_expected(3) == {:ok, 123}
    assert MyMod.get_expected(-5) == {:error, -123}
  end

  test "expected returns binary error message" do
    assert MyMod.get_expected_stringview_error(-5) == {:error, "my bad..."}
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

  test "nested vector" do
    assert MyMod.nested_vector([[1, 2], [3, 4]]) == [[2, 4], [6, 8]]
  end

  test "ordered map" do
    m = %{"a" => 1, "b" => 2, "c" => 3}
    assert MyMod.ordered_map_test(m) == m
  end

  test "complex nested map" do
    m = %{"a" => [1, 2], "b" => [3, 4]}
    assert MyMod.complex_nested_map(m) == %{"a" => [2, 4], "b" => [6, 8]}
  end

  test "byte vector" do
    # "ABC" is [65, 66, 67], plus one is [66, 67, 68] which is "BCD"
    assert MyMod.byte_vector_test("ABC") == "BCD"
  end

  test "resources" do
    res = MyMod.make_resource(123)
    assert is_reference(res)
    assert MyMod.use_resource(res) == 123
  end

  test "handle erl_error" do
    assert MyMod.throw_error(10) == 10
    assert MyMod.throw_error(0) == {:error, "some error"}
    assert MyMod.throw_error(1) == {:error, 42}
  end

  test "dirty cpu nif" do
    assert MyMod.dirty_cpu_test(5) == 50
  end

  test "Elixir exception structs" do
    # Test ArgumentError from invalid decoding
    try do
      MyMod.vector_times_int("not a list", 2)
    rescue
      e in ArgumentError ->
        assert e.message == "invalid vector"
    end

    # Test RuntimeError from C++ exception
    try do
      MyMod.raise_runtime_error_test()
    rescue
      e in RuntimeError ->
        assert e.message == "this is a runtime error from C++"
    end
  end

  test "binary identity" do
    assert MyMod.binary_identity("hello") == "hello"
  end

  test "term identity" do
    assert MyMod.term_identity(123) == 123
    assert MyMod.term_identity("hello") == "hello"
    assert MyMod.term_identity(:foo) == :foo
  end

  test "empty list and map" do
    assert MyMod.vector_times_int([], 2) == []
    assert MyMod.times2(%{}) == %{}
  end

  test "large map performance and correctness" do
    size = 1000
    map = for i <- 1..size, into: %{}, do: {i, i}
    expected = for i <- 1..size, into: %{}, do: {i, i * i}
    assert MyMod.times2(map) == expected
  end

  test "tuple arity mismatch" do
    assert_raise ArgumentError, fn ->
      MyMod.times4({1, 2})
    end

    assert_raise ArgumentError, fn ->
      MyMod.times4({1, 2, 3, 4})
    end
  end

  test "multimap roundtrip" do
    input = [{"a", 1}, {"a", 2}, {"b", 3}]
    assert MyMod.multimap_test(input) == input
  end

  test "unordered_multimap roundtrip" do
    list = [{"a", 1}, {"a", 2}, {"b", 3}]
    result = MyMod.unordered_multimap_test(list)
    assert length(result) == 3
    for {k, v} <- result, do: assert(Enum.member?(list, {k, v}))
  end

  test "empty multimap" do
    assert MyMod.multimap_test([]) == []
  end

  test "empty unordered_multimap" do
    assert MyMod.unordered_multimap_test([]) == []
  end

  test "invalid multimap raises" do
    assert_raise ArgumentError, fn ->
      MyMod.multimap_test(%{})
    end
  end

  test "int8_t bounds" do
    assert MyMod.int8_identity(0) == 0
    assert MyMod.int8_identity(127) == 127
    assert MyMod.int8_identity(-128) == -128

    assert_raise ArgumentError, fn -> MyMod.int8_identity(128) end
    assert_raise ArgumentError, fn -> MyMod.int8_identity(-129) end
  end

  test "uint8_t bounds" do
    assert MyMod.uint8_identity(0) == 0
    assert MyMod.uint8_identity(255) == 255

    assert_raise ArgumentError, fn -> MyMod.uint8_identity(256) end
    assert_raise ArgumentError, fn -> MyMod.uint8_identity(-1) end
  end

  test "int16_t bounds" do
    assert MyMod.int16_identity(0) == 0
    assert MyMod.int16_identity(32767) == 32767
    assert MyMod.int16_identity(-32768) == -32768

    assert_raise ArgumentError, fn -> MyMod.int16_identity(32768) end
    assert_raise ArgumentError, fn -> MyMod.int16_identity(-32769) end
  end

  test "float bounds" do
    assert MyMod.float_identity(0.0) == 0.0
    assert_in_delta MyMod.float_identity(3.14), 3.14, 0.001

    assert_raise ArgumentError, fn -> MyMod.float_identity(1.0e39) end
    assert_raise ArgumentError, fn -> MyMod.float_identity(-1.0e39) end
  end
end
