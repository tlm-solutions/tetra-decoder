#include <ctime>
#include <fmt/color.h>
#include <fmt/core.h>
#include <sstream>

#include <reporter.hpp>

Reporter::Reporter(unsigned send_port) {
    // output file descriptor for the udp socket
    // there goes our nice json
    std::memset(&destination, 0, sizeof(struct sockaddr_in));
    destination.sin_family = AF_INET;
    destination.sin_port = htons(send_port);
    destination.sin_addr.s_addr = inet_addr("127.0.0.1");

    output_socket_fd_ = socket(AF_INET, SOCK_DGRAM, 0);

    if (output_socket_fd_ < 0) {
        throw std::runtime_error("Couldn't create output socket");
    }
}

Reporter::~Reporter() { close(output_socket_fd_); }

inline static auto get_time() -> std::string {
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    std::stringstream ss;
    ss << std::put_time(&tm, "%F_%T%z");
    return ss.str();
}

void Reporter::emit_report(nlohmann::json& message) {
    message["time"] = get_time();

    std::string message_str = message.dump() + "\n";

    int n_bytes = sendto(output_socket_fd_, message_str.c_str(), message_str.length(), 0,
                         reinterpret_cast<struct sockaddr*>(&(this->destination)), sizeof(destination));

    if (n_bytes < 0) {
        fmt::print(fg(fmt::color::crimson) | fmt::emphasis::bold, "[ERROR] could not send data over UDP socket");
        // TODO: handle error
    }
}
