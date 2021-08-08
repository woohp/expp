defmodule Foo do
  @moduledoc """
  Documentation for Foo.
  """

  @on_load :init
  def init() do
    :ok = :erlang.load_nif('expp', 0)
  end

  def vector_times_int(_v, _i), do: "NIF library not loaded"
  def vector_char_plus_one(_v), do: "NIF library not loaded"
  def vector_int8_plus_one(_v), do: "NIF library not loaded"
  def times2(_m), do: "NIF library not loaded"
  def times4(_t), do: "NIF library not loaded"
  def times5(_i), do: "NIF library not loaded"
  def variant_int_and_string(_v), do: "NIF library not loaded"
  def bool_arguments(_b), do: "NIF library not loaded"
  def bool_returns(_i), do: "NIF library not loaded"
  def optional_arguments(_i), do: "NIF library not loaded"
  def optional_returns(_i), do: "NIF library not loaded"
  def get_erl_result(_i), do: "NIF library not loaded"
  def get_erl_result_binary_error(_i), do: "NIF library not loaded"
  def atom_arguments(_a), do: "NIF library not loaded"
  def atom_returns(_i), do: "NIF library not loaded"
end
