
- M/O/C (Manditory/Optional/Conditional)
- OS (Optional if not present assume to be the same as that of the serving cell)

| Information Element | SYNC | SYSINFO | D-MLE-SYNC | D-MLE-SYSINFO | D-NWRK-BROADCAST | Neighbour cell information for CA |
| --- | --- | --- | --- | --- | --- | --- |
| System code | M | | | | |
| Colour code | M | | | | |
| Timeslot number | M | | | | |
| Frame number | M | | | | |
| Multiframe number | M | | | | |
| Sharing mode | M | | | | | OS (see "Timeshare cell information or security parameters", not the same but close enough) |
| TS reserved frames | M | | | | |
| U-plane DTX | M | | | | |
| Frame 18 extension | M | | | | |
| Main carrier | | M | | | | M |
| Frequency band | | M | | | | OS |
| Offset | | M | | | | OS |
| Duplex spacing | | M | | | | OS |
| Reverse operation | | M | | | | OS |
| Number of common secondary control channels in use on CA main carrier | | M | | | |
| MS_TXPWR_MAX_CELL (Maximum MS transmit power) | | M | | | | OS |
| RXLEV_ACCESS_MIN (Minimum RX access level) | | M | | | | OS |
| ACCESS_PARAMETER | | M | | | |
| RADIO_DOWNLINK_TIMEOUT | | M | | | |
| Hyperframe number | | C | | | |
| CCK identifier or static cipher key version number | | C | | | |
| TS_COMMON_FRAMES either for Even or Odd multiframes | | C | | | |
| Default definition for access code A | | C | | | |
| Extended services broadcast | | C | | | |
| -> Security information | | M | | | | OS  (see "Timeshare cell information or security parameters") |
| MNC | | | M | | | OS |
| MCC | | | M | | | OS |
| Neighbour cell broadcast | | | M | | |
| Cell load CA | | | M | | M | M |
| Late entry supported | | | M | | |
| Location area (LA) | | | | M | | OS |
| Subscriber class | | | | M | | OS |
| BS service details | | | | M | | OS |
| Cell re-select parameters | | | | | M |
| TETRA network time | | | | | O |
| Number of CA neighbour cells | | | | | O |
| Cell identifier CA | | | | | | M |
| Cell reselection types supported | | | | | | M |
| Neighbour cell synchronized | | | | | | M |
| Timeshare cell information or security parameters | | | | | | O (see relevant element) |
| TDMA frame offset | | | | | | O |
