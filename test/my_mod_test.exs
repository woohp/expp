defmodule MyModTest do
  use ExUnit.Case

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

  test "vector<bool> is a list of booleans, not a binary" do
    assert MyMod.bool_vector_negate([true, false, false]) == [false, true, true]
    assert MyMod.bool_vector_negate([]) == []
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

  test "optional of an atom-decoded type accepts its atoms" do
    assert MyMod.optional_bool_arguments(true) == 1
    assert MyMod.optional_bool_arguments(false) == 0
    assert MyMod.optional_bool_arguments(nil) == -1
  end

  test "optional returns" do
    assert MyMod.optional_returns(5) == 5
    assert is_nil(MyMod.optional_returns(-123))
  end

  test "gleam option arguments" do
    assert MyMod.gleam_option_arguments({:some, 5}) == 5
    assert MyMod.gleam_option_arguments(:none) == -123

    assert_raise ArgumentError, fn ->
      MyMod.gleam_option_arguments(nil)
    end

    assert_raise ArgumentError, fn ->
      MyMod.gleam_option_arguments(:some)
    end

    assert_raise ArgumentError, fn ->
      MyMod.gleam_option_arguments({:some})
    end

    assert_raise ArgumentError, fn ->
      MyMod.gleam_option_arguments({:other, 5})
    end
  end

  test "gleam option returns" do
    assert MyMod.gleam_option_returns(5) == {:some, 5}
    assert MyMod.gleam_option_returns(-123) == :none
  end

  test "gleam tagged returns" do
    assert MyMod.gleam_tagged_image(640, 480) == {:image, 640, 480}
  end

  test "gleam tagged variant returns multiple tagged shapes" do
    assert MyMod.gleam_tagged_variant(7) == {:found, 7, "ok"}
    assert MyMod.gleam_tagged_variant(-1) == {:missing, -1}
    assert MyMod.gleam_tagged_variant(0) == {:empty}
  end

  test "expected arguments" do
    assert MyMod.expected_arguments({:ok, 5}) == {:ok, 10}
    assert MyMod.expected_arguments({:error, 5}) == {:error, 15}

    assert_raise ArgumentError, fn ->
      MyMod.expected_arguments(:ok)
    end

    assert_raise ArgumentError, fn ->
      MyMod.expected_arguments({:ok})
    end

    assert_raise ArgumentError, fn ->
      MyMod.expected_arguments({:other, 5})
    end
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

  test "UTF-8 atoms round-trip" do
    assert MyMod.atom_identity(:héllo) == :héllo
    assert MyMod.atom_identity(:日本語) == :日本語
  end

  test "invalid atom inside a returned tuple raises RuntimeError" do
    assert_raise RuntimeError, ~r/invalid atom name/, fn ->
      MyMod.tuple_with_invalid_atom()
    end
  end

  test "containers with custom allocators and comparators" do
    assert MyMod.pmr_strings_identity(["a", "bc"]) == ["a", "bc"]
    assert MyMod.descending_keys(%{1 => 0, 3 => 0, 2 => 0}) == [3, 2, 1]
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

  test "handle erl_error" do
    assert MyMod.throw_error(10) == 10
    assert MyMod.throw_error(0) == {:error, "some error"}
    assert MyMod.throw_error(1) == {:error, 42}

    assert_raise RuntimeError, ~r/invalid atom name/, fn ->
      MyMod.throw_error(2)
    end
  end

  test "Elixir exception structs" do
    assert_raise ArgumentError, ~s(expected a list, got: <<"not a list">>), fn ->
      MyMod.vector_times_int("not a list", 2)
    end

    assert_raise RuntimeError, "this is a runtime error from C++", fn ->
      MyMod.raise_runtime_error_test()
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

  describe "pids and messages" do
    test "pids round-trip" do
      assert MyMod.pid_identity(self()) == self()
    end

    test "non-pids and remote pids raise ArgumentError" do
      assert_raise ArgumentError, ~r/expected a pid/, fn -> MyMod.pid_identity(:foo) end

      node = "other@host"
      remote = :erlang.binary_to_term(<<131, 88, 119, byte_size(node), node::binary, 1::32, 0::32, 1::32>>)
      assert_raise ArgumentError, ~r/expected a local pid/, fn -> MyMod.pid_identity(remote) end
    end

    test "an env parameter doesn't count toward the arity" do
      assert MyMod.process_alive(self())
    end

    test "sent messages outlive the sender's heap, from NIF and non-VM threads" do
      parent = self()
      # Terms from the sending process's heap: a list, a tuple, a heap binary, and a refc binary
      msg = [[1, 2], {:x, [3]}, "small", String.duplicate("large", 100)]

      {pid, ref} = spawn_monitor(fn -> send(parent, {:sent, MyMod.send_twice(parent, msg)}) end)
      assert_receive {:DOWN, ^ref, :process, ^pid, :normal}
      :erlang.garbage_collect()

      assert_received {:sent, {true, true}}
      assert_received {:from_nif, ^msg}
      assert_received {:from_thread, ^msg}
    end

    test "send returns false for dead processes" do
      {pid, ref} = spawn_monitor(fn -> :ok end)
      assert_receive {:DOWN, ^ref, :process, ^pid, _}

      refute MyMod.process_alive(pid)
      assert MyMod.send_twice(pid, "hi") == {false, false}
    end
  end

  test "empty list and map" do
    assert MyMod.vector_times_int([], 2) == []
    assert MyMod.times2(%{}) == %{}
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

  describe "integer bounds" do
    test "int8_t valid values" do
      assert MyMod.int8_identity(0) == 0
      assert MyMod.int8_identity(127) == 127
      assert MyMod.int8_identity(-128) == -128
    end

    test "int8_t rejects values that overflow when cast to int8_t" do
      assert_raise ArgumentError, fn -> MyMod.int8_identity(128) end
      assert_raise ArgumentError, fn -> MyMod.int8_identity(-129) end
      assert_raise ArgumentError, fn -> MyMod.int8_identity(1000) end
    end

    test "int8_t rejects extremely large values" do
      assert_raise ArgumentError, fn -> MyMod.int8_identity(1_000_000_000_000) end
    end

    test "uint8_t valid values" do
      assert MyMod.uint8_identity(0) == 0
      assert MyMod.uint8_identity(255) == 255
    end

    test "uint8_t rejects values that overflow when cast to uint8_t" do
      assert_raise ArgumentError, fn -> MyMod.uint8_identity(256) end
      assert_raise ArgumentError, fn -> MyMod.uint8_identity(300) end
    end

    test "uint8_t rejects extremely large values" do
      assert_raise ArgumentError, fn -> MyMod.uint8_identity(1_000_000_000_000) end
    end

    test "uint8_t rejects negative values" do
      assert_raise ArgumentError, fn -> MyMod.uint8_identity(-1) end
    end

    test "char decodes as an integer" do
      assert MyMod.char_identity(65) == 65
    end

    test "error messages distinguish non-integers from out-of-range integers" do
      assert_raise ArgumentError, "expected an integer, got: foo", fn ->
        MyMod.int_identity(:foo)
      end

      assert_raise ArgumentError, "integer out of range [0, 255], got: -1", fn ->
        MyMod.uint8_identity(-1)
      end

      assert_raise ArgumentError, "integer out of range [-128, 127], got: 1000000000000", fn ->
        MyMod.int8_identity(1_000_000_000_000)
      end
    end

    test "int16_t valid values" do
      assert MyMod.int16_identity(0) == 0
      assert MyMod.int16_identity(32767) == 32767
      assert MyMod.int16_identity(-32768) == -32768
    end

    test "int16_t rejects values that overflow" do
      assert_raise ArgumentError, fn -> MyMod.int16_identity(32768) end
      assert_raise ArgumentError, fn -> MyMod.int16_identity(-32769) end
    end
  end

  describe "float bounds" do
    test "float valid values" do
      assert MyMod.float_identity(0.0) == 0.0
      assert_in_delta MyMod.float_identity(3.14), 3.14, 0.001
    end

    test "float rejects values exceeding float max" do
      assert_raise ArgumentError, fn -> MyMod.float_identity(1.0e39) end
      assert_raise ArgumentError, fn -> MyMod.float_identity(-1.0e39) end
    end
  end

  describe "int identity" do
    test "valid values" do
      assert MyMod.int_identity(0) == 0
      assert MyMod.int_identity(1) == 1
      assert MyMod.int_identity(-1) == -1
      assert MyMod.int_identity(2_147_483_647) == 2_147_483_647
      assert MyMod.int_identity(-2_147_483_648) == -2_147_483_648
    end

    test "rejects values exceeding 32-bit range" do
      assert_raise ArgumentError, fn -> MyMod.int_identity(2_147_483_648) end
      assert_raise ArgumentError, fn -> MyMod.int_identity(-2_147_483_649) end
    end
  end

  describe "uint32_t identity" do
    test "valid values" do
      assert MyMod.uint32_identity(0) == 0
      assert MyMod.uint32_identity(4_294_967_295) == 4_294_967_295
    end

    test "rejects overflow" do
      assert_raise ArgumentError, fn -> MyMod.uint32_identity(4_294_967_296) end
    end

    test "rejects negative values" do
      assert_raise ArgumentError, fn -> MyMod.uint32_identity(-1) end
    end
  end

  describe "int64_t identity" do
    test "valid values" do
      assert MyMod.int64_identity(0) == 0
      assert MyMod.int64_identity(9_223_372_036_854_775_807) == 9_223_372_036_854_775_807
      assert MyMod.int64_identity(-9_223_372_036_854_775_808) == -9_223_372_036_854_775_808
    end

    test "rejects overflow" do
      assert_raise ArgumentError, fn ->
        MyMod.int64_identity(9_223_372_036_854_775_808)
      end

      assert_raise ArgumentError, fn ->
        MyMod.int64_identity(-9_223_372_036_854_775_809)
      end
    end
  end

  describe "uint64_t identity" do
    test "valid values" do
      assert MyMod.uint64_identity(0) == 0
      assert MyMod.uint64_identity(18_446_744_073_709_551_615) == 18_446_744_073_709_551_615
    end

    test "rejects negative values" do
      assert_raise ArgumentError, fn -> MyMod.uint64_identity(-1) end
    end
  end

  describe "double identity" do
    test "valid values" do
      assert MyMod.double_identity(0.0) == 0.0
      assert MyMod.double_identity(3.141592653589793) == 3.141592653589793
      assert MyMod.double_identity(-2.71828) == -2.71828
    end

    test "large values" do
      assert MyMod.double_identity(1.0e308) == 1.0e308
      assert MyMod.double_identity(-1.0e308) == -1.0e308
    end

    test "subnormal values roundtrip" do
      assert MyMod.double_identity(5.0e-324) == 5.0e-324
      assert MyMod.double_identity(-5.0e-324) == -5.0e-324
    end
  end

  describe "string identity" do
    test "roundtrip" do
      assert MyMod.string_identity("hello") == "hello"
      assert MyMod.string_identity("") == ""
      assert MyMod.string_identity("a b c") == "a b c"
    end

    test "unicode" do
      assert MyMod.string_identity("héllo wörld ©") == "héllo wörld ©"
    end
  end

  describe "dirty NIFs" do
    test "DirtyCPU" do
      assert MyMod.dirty_cpu_test(5) == 50
    end

    test "DirtyIO" do
      assert MyMod.dirty_io_test(7) == 700
    end
  end

  describe "named NIF" do
    test "def with explicit name" do
      assert MyMod.named_nif(5) == 6
      assert MyMod.named_nif(0) == 1
      assert MyMod.named_nif(-1) == 0
    end
  end

  describe "yielding" do
    test "multi-step yielding returns final value" do
      assert MyMod.yield_values(1) == 0
      assert MyMod.yield_values(3) == 20
      assert MyMod.yield_values(5) == 40
    end

    test "invalid_argument after a suspension raises ArgumentError" do
      assert MyMod.yield_invalid_argument(7) == 7

      assert_raise ArgumentError, "n must be >= 0", fn ->
        MyMod.yield_invalid_argument(-1)
      end
    end

    test "non-standard exception after a suspension raises RuntimeError" do
      assert_raise RuntimeError, "unknown exception", fn ->
        MyMod.yield_unknown_exception()
      end
    end

    test "exhaustion without co_yield raises RuntimeError" do
      assert_raise RuntimeError, ~r/must contain at least one co_yield/, fn ->
        MyMod.yield_exhaustion()
      end
    end

    test "coroutine with simple (pair<int,int>) still works" do
      assert MyMod.simple_coroutine(5) == {4, 16}
    end

    test "persistent custom type in yielding coroutine" do
      result = MyMod.yield_persistent_type(3)
      assert result == {2, [1, 2, 3]}
    end

    test "persistent type with single step" do
      result = MyMod.yield_persistent_type(1)
      assert result == {0, [1, 2, 3]}
    end

    # enif_thread_type: 1 is a normal scheduler, 2 a dirty CPU scheduler
    test "schedule_on moves the following steps to another scheduler" do
      assert MyMod.schedule_on_steps() == [1, 2, 1]
      assert MyMod.schedule_on_steps_dirty() == [2, 2, 1]
    end
  end

  describe "resource" do
    test "make and use" do
      res = MyMod.make_resource(123)
      assert is_reference(res)
      assert MyMod.use_resource(res) == 123
    end

    test "get() on owning handle before release" do
      res = MyMod.make_resource_incremented(5)
      assert is_reference(res)
      assert MyMod.use_resource(res) == 6
    end

    test "multiple allocations" do
      res1 = MyMod.make_resource(10)
      res2 = MyMod.make_resource(20)
      res3 = MyMod.make_resource(30)
      assert MyMod.use_resource(res1) == 10
      assert MyMod.use_resource(res2) == 20
      assert MyMod.use_resource(res3) == 30
    end

    test "decoded resource can be returned without being accessed" do
      res = MyMod.make_resource(7)
      assert MyMod.resource_identity(res) == res
      assert MyMod.use_resource(MyMod.resource_identity(MyMod.make_resource(8))) == 8
    end

    test "resource trees are freed once every native and BEAM owner is gone" do
      parent = self()

      {pid, ref} =
        spawn_monitor(fn ->
          leaf = MyMod.make_node(1, [])
          root = MyMod.make_node(2, [leaf, MyMod.make_node(3, [])])
          sum = MyMod.tree_sum(root)
          :ok = MyMod.set_first_child(root, MyMod.make_node(4, []))
          # Nodes 3 and 4 are now held only by root's children.
          :erlang.garbage_collect()
          held = {sum, MyMod.tree_sum(root), MyMod.tree_sum(leaf), MyMod.node_live(), MyMod.yield_node_value(root)}
          send(parent, {:held, held})
        end)

      assert_receive {:held, {6, 9, 1, 4, 2}}
      assert_receive {:DOWN, ^ref, :process, ^pid, :normal}
      assert wait_until(fn -> MyMod.node_live() == 0 end)
    end

    test "resource binaries view the resource's bytes and keep it alive" do
      parent = self()

      {pid, ref} =
        spawn_monitor(fn ->
          bin = MyMod.buffer_tail("hello world", 6)
          :erlang.garbage_collect()
          send(parent, {:held, :binary.copy(bin), MyMod.buffer_live()})
        end)

      assert_receive {:held, "world", 1}
      assert_receive {:DOWN, ^ref, :process, ^pid, :normal}
      assert wait_until(fn -> MyMod.buffer_live() == 0 end)

      assert MyMod.buffer_tail("abc", 3) == ""
      assert_raise ArgumentError, fn -> MyMod.buffer_tail("abc", 4) end
    end

    test "throwing constructor does not run the destructor" do
      assert MyMod.make_throwing_resource() == 0
    end

    test "the load callback can allocate resources" do
      assert MyMod.loaded_value() == 42
    end

    test "invalid resource reference raises ArgumentError" do
      assert_raise ArgumentError, fn ->
        MyMod.use_resource(:not_a_resource)
      end
    end

    test "non-reference argument raises ArgumentError" do
      assert_raise ArgumentError, fn ->
        MyMod.use_resource(123)
      end
    end
  end

  describe "Expp module" do
    test "include_dir/0 returns the expp package root" do
      dir = Expp.include_dir()
      assert is_binary(dir)
      assert String.ends_with?(dir, "expp")
      assert File.dir?(dir)
      assert File.exists?(Path.join(dir, "expp.hpp"))
    end
  end

  describe "empty value edge cases" do
    test "empty binary identity" do
      assert MyMod.binary_identity("") == ""
    end

    test "empty string_view identity" do
      assert MyMod.stringview_identity("") == ""
    end
  end

  describe "large data" do
    test "10000-element vector" do
      input = Enum.to_list(1..10_000)
      # vector_times_int multiplies each element by 2
      expected = Enum.map(input, &(&1 * 2))
      assert MyMod.vector_times_int(input, 2) == expected
    end

    test "large map" do
      size = 1000
      map = for i <- 1..size, into: %{}, do: {i, i}
      expected = for i <- 1..size, into: %{}, do: {i, i * i}
      assert MyMod.times2(map) == expected
    end
  end

  defp wait_until(fun, attempts \\ 100) do
    cond do
      fun.() -> true
      attempts == 0 -> false
      true ->
        Process.sleep(10)
        wait_until(fun, attempts - 1)
    end
  end
end
