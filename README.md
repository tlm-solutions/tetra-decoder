# Tetra Decoder

This TETRA decoder support decoding signals of Conventional Access π/4-DQPSK.
Only continuous downlink transmittion and uplink transmittion is supported (no discontinuous downlink transmittion).
Use it together with [tetra-receiver](https://github.com/marenz2569/tetra-receiver).

## Usage
```
Decodes TETRA Conventional Access π/4-DQPSK traffic
Usage:
  tetra-impl [OPTION...]

  -h, --help         Print usage
  -r, --rx arg       <UDP socket> receiving from phy (default: 42000)
  -t, --tx arg       <UDP socket> sending Json data (default: 42100)
  -i, --infile arg   <file> replay data from binary file instead of UDP
  -o, --outfile arg  <file> record data to binary file (can be replayed
                     with -i option)
  -d arg             <level> print debug information (default: 0)
  -P, --packed       pack rx data (1 byte = 8 bits)
      --uplink arg   <scrambling code> enable uplink parsing with
                     predefined scrambilng code
```
