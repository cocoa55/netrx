.PHONY: all debug release profile run-debug run-release flamegraph test bench bench-precheck clean

all: debug release

debug:
	cmake --preset debug
	cmake --build --preset debug -j

release:
	cmake --preset release
	cmake --build --preset release -j

run-debug: debug
	./build-debug/itchdump $(ARGS)

run-release: release
	./build-release/itchdump $(ARGS)

FLAME_OUT ?= docs/profiles/itchdump.svg

profile:
	cmake --preset profile
	cmake --build --preset profile -j

flamegraph: profile
	perf record -F 999 --call-graph fp -o itchdump.perf.data ./build-prof/itchdump $(ARGS) > /dev/null
	mkdir -p $(dir $(FLAME_OUT))
	perf script -i itchdump.perf.data | ./FlameGraph/stackcollapse-perf.pl | ./FlameGraph/flamegraph.pl > $(FLAME_OUT)
	@echo "wrote $(FLAME_OUT)"

test: debug
	ctest --preset debug

BENCH_OUT ?= results/bench/phase2-maps.json

BENCH_MACHINE = $(BENCH_OUT:.json=.machine.txt)

bench-precheck:
	@bash scripts/bench-precheck.sh

bench: bench-precheck
	@if [ -e $(BENCH_OUT) ] && [ -z "$(BENCH_FORCE)" ]; then \
	  echo "$(BENCH_OUT) exists; pick a new BENCH_OUT=... or set BENCH_FORCE=1" >&2; exit 1; fi
	cmake --preset bench
	cmake --build --preset bench -j
	mkdir -p $(dir $(BENCH_OUT))
	taskset -c 2 ./build-bench/netrx_bench --benchmark_repetitions=5 \
	  --benchmark_report_aggregates_only=true \
	  --benchmark_out=$(BENCH_OUT) --benchmark_out_format=json
	bash scripts/bench-machine.sh > $(BENCH_MACHINE)

clean:
	rm -rf build-debug build-release build-prof build-bench
