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

  def times2(_s) do
    "NIF library not loaded"
  end

  def times3(_s) do
    "NIF library not loaded"
  end

  def times4(_s) do
    "NIF library not loaded"
  end

  def catcat(_s) do
    "NIF library not loaded"
  end

  def boo(_s) do
    "NIF library not loaded"
  end

  def bar(_s) do
    "NIF library not loaded"
  end
end
