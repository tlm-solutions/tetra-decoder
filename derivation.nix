{ stdenv
, pkg-config
, cmake
, cxxopts
, zlib
, rapidjson
}:
stdenv.mkDerivation {
  name = "tetra-impl";
  version = "0.1.0";

  src = ./.;

  nativeBuildInputs = [ cmake pkg-config ];
  buildInputs = [ cxxopts zlib rapidjson ];

  enableParallelBuilding = true;
}
