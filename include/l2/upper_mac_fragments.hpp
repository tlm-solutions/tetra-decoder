/*
 * Copyright (C) 2024 Transit Live Mapping Solutions
 * All rights reserved.
 *
 * Authors:
 *   Marenz Schmidl
 */

#pragma once

#include "l2/upper_mac_packet.hpp"
#include "prometheus.h"
#include "utils/address.hpp"
#include <cassert>
#include <memory>
#include <optional>
#include <set>
#include <stdexcept>
#include <vector>

class UpperMacFragmentsPrometheusCounters {
  private:
    /// The prometheus exporter
    std::shared_ptr<PrometheusExporter> prometheus_exporter_;

    // NOLINTBEGIN(cppcoreguidelines-avoid-const-or-ref-data-members)

    /// The family of counters for received fragments
    prometheus::Family<prometheus::Counter>& fragment_count_family_;
    /// The counter for total received fragments
    prometheus::Counter& fragment_count_total_;
    /// The counter for received fragments that could not be reassembled
    prometheus::Counter& fragment_count_error_;

    // NOLINTEND(cppcoreguidelines-avoid-const-or-ref-data-members)

  public:
    UpperMacFragmentsPrometheusCounters() = delete;
    explicit UpperMacFragmentsPrometheusCounters(const std::shared_ptr<PrometheusExporter>& prometheus_exporter,
                                                 const std::string& type)
        : prometheus_exporter_(prometheus_exporter)
        , fragment_count_family_(prometheus_exporter_->upper_mac_fragment_count())
        , fragment_count_total_(fragment_count_family_.Add({{"type", type}, {"counter_type", "All"}}))
        , fragment_count_error_(
              fragment_count_family_.Add({{"type", type}, {"counter_type", "Reconstuction Error"}})){};

    /// This function is called for every fragment where no fitting previous fragment could be found.
    auto increment_fragment_reconstruction_error() -> void { fragment_count_error_.Increment(); }
    /// This function is called for every fragment.
    auto increment_fragment_count() -> void { fragment_count_total_.Increment(); }
};

/// Class that provides the fragment reconstruction for downlink packets.
class UpperMacDownlinkFragmentation {
  private:
    /// Holds the internal state of the fragment rebuilder
    enum class State {
        kStart,
        kStartFragmentReceived,
        kContinuationFragmentReceived,
        kEndFragmentReceived,
    };
    /// The vector that holds the accumulated fragments
    std::vector<UpperMacCPlaneSignallingPacket> fragments_;
    /// Are continuation allowed in the state machine?
    std::map<State, std::set<State>> allowed_state_changes_;
    /// The current state of the fragment reassembler
    State state_;

    /// the metrics for the fragmentation
    std::shared_ptr<UpperMacFragmentsPrometheusCounters> metrics_;

    /// Try the state transtition with a fragment. Increment the error metrics if there is an invalid state transition
    /// attempted
    /// \param new_state the new state into which the state machine would be transfered with this fragment
    /// \param fragment the control plane signalling packet that is fragmented
    /// \return an optional reconstructed control plane signalling packet when reconstuction was successful
    auto change_state(State new_state,
                      const UpperMacCPlaneSignallingPacket& fragment) -> std::optional<UpperMacCPlaneSignallingPacket> {
        const auto& valid_state_changes = allowed_state_changes_[state_];

        // increment the total fragment counters
        if (metrics_) {
            metrics_->increment_fragment_count();
        }

        if (valid_state_changes.count(new_state)) {
            // valid state change. perform and add fragment
            fragments_.emplace_back(fragment);
            state_ = new_state;
        } else {
            // increment the invalid state metrics
            if (metrics_) {
                metrics_->increment_fragment_reconstruction_error();
            }

            // always save the start segment
            if (new_state == State::kStartFragmentReceived) {
                fragments_ = {fragment};
                state_ = State::kStartFragmentReceived;
            } else {
                fragments_.clear();
                state_ = State::kStart;
            }
        }

        // if we are in the end state reassmeble the packet.
        if (state_ == State::kEndFragmentReceived) {
            std::optional<UpperMacCPlaneSignallingPacket> packet;
            for (const auto& fragment : fragments_) {
                if (packet) {
                    packet->tm_sdu_->append(*fragment.tm_sdu_);
                } else {
                    packet = fragment;
                }
            }
            fragments_.clear();
            state_ = State::kStart;

            return packet;
        }

        return std::nullopt;
    };

  public:
    UpperMacDownlinkFragmentation() = delete;

    /// Constructor for the fragmentations. Optionally specify if an arbitraty numner of continuation fragments are
    /// allowed
    explicit UpperMacDownlinkFragmentation(const std::shared_ptr<UpperMacFragmentsPrometheusCounters>& metrics,
                                           bool continuation_fragments_allowed = true)
        : state_(State::kStart)
        , metrics_(metrics) {
        if (continuation_fragments_allowed) {
            allowed_state_changes_ = {
                {State::kStart, {State::kStartFragmentReceived}},
                {State::kStartFragmentReceived, {State::kContinuationFragmentReceived, State::kEndFragmentReceived}},
                {State::kContinuationFragmentReceived,
                 {State::kContinuationFragmentReceived, State::kEndFragmentReceived}},
                {State::kEndFragmentReceived, {State::kStart}}};
        } else {
            allowed_state_changes_ = {{State::kStart, {State::kStartFragmentReceived}},
                                      {State::kStartFragmentReceived, {State::kEndFragmentReceived}},
                                      {State::kEndFragmentReceived, {State::kStart}}};
        }
    };

    /// Check if we are in the start state i.e., do no have any fragments.
    auto is_in_start_state() -> bool { return state_ == State::kStart; }

    /// Push a fragment for reconstruction.
    /// \param fragment the control plane signalling packet that is fragmented
    /// \return an optional reconstructed control plane signalling packet when reconstuction was successful
    auto
    push_fragment(const UpperMacCPlaneSignallingPacket& fragment) -> std::optional<UpperMacCPlaneSignallingPacket> {
        switch (fragment.type_) {
        case MacPacketType::kMacResource:
            assert(fragment.fragmentation_);
            return change_state(State::kStartFragmentReceived, fragment);
        case MacPacketType::kMacFragmentDownlink:
            return change_state(State::kContinuationFragmentReceived, fragment);
        case MacPacketType::kMacEndDownlink:
            return change_state(State::kEndFragmentReceived, fragment);
        case MacPacketType::kMacDBlck:
            throw std::runtime_error("No fragmentation in MacDBlck");
        case MacPacketType::kMacBroadcast:
            throw std::runtime_error("No fragmentation in MacBroadcast");
        case MacPacketType::kMacAccess:
            throw std::runtime_error("MacAccess is not handled by UpperMacDownlinkFragmentation");
        case MacPacketType::kMacData:
            throw std::runtime_error("MacData is not handled by UpperMacDownlinkFragmentation");
        case MacPacketType::kMacFragmentUplink:
            throw std::runtime_error("MacFragmentUplink is not handled by UpperMacDownlinkFragmentation");
        case MacPacketType::kMacEndHu:
            throw std::runtime_error("MacEndHu is not handled by UpperMacDownlinkFragmentation");
        case MacPacketType::kMacEndUplink:
            throw std::runtime_error("MacEndUplink is not handled by UpperMacDownlinkFragmentation");
        case MacPacketType::kMacUBlck:
            throw std::runtime_error("No fragmentation in MacUBlck");
        case MacPacketType::kMacUSignal:
            throw std::runtime_error("No fragmentation in MacUSignal");
        }
    };
};

/// Class that provides the fragment reconstruction for uplink packets.
/// Uplink fragmentation may include reserved slots and is therefore harder to reconstruct. This is not handled
/// with this class.
class UpperMacUplinkFragmentation {
  private:
    /// Holds the internal state of the fragment rebuilder
    enum class State {
        kStart,
        kStartFragmentReceived,
        kContinuationFragmentReceived,
        kEndFragmentReceived,
    };
    /// The vector that holds the accumulated fragments for each mobile station by its address
    std::map<Address, std::vector<UpperMacCPlaneSignallingPacket>> fragments_per_address_;
    /// Are continuation allowed in the state machine?
    std::map<State, std::set<State>> allowed_state_changes_;
    /// The current state of the fragment reassembler for each mobile station by its address
    std::map<Address, State> state_per_address_;

    /// the metrics for the fragmentation
    std::shared_ptr<UpperMacFragmentsPrometheusCounters> metrics_;

    /// Try the state transtition with a fragment. Increment the error metrics if there is an invalid state transition
    /// attempted
    /// \param new_state the new state into which the state machine would be transfered with this fragment
    /// \param fragment the control plane signalling packet that is fragmented
    /// \return an optional reconstructed control plane signalling packet when reconstuction was successful
    auto change_state(State new_state,
                      const UpperMacCPlaneSignallingPacket& fragment) -> std::optional<UpperMacCPlaneSignallingPacket> {
        std::cout << fragment << std::endl;
        const auto& address = fragment.address_;
        auto& state = state_per_address_[address];
        auto& fragments = fragments_per_address_[address];

        const auto& valid_state_changes = allowed_state_changes_[state];

        // increment the total fragment counters
        if (metrics_) {
            metrics_->increment_fragment_count();
        }

        if (valid_state_changes.count(new_state)) {
            // valid state change. perform and add fragment
            fragments.emplace_back(fragment);
            state = new_state;
        } else {
            // increment the invalid state metrics
            if (metrics_) {
                metrics_->increment_fragment_reconstruction_error();
            }

            // always save the start segment
            if (new_state == State::kStartFragmentReceived) {
                fragments = {fragment};
                state = State::kStartFragmentReceived;
            } else {
                fragments.clear();
                state = State::kStart;
            }
        }

        // if we are in the end state reassmeble the packet.
        if (state == State::kEndFragmentReceived) {
            std::optional<UpperMacCPlaneSignallingPacket> packet;
            for (const auto& fragment : fragments) {
                if (packet) {
                    packet->tm_sdu_->append(*fragment.tm_sdu_);
                } else {
                    packet = fragment;
                }
            }
            fragments.clear();
            state = State::kStart;

            return packet;
        }

        return std::nullopt;
    };

  public:
    UpperMacUplinkFragmentation() = delete;

    /// Constructor for the fragmentations. Optionally specify if an arbitraty numner of continuation fragments are
    /// allowed
    explicit UpperMacUplinkFragmentation(const std::shared_ptr<UpperMacFragmentsPrometheusCounters>& metrics)
        : metrics_(metrics) {
        allowed_state_changes_ = {
            {State::kStart, {State::kStartFragmentReceived}},
            {State::kStartFragmentReceived, {State::kContinuationFragmentReceived, State::kEndFragmentReceived}},
            {State::kContinuationFragmentReceived, {State::kContinuationFragmentReceived, State::kEndFragmentReceived}},
            {State::kEndFragmentReceived, {State::kStart}}};
    };

    /// Push a fragment for reconstruction.
    /// \param fragment the control plane signalling packet that is fragmented
    /// \return an optional reconstructed control plane signalling packet when reconstuction was successful
    auto
    push_fragment(const UpperMacCPlaneSignallingPacket& fragment) -> std::optional<UpperMacCPlaneSignallingPacket> {
        switch (fragment.type_) {
        case MacPacketType::kMacResource:
            throw std::runtime_error("MacResource is not handled by UpperMacUplinkFragmentation");
        case MacPacketType::kMacFragmentDownlink:
            throw std::runtime_error("MacFragmentDownlink is not handled by UpperMacUplinkFragmentation");
        case MacPacketType::kMacEndDownlink:
            throw std::runtime_error("MacEndDownlink is not handled by UpperMacUplinkFragmentation");
        case MacPacketType::kMacDBlck:
            throw std::runtime_error("No fragmentation in MacDBlck");
        case MacPacketType::kMacBroadcast:
            throw std::runtime_error("No fragmentation in MacBroadcast");
        case MacPacketType::kMacAccess:
        case MacPacketType::kMacData:
            assert(fragment.fragmentation_);
            return change_state(State::kStartFragmentReceived, fragment);
        case MacPacketType::kMacFragmentUplink:
            return change_state(State::kContinuationFragmentReceived, fragment);
        case MacPacketType::kMacEndHu:
            throw std::runtime_error("MacEndHu is in a reserverd subslot and not handled since there is no "
                                     "integration between the uplink and downlink processing");
        case MacPacketType::kMacEndUplink:
            return change_state(State::kEndFragmentReceived, fragment);
        case MacPacketType::kMacUBlck:
            throw std::runtime_error("No fragmentation in MacUBlck");
        case MacPacketType::kMacUSignal:
            throw std::runtime_error("No fragmentation in MacUSignal");
        }
    };
};