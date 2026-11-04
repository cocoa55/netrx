**Test:** sender pacing loop only, with no network. 1,000,000 iterations at a 10 µs interval, busy-waiting on `steady_clock`. Lateness = actual time minus scheduled time.

**1. Desktop (Arch, unoptimized build), 4 runs**
- p50: 31–33 ns
- p99: 39–41 ns
- p99.9: 7–15 µs
- max: 1.25–1.33 ms, every run

**2. Server (Debian, Release build, normal priority), 3 runs**
- p50: 29 ns
- p99: 37 ns
- p99.9: 15–17 µs
- max: 3.8–4.0 ms, every run

**3. Server, three containers stopped (syncthing, jellyfin, dsa-logger), 3 runs**
- p99.9: 12–37 µs
- max: 3.5–3.75 ms
- No meaningful change, so those containers aren’t the cause.

**4. Server, real-time priority (`chrt -f 99`), 3 runs**
- p99.9: 17–18 µs
- max: 2.7–2.9 ms
- Average down from about 850 ns to about 510 ns.

**What it means**
- The pacing loop itself is precise: 99% of packets are within about 40 ns of schedule: on both machines.
- The tail is dominated by stalls of milliseconds that recur in every run.
- On the server, normal processes cause about 1 ms of the stall.
- About 2.8 ms remains that process priority can’t remove. The cause is unknown; phase 2 tuning will test it.
- The server has a worse tail than the desktop, despite fewer background tasks.
