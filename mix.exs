defmodule Expp.MixProject do
  use Mix.Project

  def project do
    compilers = if env() == :dev, do: [:elixir_make] ++ Mix.compilers(), else: Mix.compilers()

    [
      app: :expp,
      version: "0.1.0",
      elixir: "~> 1.15",
      compilers: compilers,
      start_permanent: env() == :prod,
      deps: deps()
    ]
  end

  def application do
    [extra_applications: [:logger]]
  end

  defp deps do
    [
      {:elixir_make, "~> 0.6", only: :dev, runtime: false}
    ]
  end

  defp env, do: Mix.env()
end
