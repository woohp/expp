defmodule Mix.Tasks.Compile.MyMod do
  def run(_) do
    if !File.exists?("priv") do
      File.mkdir!("priv")
    end

    {result, _error_code} = System.cmd("make", ["priv/my_mod.so"], stderr_to_stdout: true)
    IO.binwrite(result)
    :ok
  end
end

defmodule MyMod.MixProject do
  use Mix.Project

  def project do
    [
      app: :my_mod,
      version: "0.1.0",
      elixir: "~> 1.12",
      start_permanent: Mix.env() == :prod,
      compilers: [:my_mod, :elixir, :app],
      deps: deps()
    ]
  end

  # Run "mix help compile.app" to learn about applications.
  def application do
    [
      extra_applications: [:logger]
    ]
  end

  # Run "mix help deps" to learn about dependencies.
  defp deps do
    [
      # {:dep_from_hexpm, "~> 0.3.0"},
      # {:dep_from_git, git: "https://github.com/elixir-lang/my_dep.git", tag: "0.1.0"}
    ]
  end
end
