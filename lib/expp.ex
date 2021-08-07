defmodule Foo do
  @moduledoc """
  Documentation for Foo.
  """

  @on_load :init
  def init() do
    :ok = :erlang.load_nif('expp', 0)
  end

  def times(_v, _i) do
    "NIF library not loaded"
  end

  def times2(_m) do
    "NIF library not loaded"
  end

  def times4(_t) do
    "NIF library not loaded"
  end

  def times5(_i) do
    "NIF library not loaded"
  end

  def handle_variant(_v) do
    "NIF library not loaded"
  end

  def bool_arguments(_b) do
    "NIF library not loaded"
  end

  def bool_returns(_i) do
    "NIF library not loaded"
  end
end
