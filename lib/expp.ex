defmodule Expp do
  defmacro __using__(opts) do
    quote do
      @on_load :load_nif
      def load_nif() do
        ext = Keyword.fetch!(unquote(opts), :ext)
        ext = to_charlist(ext)
        :ok = :erlang.load_nif(ext, 0)
      end
    end
  end
end
