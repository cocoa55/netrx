**Run set A: 10 µs interval, with `sendto`, lateness measured after `sendto`**

|Run|avg|p50|p95|p99|p99.9|max|
|---|---|---|---|---|---|---|
|1|1350 ns|32 ns|40 ns|16.7 µs|278.3 µs|1.45 ms|
|2|506 ns|32 ns|39 ns|12.3 µs|42.1 µs|1.27 ms|
|3|456 ns|32 ns|39 ns|7.6 µs|42.3 µs|1.24 ms|

**Run set B: 10 µs interval, lateness measured before `sendto`, `sendto` timed separately**

_Lateness_

|Run|avg|p50|p95|p99|p99.9|max|
|---|---|---|---|---|---|---|
|1|1135 ns|32 ns|39 ns|15.4 µs|48.2 µs|2.67 ms|
|2|1333 ns|32 ns|39 ns|15.0 µs|212.2 µs|2.66 ms|
|3|1180 ns|32 ns|39 ns|15.1 µs|158.3 µs|2.57 ms|

_`sendto` duration_

|Run|avg|p50|p95|p99|p99.9|max|
|---|---|---|---|---|---|---|
|1|4600 ns|4478 ns|4627 ns|6227 ns|27.6 µs|489.2 µs|
|2|4617 ns|4487 ns|4764 ns|6167 ns|27.0 µs|580.9 µs|
|3|4597 ns|4472 ns|4690 ns|6161 ns|27.0 µs|456.2 µs|

**Run set C: 20 µs interval, same setup as B (each run takes 20 s)**

_Lateness_

|Run|avg|p50|p95|p99|p99.9|max|
|---|---|---|---|---|---|---|
|1|584 ns|31 ns|39 ns|5.7 µs|22.5 µs|2.82 ms|
|2|502 ns|32 ns|39 ns|60 ns|19.3 µs|3.29 ms|
|3|739 ns|32 ns|39 ns|12.6 µs|47.2 µs|2.94 ms|

_`sendto` duration_

| Run | avg     | p50     | p95     | p99     | p99.9   | max      |
| --- | ------- | ------- | ------- | ------- | ------- | -------- |
| 1   | 4578 ns | 4508 ns | 4710 ns | 5216 ns | 19.9 µs | 465.6 µs |
| 2   | 4701 ns | 4558 ns | 5389 ns | 6881 ns | 18.1 µs | 93.8 µs  |
| 3   | 4647 ns | 4530 ns | 4713 ns | 5315 ns | 29.5 µs | 469.7 µs |

||sendto p50|sendto p99|lateness p99|lateness p99.9|max|
|---|---|---|---|---|---|
|Desktop, 10 µs|~4.5 µs|~6.2 µs|~15 µs|48–212 µs|~2.6 ms|
|Server, 20 µs|~2.6 µs|~3.0 µs|37–40 ns|15–61 µs|5.4–6.6 ms|
|Server, 10 µs|~2.6 µs|~3.0 µs|37–40 ns|39 µs to 15.9 ms|5.5–16.9 ms|
**Findings**

1. Adding `sendto` moved lateness p99 from about 40 ns (no network) to 8–17 µs at a 10 µs interval. p50 and p95 didn’t change.
2. `sendto` on loopback costs about 4.5 µs at p50, independent of the send interval. That’s about 45% of a 10 µs interval.
3. `sendto`’s own p99 (about 6 µs) stays under the interval, so slow sends alone don’t explain lateness p99. The cause is catch-up: after a stall, the sender only recovers (interval − 4.5 µs) per packet, so every stall leaves a longer trail of late packets.
4. Doubling the interval to 20 µs halved average lateness and lowered p99, but p99 became noisy between runs (60 ns to 12.6 µs), because late packets now sit right around the 1% boundary.
5. Millisecond stalls (max 1.2–3.3 ms) are present in every configuration. That’s the root cause, and phase 2 will test it on the server.