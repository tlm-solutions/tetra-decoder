#include "prometheus.h"

PrometheusExporter::PrometheusExporter(const std::string& prometheus_host, const std::string& prometheus_name) noexcept
    : prometheus_name_(std::move(prometheus_name)) {
    exposer_ = std::make_unique<prometheus::Exposer>(prometheus_host);
    registry_ = std::make_shared<prometheus::Registry>();

    exposer_->RegisterCollectable(registry_);
}

auto PrometheusExporter::burst_received_count() noexcept -> prometheus::Family<prometheus::Counter>& {
    return prometheus::BuildCounter()
        .Name("burst_received_count")
        .Help("Incrementing counter of the received bursts")
        .Labels({{"name", prometheus_name_}})
        .Register(*registry_);
}

auto PrometheusExporter::burst_lower_mac_decode_error_count() noexcept -> prometheus::Family<prometheus::Counter>& {
    return prometheus::BuildCounter()
        .Name("burst_lower_mac_decode_error_count")
        .Help("Incrementing counter of the decoding errors on received bursts in the lower MAC")
        .Labels({{"name", prometheus_name_}})
        .Register(*registry_);
}

auto PrometheusExporter::burst_lower_mac_mismatch_count() noexcept -> prometheus::Family<prometheus::Counter>& {
    return prometheus::BuildCounter()
        .Name("burst_lower_mac_mismatch_count")
        .Help("Incrementing counter of the number of mismatched bursts in the lower MAC on downlink")
        .Labels({{"name", prometheus_name_}})
        .Register(*registry_);
}
