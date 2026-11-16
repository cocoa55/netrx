.PHONY: all debug release profile run-debug run-release flamegraph test clean

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

clean:
	rm -rf build-debug build-release build-prof
