# Phase 2: order table, std::unordered_map -> RefMap

Date: 2026-11-17
Before: commit `b7b940f`
After: commit (this commit)
Machine / kernel: `Intel(R) Core(TM) i5-9400F CPU @ 2.90GHz`, `7.3.1-1-cachyos`, cmdline `quiet nowatchdog splash rw rootflags=subvol=/@ rd.luks.uuid=c094aedc-4566-4f74-8b2f-13b866f9e9d1 root=/dev/mapper/luks-c094aedc-4566-4f74-8b2f-13b866f9e9d1`
Build: Release (`make release`), GCC `g++ (GCC) 16.2.1 20260810`
Input: 20191230.BX_ITCH_50 (whole day, 29,156,757 messages)

## What changed

The book's order table (ref -> Order, one per stock locate) moved from
`std::unordered_map` to `RefMap` (include/netrx/refmap.hpp): open addressing,
linear probing, key 0 = empty slot, backward-shift erase, power-of-two capacity,
max load 1/2, identity hash, each book starting at 16 slots and doubling.

## Correctness (whole day, Debug, asserts + _GLIBCXX_ASSERTIONS on)

Output identical before and after:

```
S 6 R 8906 A 12210139 F 45058 E 578839 C 2686 X 348198 D 11821540 U 1741672 unknown 2399713
total 29156757 live 0 peak 76126 last event 'C'
```

Unit tests: `make test` 37/37 under ASan + UBSan (includes tests/refmap_test.cpp,
6 cases, and the 15 book cases now running on RefMap).

## Wall time (Release, same boot, alternating old/new, run 1 dropped as warm-up)

| run | before (unordered_map) | after (RefMap) |
|---|---|---|
| 2 | 3.02 s | 2.07 s |
| 3 | 3.02 s | 2.03 s |
| 4 | 3.03 s | 2.11 s |
| **median** | **3.02 s** | **2.07 s** |

0.95 s less per day, about 31% (1.46x faster).
Command: `time ./build-release/itchdump > /dev/null` (fish), 4 alternating pairs.

## Profile (perf, RelWithDebInfo, frame pointers; shares read by eye)

| part | before | after |
|---|---|---|
| order table | ~50% (std::_Hashtable) | ~25% (RefMap: Insert in Add ~13%, Erase in Delete ~10%) |
| decoder | ~15-20% | ~26% |
| flat_map price levels | ~10% | ~15% |

Decoder and flat_map code did not change; their share grew because the run got shorter.

- before: docs/profiles/phase2before.svg
- after: docs/profiles/phase2after.svg

## Not tuned yet

Max load (0.5 vs 0.75), hash (identity vs multiply + fold), starting capacity,
per-book vs one shared table. Next: Google Benchmark against std::unordered_map
and absl::flat_hash_map.
