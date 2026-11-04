# netrx

A system that sends and receives 1 million UDP packets, one every 10 microseconds,
and measures the lateness and latency of each one.

## What it measures

- **Lateness:** how far behind schedule the sender was when sending each packet
- **Latency:** time from a packet's scheduled send time to when the receiver gets it

Results are reported as p50, p95, p99, p99.9, max and average. Averages alone
are misleading: p50 and p99 can be low while a few very slow packets drag the
average up, so the focus is on the tail.

## Status

Work in progress. A sender and receiver are working, built on an RAII socket
wrapper and a sequence tracker that detects dropped and reordered packets.

Next: tuning the Linux kernel and system (core isolation, IRQ affinity, busy
polling, receive buffer size) to reduce the stalls that cause the slowest
packets, measuring each change separately.

## Build and run

```
cmake -B build && cmake --build build
taskset -c 2 ./build/receiver &
taskset -c 4 ./build/sender
```
