#include <csignal>
#include <cstdlib>
#include <cxxopts.hpp>
#include <memory>
#include <stdio.h>

#include <decoder.hpp>

static bool stop = false;

void sigint_handler(int s) {
    (void)s;

    stop = true;
}

int main(int argc, char** argv) {
    unsigned int receive_port, send_port, debug_level;
    bool packed;
    std::optional<std::string> input_file, output_file;
    std::optional<unsigned> uplink_scrambling_code;

    struct sigaction signal_action {};
    signal_action.sa_handler = sigint_handler;
    sigaction(SIGINT, &signal_action, 0);

    cxxopts::Options options("tetra-impl", "Decodes TETRA downstream traffic");

    // clang-format off
	options.add_options()
		("h,help", "Print usage")
		("r,rx", "<UDP socket> receiving from phy", cxxopts::value<unsigned>()->default_value("42000"))
		("t,tx", "<UDP socket> sending Json data", cxxopts::value<unsigned>()->default_value("42100"))
		("i,infile", "<file> replay data from binary file instead of UDP", cxxopts::value<std::optional<std::string>>(input_file))
		("o,outfile", "<file> record data to binary file (can be replayed with -i option)", cxxopts::value<std::optional<std::string>>(output_file))
		("d", "<level> print debug information", cxxopts::value<unsigned>()->default_value("0"))
		("P,packed", "pack rx data (1 byte = 8 bits)", cxxopts::value<bool>()->default_value("false"))
		("uplink", "<scrambling code> enable uplink parsing with predefined scrambilng code", cxxopts::value<std::optional<unsigned>>(uplink_scrambling_code))
		;
    // clang-format on

    try {
        auto result = options.parse(argc, argv);

        if (result.count("help")) {
            std::cout << options.help() << std::endl;
            exit(0);
        }

        receive_port = result["rx"].as<unsigned>();
        send_port = result["tx"].as<unsigned>();

        debug_level = result["d"].as<unsigned>();
        packed = result["packed"].as<bool>();
    } catch (std::exception& e) {
        std::cout << "error parsing options: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    auto decoder =
        std::make_unique<Decoder>(receive_port, send_port, packed, input_file, output_file, uplink_scrambling_code);

    if (input_file.has_value()) {
        std::cout << "Reading from input file " << *input_file << std::endl;
    } else {
        std::cout << "Listening on UDP socket " << receive_port << std::endl;
    }
    std::cout << "Sending on UDP socket " << send_port << std::endl;
    if (output_file.has_value()) {
        std::cout << "Writing to output file " << *output_file << std::endl;
    }

    while (!stop) { // NOLINT handled by signal action
        decoder->main_loop();
    }

    return EXIT_SUCCESS;
}
