defmodule Expp do
  @include_dir Path.expand(".")

  @doc """
  Returns the directory containing the expp C++ headers (`expp.hpp`).
  """
  @spec include_dir() :: String.t()
  def include_dir(), do: @include_dir

  defmacro __using__(opts) do
    quote do
      @on_load :load_nif
      def load_nif() do
        path = Keyword.fetch!(unquote(opts), :path)
        path = to_charlist(path)
        :ok = :erlang.load_nif(path, 0)
      end
    end
  end
end
