#!/usr/bin/env python3
"""Plot netrx results.

Reads the raw int64 nanosecond samples that the sender and receiver write
(little-endian, in packet order) and produces:

  lateness_timeline.png    sender lateness for every packet over the run
  latency_<mode>.png       end-to-end vs pipeline latency histogram, per mode

It also prints percentiles and how the late packets cluster in time.

Usage:
  python3 scripts/plot.py [results_dir] [output_dir]
Defaults: results_dir = results, output_dir = results/plots
"""

import sys
from pathlib import Path

import matplotlib

matplotlib.use("Agg")  # write files, no window needed
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.ticker import FuncFormatter, LogLocator, NullFormatter

INTERVAL_NS = 10_000  # must match netrx::INTERVAL
LATE_NS = 5_000  # a packet counts as "late" above this, for the cluster report
BURST_GAP_NS = 1_000_000  # late packets closer than this belong to one burst

BLUE = "#2a78d6"  # series 1
ORANGE = "#eb6834"  # series 2
INK = "#3d3d3a"
MUTED = "#8a8a85"
GRID = "#e6e5e0"

MODES = {"Sleep": "sleep mode", "Poll": "busy-poll mode"}


def load(path: Path) -> np.ndarray:
    return np.fromfile(path, dtype="<i8")


def pct(samples: np.ndarray) -> dict:
    qs = {"p50": 50, "p99": 99, "p99.9": 99.9}
    out = {k: float(np.percentile(samples, q)) for k, q in qs.items()}
    out["max"] = float(samples.max())
    out["n"] = len(samples)
    return out


def fmt_ns(ns: float) -> str:
    if ns >= 1_000_000:
        return f"{ns / 1e6:.2f} ms"
    if ns >= 1_000:
        return f"{ns / 1e3:.2f} us"
    return f"{ns:.0f} ns"


def style(ax, title: str, xlabel: str, ylabel: str) -> None:
    ax.set_title(title, loc="left", color=INK, fontsize=12, pad=10)
    ax.set_xlabel(xlabel, color=MUTED)
    ax.set_ylabel(ylabel, color=MUTED)
    ax.grid(True, which="major", color=GRID, linewidth=0.8)
    ax.set_axisbelow(True)
    for side in ("top", "right"):
        ax.spines[side].set_visible(False)
    for side in ("left", "bottom"):
        ax.spines[side].set_color(GRID)
    ax.tick_params(colors=MUTED, which="both")


def plain_log_axis(axis) -> None:
    """Label a log axis 1, 2, 5, 10, 20, 50... instead of 2x10^1."""
    axis.set_major_locator(LogLocator(base=10, subs=(1.0, 2.0, 5.0)))
    axis.set_major_formatter(FuncFormatter(lambda v, _: f"{v:g}"))
    axis.set_minor_formatter(NullFormatter())


def lateness_timeline(lateness: np.ndarray, out: Path) -> None:
    t_s = np.arange(len(lateness)) * INTERVAL_NS / 1e9
    shown = lateness >= 1_000  # the ~30 ns bulk would bury everything else
    fig, ax = plt.subplots(figsize=(10, 4), dpi=150)
    ax.scatter(t_s[shown], lateness[shown] / 1e3, s=4, color=BLUE, linewidths=0,
               rasterized=True)
    ax.set_yscale("log")
    plain_log_axis(ax.yaxis)
    style(ax,
          f"Sender lateness over the run ({shown.sum():,} of {len(lateness):,} "
          f"packets at 1 us or more; the rest are under 1 us)",
          "time since first packet (s)", "lateness (us, log scale)")
    fig.tight_layout()
    fig.savefig(out)
    plt.close(fig)


def cluster_report(lateness: np.ndarray) -> None:
    late_idx = np.flatnonzero(lateness > LATE_NS)
    print(f"\nLate packets (> {fmt_ns(LATE_NS)}): {len(late_idx):,}")
    if len(late_idx) == 0:
        return
    times = late_idx * INTERVAL_NS
    starts = np.r_[0, np.flatnonzero(np.diff(times) > BURST_GAP_NS) + 1]
    ends = np.r_[starts[1:], len(times)]
    print(f"They form {len(starts)} bursts (late packets less than "
          f"{fmt_ns(BURST_GAP_NS)} apart are grouped):")
    for s, e in zip(starts, ends):
        worst = lateness[late_idx[s:e]].max()
        print(f"  at {times[s] / 1e9:7.3f} s: {e - s:5d} late packets, "
              f"worst {fmt_ns(worst)}")
    if len(starts) > 2:
        spacing = np.diff(times[starts]) / 1e6
        print(f"Spacing between bursts: median {np.median(spacing):.1f} ms, "
              f"min {spacing.min():.1f} ms, max {spacing.max():.1f} ms")


def latency_hist(e2e: np.ndarray, pipe: np.ndarray, mode: str, out: Path) -> None:
    e2e, pipe = e2e / 1e3, pipe / 1e3  # plot in microseconds
    lo = max(min(e2e.min(), pipe.min()), 0.1)
    hi = max(e2e.max(), pipe.max())
    bins = np.geomspace(lo, hi, 200)
    fig, ax = plt.subplots(figsize=(10, 4), dpi=150)
    for data, color, name in ((e2e, BLUE, "end-to-end (from intended send)"),
                              (pipe, ORANGE, "pipeline (from actual send)")):
        p = pct(data * 1e3)
        label = (f"{name}: p50 {fmt_ns(p['p50'])}, p99 {fmt_ns(p['p99'])}, "
                 f"p99.9 {fmt_ns(p['p99.9'])}")
        ax.hist(data, bins=bins, histtype="step", linewidth=2, color=color,
                label=label)
    ax.set_xscale("log")
    ax.set_yscale("log")
    plain_log_axis(ax.xaxis)
    ax.yaxis.set_major_formatter(FuncFormatter(lambda v, _: f"{v:,.0f}"))
    style(ax, f"Receive latency, {MODES[mode]} ({len(e2e):,} packets)",
          "latency (us, log scale)", "packets per bin (log scale)")
    ax.legend(frameon=False, loc="upper right", fontsize=9, labelcolor=INK)
    fig.tight_layout()
    fig.savefig(out)
    plt.close(fig)


def main() -> int:
    results = Path(sys.argv[1]) if len(sys.argv) > 1 else Path("results")
    out_dir = Path(sys.argv[2]) if len(sys.argv) > 2 else results / "plots"
    out_dir.mkdir(parents=True, exist_ok=True)
    made = []

    lat_path = results / "lateness.bin"
    if lat_path.exists():
        lateness = load(lat_path)
        p = pct(lateness)
        print(f"lateness: n={p['n']:,} p50 {fmt_ns(p['p50'])} p99 {fmt_ns(p['p99'])}"
              f" p99.9 {fmt_ns(p['p99.9'])} max {fmt_ns(p['max'])}")
        lateness_timeline(lateness, out_dir / "lateness_timeline.png")
        made.append("lateness_timeline.png")
        cluster_report(lateness)

    for mode in MODES:
        e2e_path = results / f"latency{mode}.bin"
        pipe_path = results / f"pipeline{mode}.bin"
        if e2e_path.exists() and pipe_path.exists():
            name = f"latency_{mode.lower()}.png"
            latency_hist(load(e2e_path), load(pipe_path), mode, out_dir / name)
            made.append(name)

    if not made:
        print(f"no .bin files found in {results}")
        return 1
    print(f"\nwrote {', '.join(made)} to {out_dir}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
