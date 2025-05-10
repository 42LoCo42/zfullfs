{
  outputs = { flake-utils, nixpkgs, ... }:
    flake-utils.lib.eachDefaultSystem (system:
      let pkgs = import nixpkgs { inherit system; }; in rec {
        packages.default = pkgs.stdenv.mkDerivation {
          pname = "zfullfs";
          version = "1.1.0";
          src = ./.;

          nativeBuildInputs = with pkgs; [
            pkg-config
          ];

          buildInputs = with pkgs; [
            fuse3
            zfs
          ];

          CFLAGS = "-O3";
        };

        devShells.default = pkgs.mkShell {
          inputsFrom = [ packages.default ];
          packages = with pkgs; [
            bear

            (gf.overrideAttrs (old: {
              patches = old.patches ++ [
                (pkgs.fetchpatch {
                  url = "https://github.com/nakst/gf/pull/188.patch";
                  hash = "sha256-RlT4nOd24pXnTbFqOBLcd5STo1zdPn+mqx3THVS0EQU=";
                })
              ];
            }))
          ];

          CFLAGS = "-ggdb";
        };
      });
}
