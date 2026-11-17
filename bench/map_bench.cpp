// Order-table map benchmark: replays the real ITCH day's insert/find/erase
// pattern (ref -> remaining shares) against std::unordered_map,
// absl::flat_hash_map and netrx::RefMap, per stock locate and shared.

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <format>
#include <print>
#include <string>
#include <unordered_map>
#include <vector>

#include <absl/container/flat_hash_map.h>
#include <benchmark/benchmark.h>

#include "netrx/refmap.hpp"
#include "trace.hpp"

namespace {

using netrx::bench::Kind;
using netrx::bench::Op;
using Key = std::uint64_t;
using Val = std::uint32_t;

constexpr std::size_t kLocates = 65'536;
constexpr std::size_t kExpectedPeak = 76'126;

std::vector<Op> gTrace; // built once in main(), before any timing

// ---- Adapters: one per map type, so Replay() is a single template --------

template <class M> struct StdAdapter {
  using Map = M;
  using It = typename M::iterator;
  static It Find(Map &m, Key k) { return m.find(k); }
  static bool Found(Map &m, It it) { return it != m.end(); }
  static Val &Value(It it) { return it->second; }
  static void Erase(Map &m, It it) { m.erase(it); }
  static void Insert(Map &m, Key k, Val v) { m.emplace(k, v); }
};

template <std::size_t Num, std::size_t Den> struct RefMapAdapter {
  using Map = netrx::RefMap<Key, Val, Num, Den>;
  using It = typename Map::iterator;
  static It Find(Map &m, Key k) { return m.Find(k); }
  static bool Found(Map &m, It it) { return it != m.end(); }
  static Val &Value(It it) { return it->second; }
  static void Erase(Map &m, It it) { m.Erase(it); }
  static void Insert(Map &m, Key k, Val v) { m.Insert(k, v); }
};

using UnorderedA = StdAdapter<std::unordered_map<Key, Val>>;
using AbslA = StdAdapter<absl::flat_hash_map<Key, Val>>;
using RefMapHalf = RefMapAdapter<1, 2>;
using RefMap34 = RefMapAdapter<3, 4>;

// ---- Replay ----------------------------------------------------------------

struct Result {
  std::size_t live{};
  std::size_t peak{};
  std::uint64_t missing{};   // finds that should have hit but didn't
  std::uint64_t underflow{}; // executed/cancelled more shares than remained
};

// MapFor(op) returns the map the op goes to (per-locate or the shared one).
template <class A, class MapFor>
Result Replay(const std::vector<Op> &trace, MapFor &&mapFor) {
  Result r;
  for (const Op &op : trace) {
    auto &m = mapFor(op);
    switch (static_cast<Kind>(op.kind)) {
    case Kind::Add:
      A::Insert(m, op.ref, op.shares);
      r.peak = std::max(r.peak, ++r.live);
      break;
    case Kind::Execute:
    case Kind::Cancel: {
      auto it = A::Find(m, op.ref);
      if (!A::Found(m, it)) [[unlikely]] {
        ++r.missing;
        break;
      }
      Val &left = A::Value(it);
      if (op.shares > left) [[unlikely]] {
        ++r.underflow;
        left = op.shares; // force to zero below so the entry is still erased
      }
      left -= op.shares;
      if (left == 0) {
        A::Erase(m, it);
        --r.live;
      }
      break;
    }
    case Kind::Delete: {
      auto it = A::Find(m, op.ref);
      if (!A::Found(m, it)) [[unlikely]] {
        ++r.missing;
        break;
      }
      A::Erase(m, it);
      --r.live;
      break;
    }
    case Kind::Replace: {
      auto it = A::Find(m, op.ref);
      if (!A::Found(m, it)) [[unlikely]] {
        ++r.missing;
        break;
      }
      A::Erase(m, it);
      A::Insert(m, op.newRef, op.shares);
      break;
    }
    }
  }
  return r;
}

void Report(benchmark::State &state, const Result &r) {
  state.SetItemsProcessed(static_cast<std::int64_t>(gTrace.size()));
  state.counters["peak"] = static_cast<double>(r.peak);
  state.counters["final_live"] = static_cast<double>(r.live);

  if (r.peak != kExpectedPeak || r.live != 0 || r.missing != 0 ||
      r.underflow != 0) {
    const auto msg = std::format(
        "check failed: peak={} (want {}) final_live={} (want 0) "
        "missing_finds={} share_underflows={}",
        r.peak, kExpectedPeak, r.live, r.missing, r.underflow);
    state.SkipWithError(msg);
  }
}

// ---- Group A: one map per stock locate (like the book) --------------------

template <class A, class Make> void PerLocate(benchmark::State &state, Make make) {
  Result r;
  for (auto _ : state) {
    std::vector<typename A::Map> maps;
    maps.reserve(kLocates);
    for (std::size_t i = 0; i < kLocates; ++i)
      maps.push_back(make());
    r = Replay<A>(gTrace, [&](const Op &op) -> auto & {
      return maps[op.locate];
    });
    benchmark::DoNotOptimize(maps);
    benchmark::ClobberMemory();
  }
  Report(state, r);
}

// ---- Group B: one shared map for all orders --------------------------------

template <class A, class Make> void Shared(benchmark::State &state, Make make) {
  Result r;
  for (auto _ : state) {
    typename A::Map map = make();
    r = Replay<A>(gTrace, [&](const Op &) -> auto & { return map; });
    benchmark::DoNotOptimize(map);
    benchmark::ClobberMemory();
  }
  Report(state, r);
}

template <class F> void Register(const std::string &name, F fn) {
  benchmark::RegisterBenchmark(name, fn)
      ->Iterations(1)
      ->Repetitions(5)
      ->Unit(benchmark::kMillisecond);
}

void RegisterAll() {
  // A. per stock locate
  Register("A1/per_locate/unordered_map", [](benchmark::State &s) {
    PerLocate<UnorderedA>(s, [] { return UnorderedA::Map{}; });
  });
  Register("A2/per_locate/absl_flat_hash_map", [](benchmark::State &s) {
    PerLocate<AbslA>(s, [] { return AbslA::Map{}; });
  });
  Register("A3/per_locate/RefMap_1_2_cap16", [](benchmark::State &s) {
    PerLocate<RefMapHalf>(s, [] { return RefMapHalf::Map{16}; });
  });
  Register("A4/per_locate/RefMap_3_4_cap16", [](benchmark::State &s) {
    PerLocate<RefMap34>(s, [] { return RefMap34::Map{16}; });
  });

  // B. one shared map
  Register("B5/shared/unordered_map_noreserve", [](benchmark::State &s) {
    Shared<UnorderedA>(s, [] { return UnorderedA::Map{}; });
  });
  Register("B6/shared/unordered_map_reserve262144", [](benchmark::State &s) {
    Shared<UnorderedA>(s, [] {
      UnorderedA::Map m;
      m.reserve(262'144);
      return m;
    });
  });
  Register("B7/shared/absl_flat_hash_map_reserve262144",
           [](benchmark::State &s) {
             Shared<AbslA>(s, [] {
               AbslA::Map m;
               m.reserve(262'144);
               return m;
             });
           });
  Register("B8/shared/RefMap_1_2_cap16", [](benchmark::State &s) {
    Shared<RefMapHalf>(s, [] { return RefMapHalf::Map{16}; });
  });
  Register("B9/shared/RefMap_1_2_cap262144", [](benchmark::State &s) {
    Shared<RefMapHalf>(s, [] { return RefMapHalf::Map{262'144}; });
  });
  Register("B10/shared/RefMap_3_4_cap131072", [](benchmark::State &s) {
    Shared<RefMap34>(s, [] { return RefMap34::Map{131'072}; });
  });
}

} // namespace

int main(int argc, char **argv) {
  benchmark::Initialize(&argc, argv);
  if (benchmark::ReportUnrecognizedArguments(argc, argv))
    return 1;

  gTrace = netrx::bench::BuildTrace(NETRX_ITCH_FILE);
  const auto bytes = static_cast<double>(gTrace.size() * sizeof(Op));
  std::println("trace: {} ops x {} B = {:.1f} MB ({:.1f} MiB), from {}",
               gTrace.size(), sizeof(Op), bytes / 1e6, bytes / (1024.0 * 1024.0),
               NETRX_ITCH_FILE);
  std::fflush(stdout);

  RegisterAll();
  benchmark::RunSpecifiedBenchmarks();
  benchmark::Shutdown();
  return 0;
}
