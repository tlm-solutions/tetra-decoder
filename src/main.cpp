#include <cxxopts.hpp>
#include <filesystem>
#include <memory>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include <Decoder.hpp>

static bool stop = false;

void sigint_handler(int s) {
	(void)s;

	stop = true;
}

int main(int argc, char **argv) {
  unsigned rxPort, txPort, debugLevel;
  bool keepFillBits, packed;
  std::optional<std::string> inFile, outFile;
  std::optional<std::filesystem::path> inFilePath, outFilePath;

  signal(SIGINT, &sigint_handler);

  cxxopts::Options options("tetra-impl", "Decodes TETRA downstream traffic");

  // clang-format off
	options.add_options()
		("h,help", "Print usage")
		("r,rx", "<UDP socket> receiving from phy", cxxopts::value<unsigned>()->default_value("42000"))
		("t,tx", "<UDP socket> sending Json data", cxxopts::value<unsigned>()->default_value("42100"))
		("i,infile", "<file> replay data from binary file instead of UDP", cxxopts::value<std::optional<std::string>>(inFile))
		("o,outfile", "<file> record data to binary file (can be replayed with -i option)", cxxopts::value<std::optional<std::string>>(outFile))
		("d", "<level> print debug information", cxxopts::value<unsigned>()->default_value("0"))
		("f,keep-fill-bits", "keep fill bits", cxxopts::value<bool>()->default_value("false"))
		("P,packed", "pack rx data (1 byte = 8 bits)", cxxopts::value<bool>()->default_value("false"))
		;
  // clang-format on

  try {
    auto result = options.parse(argc, argv);

    if (result.count("help")) {
      std::cout << options.help() << std::endl;
      exit(0);
    }

    rxPort = result["rx"].as<unsigned>();
    txPort = result["tx"].as<unsigned>();

    debugLevel = result["d"].as<unsigned>();
    keepFillBits = result["keep-fill-bits"].as<bool>();
    packed = result["packed"].as<bool>();
  } catch (std::exception &e) {
    std::cout << "error parsing options: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  if (inFile.has_value()) {
    inFilePath = std::filesystem::path(*inFile);
  }

  if (outFile.has_value()) {
    outFilePath = std::filesystem::path(*outFile);
  }

  auto decoder = std::make_unique<Decoder>(rxPort, txPort, packed, keepFillBits,
                                           inFilePath, outFilePath);

  while (!stop) {
    decoder->main_loop();
  }

  return EXIT_SUCCESS;
}
