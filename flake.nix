{
  inputs = {
    nixpkgs.url = github:NixOS/nixpkgs/nixos-23.11;

    utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, utils, nixpkgs, ... }:
    utils.lib.eachDefaultSystem
      (system:
        let
          pkgs = nixpkgs.legacyPackages.${system};
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
