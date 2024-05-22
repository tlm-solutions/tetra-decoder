#include "prometheus.h"

PrometheusExporter::PrometheusExporter(const std::string& prometheus_host) noexcept {
    exposer_ = std::make_unique<prometheus::Exposer>(prometheus_host);
    registry_ = std::make_shared<prometheus::Registry>();

    exposer_->RegisterCollectable(registry_);
}

auto PrometheusExporter::burst_received_count() noexcept -> prometheus::Family<prometheus::Counter>& {
    return prometheus::BuildCounter()
        .Name("burst_received_count")
        .Help("Incrementing counter of the received bursts")
        .Register(*registry_);
}

auto PrometheusExporter::burst_decode_error_count() noexcept -> prometheus::Family<prometheus::Counter>& {
    return prometheus::BuildCounter()
        .Name("burst_decode_error_count")
        .Help("Incrementing counter of the decoding errors on received bursts")
        .Register(*registry_);
}
