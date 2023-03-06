{ clangStdenv
, pkg-config
, cmake
, cxxopts
, zlib
, rapidjson
, fmt
, loguru-src
}:
clangStdenv.mkDerivation {
  name = "tetra-impl";
  version = "0.1.0";

  src = ./.;

  nativeBuildInputs = [ cmake pkg-config fmt ];
  buildInputs = [ cxxopts zlib rapidjson ];

  enableParallelBuilding = true;
}
