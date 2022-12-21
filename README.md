# Tetra Receiver

Receive multiple TETRA streams at once and send the bits out via UDP.

The tetra streams can be decoding using [`tetra-rx` from the osmocom tetra project](https://github.com/osmocom/osmo-tetra) or [`decoder` from the tetra-kit project](https://gitlab.com/larryth/tetra-kit).

Usage with tetra-rx: `socat STDIO UDP-LISTEN:42000 | stdbuf -i0 -o0 tetra-rx /dev/stdin`

## Usage
```
Receive multiple TETRA streams at once and send the bits out via UDP
Usage:
  tetra-receiver [OPTION...]

  -h, --help                  Print usage
      --rf arg                RF gain (default: 10)
      --if arg                IF gain (default: 10)
      --bb arg                BB gain (default: 10)
      --device-string arg     additional device arguments for osmosdr, see
                              https://projects.osmocom.org/projects/gr-osmos
                              dr/wiki/GrOsmoSDR (default: "")
      --center-frequency arg  Center frequency of the SDR (default: 0)
      --offsets arg           Offsets of the TETRA streams
      --samp-rate arg         Sample rate of the sdr (default: 1000000)
      --udp-start arg         Start UDP port. Each stream gets its own UDP
                              port, starting at udp-start (default: 42000)
```
