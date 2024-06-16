# Metrics

This TETRA reception stack comes with a lot of usefull metrics for monitoring the health and troughput of a cell.
All metrics are available through a prometheus exposer configurable with the command line arguments `--prometheus-address` and `--prometheus-name`.

Following metrics are supported:

| Metric name | Type | Description | Labels |
|---|---|---|---|
| `burst_received_count` | Counter | Counters for received bursts | `burst_type`: The type of received burst |
| `burst_lower_mac_decode_error_count` | Counter | Counters for decoding errors on received bursts in the lower MAC | `burst_type`: The type of received burst |
| `burst_lower_mac_mismatch_count` | Counter | Counters for mismatched number of bursts in the downlink lower MAC | `mismatch_type`: Any of `Skipped` or `Too many` |
| `lower_mac_time_gauge` | Gauge | Gauges for the network time | `type`: Any of `Synchronization Burst` or `Prediction` |
| `upper_mac_total_slot_count` | Counter | Counters for all received slots | `logical_channel`: The logical channel that is contained in the slot. |
| `upper_mac_slot_error_count` | Counter | Counters for all received slots with errors | `logical_channel`: The logical channel that is contained in the slot. `error_type`: Any of `CRC Error` or `Decode Error`. This includes errors in decoding for the upper mac or any layer on above. Errors in decoding reconstructed fragments are reported in the slot of the last fragment. |
| `upper_mac_packet_count` | Counter | Counters for all received upper mac packets | `packet_type`: The type of receivesd upper mac packet. |
| `upper_mac_fragment_count` | Counter | Counters for all received c-plane fragments | `type`: Any of `Continous` or `Stealing Channel`. `counter_type`: Any of `All` or `Reconstuction Error`. If there was a disallowed state transition in the reconstruction, the counter is incremented. Additional for  `Stealing Channel` the counter is incremented if the fragment was not finalized across the stealing channel. |
| `c_plane_packet_count` | Counter | Counters for all received c-plane packets before fragment reconstruction. | `packet_type`: The mac packet type for c-plane packets. (Start fragments are seperated) |