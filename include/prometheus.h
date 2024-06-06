#ifndef PROMETHEUS_H
#define PROMETHEUS_H

#include <memory>

#include <prometheus/counter.h>
#include <prometheus/exposer.h>
#include <prometheus/registry.h>

class PrometheusExporter {
  private:
    std::shared_ptr<prometheus::Registry> registry_;
    std::unique_ptr<prometheus::Exposer> exposer_;
    const std::string prometheus_name_;

  public:
    PrometheusExporter(const std::string& prometheus_host, const std::string& prometheus_name) noexcept;
    ~PrometheusExporter() noexcept = default;

    /// The family of counters for received bursts
    auto burst_received_count() noexcept -> prometheus::Family<prometheus::Counter>&;
    /// The family of counters for decoding errors on received bursts in the lower MAC
    auto burst_lower_mac_decode_error_count() noexcept -> prometheus::Family<prometheus::Counter>&;
    /// The family of counters for mismatched number of bursts in the downlink lower MAC
    auto burst_lower_mac_mismatch_count() noexcept -> prometheus::Family<prometheus::Counter>&;
};

#endif // PROMETHEUS_H
