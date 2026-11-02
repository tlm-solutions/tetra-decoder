/*
 * Copyright (C) 2026 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#pragma once

#include "application/lio_packet.hpp"
#include "l3/short_data_service_packet.hpp"
#include "utils/packet_parser.hpp"

class LioParser : public PacketParser<ShortDataServicePacket, LioPacket> {
  public:
    LioParser() = delete;
    explicit LioParser(const std::shared_ptr<PrometheusExporter>& prometheus_exporter)
        : PacketParser(prometheus_exporter, "application_lio") {
        for (auto i = 0; i < uplink_lio_pdu_description_.size(); i++) {
            uplink_lio_pdu_description_.at(i) = "Unknown Uplink id " + std::to_string(i);
        }

        uplink_lio_pdu_description_[0x03] = "Uplink RequestToTalk";
        uplink_lio_pdu_description_[0x06] = "Uplink DriverMessage";
        uplink_lio_pdu_description_[0x0e] = "Uplink DeleteMessage";
        uplink_lio_pdu_description_[0x0f] = "Uplink CallAccept";
        uplink_lio_pdu_description_[0x20] = "Uplink VehicleLogoff";
        uplink_lio_pdu_description_[0x44] = "Uplink TotalDistance";
        uplink_lio_pdu_description_[0x45] = "Uplink VehicleMultiAck";
        uplink_lio_pdu_description_[0x49] = "Uplink DepartureMessage";
        uplink_lio_pdu_description_[0x63] = "Uplink LocationCode";
        uplink_lio_pdu_description_[0x70] = "Uplink Occupation";
        uplink_lio_pdu_description_[0x92] = "Uplink TripStart";
        uplink_lio_pdu_description_[0x94] = "Uplink TrainAssembly";
        uplink_lio_pdu_description_[0xa1] = "Uplink VehicleAnswer71";
        uplink_lio_pdu_description_[0xa2] = "Uplink DataVersion";
        uplink_lio_pdu_description_[0xd1] = "Uplink SICULogon";
        uplink_lio_pdu_description_[0xd4] = "Uplink Ping1";
        uplink_lio_pdu_description_[0xd6] = "Uplink SICUSoftwareVersion";
        uplink_lio_pdu_description_[0xd7] = "Uplink SICUGroupState";
        uplink_lio_pdu_description_[0xda] = "Uplink SICUAck";
        uplink_lio_pdu_description_[0xdd] = "Uplink SICUMultiAck";
        uplink_lio_pdu_description_[0xd9] = "Uplink SICUDataVersion";
        uplink_lio_pdu_description_[0xde] = "Uplink LayoutStatus";
        uplink_lio_pdu_description_[0xdf] = "Uplink SICUMultiBlockAck";
        uplink_lio_pdu_description_[0xe1] = "Uplink Trip";
        uplink_lio_pdu_description_[0xe3] = "Uplink GPS";

        for (auto i = 0; i < downlink_lio_pdu_description_.size(); i++) {
            downlink_lio_pdu_description_.at(i) = "Unknown Downlink id " + std::to_string(i);
        }

        downlink_lio_pdu_description_[0x00] = "Downlink Sync";
        downlink_lio_pdu_description_[0x0e] = "Downlink TimeSlotAllocation";
        downlink_lio_pdu_description_[0x0f] = "Downlink DateTime";
        downlink_lio_pdu_description_[0x11] = "Downlink Ack";
        downlink_lio_pdu_description_[0x2a] = "Downlink DisplaySwitchCmd";
        downlink_lio_pdu_description_[0x30] = "Downlink SwitchToVoiceMode";
        downlink_lio_pdu_description_[0x37] = "Downlink StopAnnouncement";
        downlink_lio_pdu_description_[0x42] = "Downlink IbisText1";
        downlink_lio_pdu_description_[0x49] = "Downlink IbisText2";
        downlink_lio_pdu_description_[0x70] = "Downlink RequestEvent";
        downlink_lio_pdu_description_[0x71] = "Downlink RequestSoftwareVersion";
        downlink_lio_pdu_description_[0x73] = "Downlink RequestDataVersion";
        downlink_lio_pdu_description_[0x90] = "Downlink InfosetDefinition";
        downlink_lio_pdu_description_[0x92] = "Downlink DisplayLayoutCmd";
        downlink_lio_pdu_description_[0x93] = "Downlink InfosetLayoutCmd";
        downlink_lio_pdu_description_[0x94] = "Downlink DfiText";
        downlink_lio_pdu_description_[0x95] = "Downlink ReplaceTrip";
    };

  private:
    [[nodiscard]] auto packet_name(const LioPacket& packet) const -> std::string override {
        return packet.is_uplink() ? uplink_lio_pdu_description_.at(packet.lio_protocol_identifier_)
                                  : downlink_lio_pdu_description_.at(packet.lio_protocol_identifier_);
    }

    auto forward(const LioPacket& packet) -> std::unique_ptr<LioPacket> override {
        return std::make_unique<LioPacket>(packet);
    };

    std::array<std::string, 256> uplink_lio_pdu_description_;
    std::array<std::string, 256> downlink_lio_pdu_description_;
};