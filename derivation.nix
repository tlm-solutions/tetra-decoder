{ clangStdenv
, pkg-config
, cmake
, cxxopts
, zlib
, fmt
, nlohmann_json
}:
clangStdenv.mkDerivation {
  name = "tetra-decoder";
  version = "0.1.0";

  src = ./.;

  nativeBuildInputs = [ cmake pkg-config fmt ];
  buildInputs = [ cxxopts zlib nlohmann_json ];

  enableParallelBuilding = true;
}
