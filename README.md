# netrx

A system that sends and receives 1 million UDP packets, one every 10 microseconds,
and measures the lateness and latency of each one.

## What it measures

- **Lateness:** how far behind schedule the sender was when sending each packet
- **Latency:** time from a packet's scheduled send time to when the receiver gets it
- **Pipeline latency:** time from when the packet was actually sent to when the
  receiver gets it, which shows the receiver's side without the sender's delays

Results are reported as p50, p95, p99, p99.9, max and average. Averages alone
are misleading: p50 and p99 can be low while a few very slow packets drag the
average up, so the focus is on the tail.

## Tuning results (loopback)

All results are on a Debian 13 server (kernel 6.12, 4 cores / 8 threads) over
UDP loopback. Each change was measured separately, over at least 5 runs.

### The biggest change came from my own code

The biggest change came from my own code, not a kernel setting. The sender took
its start time before creating two 8 MB sample vectors. Creating them also
writes to them, which caused about 4,000 page faults, so the first ~600 packets
were recorded as late. Moving one line cut the worst-case latency from about
5 ms to under 140 µs. When you create a large vector, you have to think about
paging, especially before anything you're timing.

### Busy polling

Busy polling was the biggest receiver-side win. Instead of sleeping until a
packet arrives, the receiver never sleeps and keeps checking for packets. That
removes the wake-up cost, and the expensive wake-ups were a big part of the
receive tail. Pipeline latency p50 dropped from about 3.0–3.3 µs to 1.7–2.1 µs,
and p99.9 from 5.3–13.6 µs to about 3.7–4.2 µs, with much less run-to-run
spread. The cost is a whole CPU core spent checking for packets.

### What didn't help

Limiting idle states, isolating the receiver's core, stopping the timer tick
(`nohz_full`) and making the sender sleep between packets did little or
nothing, and some made things worse: `nohz_full` and sleep-then-spin both made
`sendto` slower. Sometimes your own code makes more of a difference than kernel
settings. Every knob has a cost, so measure before keeping one.

Full kernel preemption (`preempt=full`) was the one kernel setting I kept:
across 10 runs it kept the sender's worst case under 100 µs in 8 of them,
against 255 µs to 1.4 ms with the default, at no measurable cost to the median.

### Before and after

Sleep-mode receiver, latency from the scheduled send time, 5 runs each.

| | First server baseline | After tuning |
|---|---|---|
| p50 | 3.36–3.39 µs | 3.04–3.29 µs |
| p99.9 | 25.6–201 µs | 15.0–25.0 µs |
| max | 5.9–6.8 ms | 83–360 µs |

## Roadmap

- Real network link between two machines, measured by round trip since the
  machines don't share a clock
- Kernel bypass with AF_XDP
- An order book fed by the receiver
- Replaying a real exchange feed (Nasdaq ITCH) and a 24-hour stability run

## Build and run

```
cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build
mkdir -p results
sudo taskset -c 2 chrt -f 99 ./build/receiver sleep &
sleep 1; sudo taskset -c 4 chrt -f 99 ./build/sender
```

The receiver takes a mode, `sleep` (blocking `recv`) or `busy` (busy polling),
and an optional receive buffer size in bytes. Both programs write raw samples
to `results/`, and `scripts/plot.py` turns them into plots.
