# Tetra Decoder

Decodes downlink TETRA stream.
Use with [tetra-receiver](https://github.com/marenz2569/tetra-receiver).

## Usage
```
Decodes TETRA downstream traffic
Usage:
  tetra-impl [OPTION...]

  -h, --help            Print usage
  -r, --rx arg          <UDP socket> receiving from phy (default: 42000)
  -t, --tx arg          <UDP socket> sending Json data (default: 42100)
  -i, --infile arg      <file> replay data from binary file instead of UDP
  -o, --outfile arg     <file> record data to binary file (can be replayed
                        with -i option)
  -d arg                <level> print debug information (default: 0)
  -f, --keep-fill-bits  keep fill bits
  -P, --packed          pack rx data (1 byte = 8 bits)
```
