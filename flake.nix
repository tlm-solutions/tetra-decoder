{
  inputs = {
    nixpkgs.url = github:NixOS/nixpkgs/nixos-22.11;

    utils.url = "github:numtide/flake-utils";

    loguru = {
      url = github:emilk/loguru;
      flake = false;
    };
  };

  outputs = { self, utils, nixpkgs, loguru, ... }:
    utils.lib.eachDefaultSystem
      (system:
        let
          pkgs = nixpkgs.legacyPackages.${system};
          tetra-impl = pkgs.callPackage ./derivation.nix { 
            loguru-src = loguru;
          };
        in
        rec {
          checks = packages;
          packages = {
            tetra-impl = tetra-impl;
            default = tetra-impl;
          };
        }
      ) // {
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
