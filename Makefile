.PHONY: all debug release profile run-debug run-release flamegraph test bench clean

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

bench:
	cmake --preset bench
	cmake --build --preset bench -j
	mkdir -p $(dir $(BENCH_OUT))
	taskset -c 2 ./build-bench/netrx_bench --benchmark_repetitions=5 \
	  --benchmark_report_aggregates_only=true \
	  --benchmark_out=$(BENCH_OUT) --benchmark_out_format=json

clean:
	rm -rf build-debug build-release build-prof build-bench
