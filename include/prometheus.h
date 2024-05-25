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

    auto burst_received_count() noexcept -> prometheus::Family<prometheus::Counter>&;
    auto burst_decode_error_count() noexcept -> prometheus::Family<prometheus::Counter>&;
};

#endif // PROMETHEUS_H
