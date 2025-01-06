{
  inputs = {
    nixpkgs.url = github:NixOS/nixpkgs/nixos-24.11;

    utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, utils, nixpkgs, ... }:
    utils.lib.eachDefaultSystem
      (system:
        let
          pkgs = import nixpkgs {
            inherit system;
            overlays = [
              (self: super: {
                armadillo = super.armadillo.overrideAttrs(oldAttrs: rec {
                  # when using lapack derived from openblas instead of the reference implementation the build tetra-decoder executable results in having errors when creating pthreads
                  buildInputs = with super; [ blas lapack-reference superlu ];
                });
              })
            ];
          };
          tetra-decoder = pkgs.callPackage ./derivation.nix { };
        in
        rec {
          checks = packages;
          packages = {
            inherit tetra-decoder;
            default = tetra-decoder;
          };
        }
      ) // {
      overlays.default = final: prev: {
        inherit (self.packages.${prev.system})
          tetra-decoder;
      };

      nixosModules = rec {
        default = tetra-decoder;
        tetra-decoder = {
          imports = [ ./nixos-modules ];

          nixpkgs.overlays = [
            self.overlays.default
          ];
        };
      };

      hydraJobs =
        let
          hydraSystems = [ "x86_64-linux" ];
          hydraBlacklist = [ ];
        in
        builtins.foldl'
          (hydraJobs: system:
            builtins.foldl'
              (hydraJobs: pkgName:
                if builtins.elem pkgName hydraBlacklist
                then hydraJobs
                else
                  nixpkgs.lib.recursiveUpdate hydraJobs {
                    ${pkgName}.${system} = self.packages.${system}.${pkgName};
                  }
              )
              hydraJobs
              (builtins.attrNames self.packages.${system})
          )
          { }
          hydraSystems;
    };
}
