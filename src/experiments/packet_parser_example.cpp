/*
 * Copyright (C) 2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#include "borzoi/borzoi_packets.hpp"
#include "nlohmann/borzoi_receive_tetra_packet.hpp" // IWYU pragma: keep
#include <cstddef>
#include <cstdlib>
#include <cxxopts.hpp>
#include <filesystem>
#include <iostream>
#include <nlohmann/json_fwd.hpp>
#include <rapidcsv.h>
#include <string>

auto main(int argc, char** argv) -> int {
    std::string datafile;

    cxxopts::Options options("packet-parser-example", "Reads a csv file and extracts the contained packets.");

    // clang-format off
	options.add_options()
		("h,help", "Print usage")
		("datafile", "the path to the file containing historic packet data", cxxopts::value<std::string>(datafile)->default_value("datafile.csv"))
		;
    // clang-format on

    try {
        auto result = options.parse(argc, argv);

        if (result.count("help")) {
            std::cout << options.help() << std::endl;
            return EXIT_SUCCESS;
        }
    } catch (std::exception& e) {
        std::cout << "error parsing options: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    if (!std::filesystem::exists(datafile)) {
        std::cout << "Datafile does not exists." << std::endl;
        return EXIT_FAILURE;
    }

    /// The variable datafile contains the path to the csv file.
    std::cout << "Parsing datafile: " << datafile << std::endl;

    rapidcsv::Document doc(datafile);

    std::cout << "Read " << doc.GetRowCount() << " rows." << std::endl;

    auto column_names = doc.GetColumnNames();

    for (std::size_t i = 0; i < doc.GetRowCount(); i++) {
        auto row = doc.GetRow<std::string>(i);
        auto json = nlohmann::json::object();

        for (std::size_t i = 0; i < row.size(); i++) {
            auto value = row.at(i);
            auto column = column_names.at(i);
            if (column == "value") {
                json[column] = nlohmann::json::parse(value);
            } else {
                json[column] = value;
            }
        }

        std::cout << json << std::endl;
        BorzoiReceiveTetraPacket packet = json;
        std::cout << packet << std::endl;
    }

    return EXIT_SUCCESS;
}