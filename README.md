# Ghostroute
Ghostroute is a tool demonstrate the use of raw and packet sockets to simulate/fake extra hops in a traceroute.  
For this to work, the target IPv6 address should be routed to, but not configured on the host running ghostroute.

When using an address configured on the host, use a firewall or disable ICMP on the host to prevent the host from responding. Linux will respond with a hop limit of 64, which can be used to filter out host replies.  
If forwarding is enabled on the host, add a blackhole route for the target to prevent replies from the host:
```sh
$ sudo ip -6 r a blackhole 2001:db8::abcd:1234/128
```
For replies to reach the initiating host over the internet, the addresses used as hops should be unfiltered by the uplink. Using addresses from the prefix routed to you by your provider should cover this.

Access to a reverse zone for the prefix is required to have hops resolve to a custom string, but not required to use this tool.

## Build
No dependencies required, except for a recent c++23 (ie. GCC 14) compiler and CMake.
```sh
$ mkdir build
$ cmake -B build -S .
$ cmake --build build
```

## Run
Create settings.json in the working directory according to the following format:
```json
{
  "settings": {
    "target": "2001:db8::abcd:1234",
    "icmp_hops": [
      "2001:db8::ffff:ffff",
      "2001:db8::ffff:aaaa",
      "2001:db8::ffff:bbbb",
      "2001:db8::ffff:cccc"
    ],
    "udp_hops": [
      "2001:db8::aaaa:ffff",
      "2001:db8::aaaa:1111",
      "2001:db8::bbbb:2222",
      "2001:db8::cccc:3333"
    ]
  }
}
```
'target' is the address to listen to.  
'icmp_hops' and 'udp_hops' are the hops to respond with to icmp or udp source packets. The first hop will only be visible on local links.

Elevated priviliges are required for the 'CAP_NET_RAW' capability.

With the config in place, simply run `$ sudo ./ghostroute`.

Example traceroute output run from a neighboring machine:
```sh
$ traceroute -n 2001:db8::abcd:1234
traceroute to 2001:db8::abcd:1234 (2001:db8::abcd:1234), 30 hops max, 80 byte packets
 1  2001:db8::aaaa:1111  1.335 ms  1.059 ms  0.925 ms
 2  2001:db8::bbbb:2222  0.800 ms  0.668 ms  0.535 ms
 3  2001:db8::cccc:3333  0.613 ms  0.474 ms  0.510 ms
 4  2001:db8::abcd:1234  0.360 ms  0.500 ms  0.361 ms
```
Reaching the first hop on a local link:
```sh
$ ping -t 0 -c 1 2001:db8::abcd:1234
PING 2001:db8::abcd:1234(2001:db8::abcd:1234) 56 data bytes
From 2001:db8::ffff:ffff icmp_seq=1 Time exceeded: Hop limit
```

## Caveats
ECMP could probably mess with results, especially with multiple probes per hop.  
With forwarding enabled, the host itself will still reply and be visible as a hop.