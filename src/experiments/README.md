# Experiments using TETRA data

The borzoi integration allows to save all relevant packet data of a TETRA cell.
We can use this data to do queries and further analysis.

## Received Packets

Using a query we can extract all saved data through the grafana explore pane: `SELECT key, value::jsonb, time, station, protocol_version  FROM tetra_data WHERE $__timeFilter(time) ORDER BY time desc`.

To download the selected data as CSV click on: `Query inspector` -> `Data` -> `Download CSV`. The application `packet_parser_example` show how to convert this data back into the correct C++ structures.

## Slots with CRC errors

Query: `SELECT *  FROM tetra_failed_slots WHERE $__timeFilter(time) ORDER BY time desc`.

There is no example application provided showing how to convert the csv from grafana to C++ structures.
However, the principle is the same as in the `packet_parser_example`.