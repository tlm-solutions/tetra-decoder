#include <cassert>
#include <l3/circuit_mode_control_entity.hpp>
#include <memory>

auto CircuitModeControlEntity::process(const MobileLinkEntityPacket& packet)
    -> std::unique_ptr<CircuitModeControlEntityPacket> {
    auto cmce_packet = packet_builder_.parse_mobile_link_entity(packet);

    if (metrics_) {
        metrics_->increment(to_string(cmce_packet->packet_type_));
    }

    return cmce_packet;
}