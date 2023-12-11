defmodule MyMod do
  @moduledoc """
  Documentation for `MyMod`.
  """

  @on_load :init

  app = Mix.Project.config()[:app]

  def init do
    base_path =
      case :code.priv_dir(unquote(app)) do
        {:error, :bad_name} -> ~c"priv"
        dir -> dir
      end

    path = :filename.join(base_path, ~c"my_mod")
    :ok = :erlang.load_nif(path, 0)
  end

  def add(_a, _b) do
    "NIF library not loaded"
  end
end
