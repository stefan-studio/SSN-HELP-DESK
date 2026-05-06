{
  description = "Python project with Flask";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs { inherit system; };

        pythonEnv = pkgs.python312.withPackages (ps: with ps; [
          pip
          setuptools
          wheel
        ]);

      in {
        devShells.default = pkgs.mkShell {
          name = "helpdesk-env";

          buildInputs = [
          	pkgs.gcc
            pythonEnv
            pkgs.direnv
            pkgs.stdenv.cc.cc.lib
          ];

          shellHook = ''
            if [ ! -d .venv ]; then
              echo "Creating virtual environment..."
              python -m venv .venv --system-site-packages
            fi

            source .venv/bin/activate

            pip install --quiet \
				flask
            echo ""
            echo "✓ flask:       $(flask --version)"
            echo ""
          '';
        };
      }
    );
}
