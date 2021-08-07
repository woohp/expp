defmodule Foo do
  @moduledoc """
  Documentation for Foo.
  """

  @on_load :init
  def init() do
    :ok = :erlang.load_nif('expp', 0)
  end

  def times(_v, _i), do: "NIF library not loaded"
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
end
