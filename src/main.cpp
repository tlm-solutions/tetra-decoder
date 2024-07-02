#include "decoder.hpp"
#include <csignal>
#include <cstdlib>
#include <cxxopts.hpp>
#include <memory>
#include <signal_handler.hpp>

volatile bool stop = false;

void sigint_handler(int s) {
    (void)s;

    stop = true;
}

auto main(int argc, char** argv) -> int {
    unsigned int receive_port;
    bool packed;
    bool iq_or_bit_stream;
    std::optional<std::string> input_file;
    std::optional<std::string> output_file;
    std::string borzoi_url;
    std::string borzoi_uuid;
    std::optional<unsigned> uplink_scrambling_code;
    std::optional<std::string> prometheus_address;
    std::optional<std::string> prometheus_name;

    std::shared_ptr<PrometheusExporter> prometheus_exporter;

    struct sigaction signal_action {};
    signal_action.sa_handler = sigint_handler;
    sigaction(SIGINT, &signal_action, 0);

    cxxopts::Options options("tetra-decoder", "Decodes TETRA downstream traffic");

    // clang-format off
	options.add_options()
		("h,help", "Print usage")
		("r,rx", "<UDP socket> receiving from phy", cxxopts::value<unsigned>()->default_value("42000"))
		("t,tx", "<UDP socket> sending Json data", cxxopts::value<unsigned>()->default_value("42100"))		
		("borzoi-url", "<borzoi-url> the base url of which borzoi is running", cxxopts::value<std::string>(borzoi_url)->default_value("http://localhost:3000"))
		("borzoi-uuid", "<borzoi-uuid> the UUID of this tetra-decoder sending data to borzoi", cxxopts::value<std::string>(borzoi_uuid)->default_value("00000000-0000-0000-0000-000000000000"))
		("i,infile", "<file> replay data from binary file instead of UDP", cxxopts::value<std::optional<std::string>>(input_file))
		("o,outfile", "<file> record data to binary file (can be replayed with -i option)", cxxopts::value<std::optional<std::string>>(output_file))
		("P,packed", "pack rx data (1 byte = 8 bits)", cxxopts::value<bool>()->default_value("false"))
		("iq", "Receive IQ instead of bitstream", cxxopts::value<bool>()->default_value("false"))
		("uplink", "<scrambling code> enable uplink parsing with predefined scrambilng code", cxxopts::value<std::optional<unsigned>>(uplink_scrambling_code))
		("prometheus-address", "<prometheus-address> on which ip and port the webserver for prometheus should listen. example: 127.0.0.1:9010", cxxopts::value<std::optional<std::string>>(prometheus_address))
		("prometheus-name", "<prometheus-name> the name which is included in the prometheus metrics", cxxopts::value<std::optional<std::string>>(prometheus_name))
		;
    // clang-format on

    try {
        auto result = options.parse(argc, argv);

        if (result.count("help")) {
            std::cout << options.help() << std::endl;
            return EXIT_SUCCESS;
        }

        receive_port = result["rx"].as<unsigned>();

        packed = result["packed"].as<bool>();
        iq_or_bit_stream = result["iq"].as<bool>();

        if (prometheus_address) {
            prometheus_exporter = std::make_shared<PrometheusExporter>(
                *prometheus_address, prometheus_name.value_or("Unnamed Tetra Decoder"));
        }
    } catch (std::exception& e) {
        std::cout << "error parsing options: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    auto decoder = std::make_unique<Decoder>(receive_port, borzoi_url, borzoi_uuid, packed, input_file, output_file,
                                             iq_or_bit_stream, uplink_scrambling_code, prometheus_exporter);

    if (input_file.has_value()) {
        std::cout << "Reading from input file " << *input_file << std::endl;
    } else {
        std::cout << "Listening on UDP socket " << receive_port << std::endl;
    }
    std::cout << "Sending to Borzoi on: " << borzoi_url << std::endl;
    if (output_file.has_value()) {
        std::cout << "Writing to output file " << *output_file << std::endl;
    }

    while (!stop) { // NOLINT handled by signal action
        decoder->main_loop();
    }

    return EXIT_SUCCESS;
}
