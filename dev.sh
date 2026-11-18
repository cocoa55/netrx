#!/usr/bin/env bash
# ./dev.sh debug | release | test
#          profile <target> [args] | flame <target> [args] | compare <target> [args]
#          bench [benchmark args] | bench-compare [benchmark args]
#
# Presets and build dirs (CMakePresets.json):
#   debug   -> build-debug    ASan + UBSan + clang-tidy, runs the tests
#   release -> build-release  -O3
#   profile -> build-prof     -O3 -g, frame pointers
#   bench   -> build-bench    Google Benchmark, target netrx_bench
set -euo pipefail

usage() { echo "usage: ./dev.sh debug|release|test|profile|flame|compare|bench|bench-compare [target] [args...]" >&2; exit 1; }
[[ $# -ge 1 ]] || usage
mode="$1"; shift

fg="${FLAMEGRAPH_DIR:-$PWD/FlameGraph}"
out="build-compare"; base="$out/base-src"
pin=(); [[ -n "${COMPARE_CPU:-}" ]] && pin=(taskset -c "$COMPARE_CPU")

build() { cmake --preset "$1" && cmake --build --preset "$1" -j; }
need_target() { [[ $# -ge 1 ]] || { echo "need a target name" >&2; exit 1; }; }

# check out the pushed version into a worktree and build both sides
setup_base() {  # preset
  git fetch --quiet
  ref="${COMPARE_REF:-$(git rev-parse --abbrev-ref --symbolic-full-name '@{upstream}')}"
  mkdir -p "$out"
  if [[ -d "$base" ]]; then git -C "$base" checkout --quiet --detach "$ref"
  else git worktree add --quiet --detach "$base" "$ref"; fi
  build "$1"
  (cd "$base" && build "$1")
}

case "$mode" in
  debug|release) build "$mode" ;;
  test) build debug && ctest --preset debug ;;
  profile)
    need_target "$@"; target="$1"; shift
    build profile
    perf stat -d -- "build-prof/$target" "$@"
    perf record --call-graph fp -o build-prof/perf.data -- "build-prof/$target" "$@"
    perf report -i build-prof/perf.data ;;
  flame)
    need_target "$@"; target="$1"; shift
    build profile
    perf record -F 999 --call-graph fp -o build-prof/perf.data -- "build-prof/$target" "$@" > /dev/null
    perf script -i build-prof/perf.data | "$fg/stackcollapse-perf.pl" | "$fg/flamegraph.pl" > build-prof/flame.svg
    echo "wrote build-prof/flame.svg" ;;
  compare)
    # whole program: working tree (uncommitted changes included) vs the pushed version
    need_target "$@"; target="$1"; shift
    setup_base profile
    run_side() {  # name binary args...
      local name="$1" bin="$2"; shift 2
      echo "== $name =="
      "${pin[@]}" perf stat -r "${COMPARE_RUNS:-10}" -- "$bin" "$@" > /dev/null
      "${pin[@]}" perf record -q -F 999 --call-graph fp -o "$out/$name.data" -- "$bin" "$@" > /dev/null
      perf script -i "$out/$name.data" | "$fg/stackcollapse-perf.pl" > "$out/$name.folded"
    }
    run_side base "$base/build-prof/$target" "$@"
    run_side new "build-prof/$target" "$@"
    "$fg/difffolded.pl" -n "$out/base.folded" "$out/new.folded" \
      | "$fg/flamegraph.pl" --title "$ref -> working tree (red = more time)" > "$out/diff.svg"
    "$fg/difffolded.pl" -n "$out/new.folded" "$out/base.folded" \
      | "$fg/flamegraph.pl" --negate --title "working tree -> $ref (blue = removed)" > "$out/diff-removed.svg"
    echo "compared against: $ref"
    echo "wrote $out/diff.svg and $out/diff-removed.svg" ;;
  bench)
    # e.g. ./dev.sh bench --benchmark_filter=BM_RefMap
    build bench
    "${pin[@]}" build-bench/netrx_bench "$@" ;;
  bench-compare)
    # per benchmark: working tree vs the pushed version, with a significance test
    setup_base bench
    flags=(--benchmark_repetitions="${COMPARE_RUNS:-10}"
           --benchmark_enable_random_interleaving=true
           --benchmark_out_format=json)
    echo "running base ($ref)..."
    "${pin[@]}" "$base/build-bench/netrx_bench" "${flags[@]}" --benchmark_out="$out/base.json" "$@" > /dev/null
    echo "running working tree..."
    "${pin[@]}" build-bench/netrx_bench "${flags[@]}" --benchmark_out="$out/new.json" "$@" > /dev/null
    python3 build-bench/_deps/benchmark-src/tools/compare.py benchmarks "$out/base.json" "$out/new.json" ;;
  *) usage ;;
esac
