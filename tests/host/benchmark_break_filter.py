#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Optional host CPU benchmark, after run_break_filter.sh; not FM-1 profiling."""
import json
from pathlib import Path
import statistics
import subprocess

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "build/host/break-filter"
cases = [("baseline", "reference_test", 64), ("off", "dsp_test", 64),
         ("lp", "dsp_test", 0), ("hp", "dsp_test", 127)]
times = {name: [] for name, _, _ in cases}
for trial in range(9):
    order = cases[trial % len(cases):] + cases[:trial % len(cases)]
    for name, exe, pot in order:
        seconds = float(subprocess.check_output([str(OUT / exe), "bench", str(pot)], cwd=ROOT))
        times[name].append(seconds)
medians = {name: statistics.median(values) for name, values in times.items()}
result = {
    "frames_per_trial": 65536 * 256,
    "median_cpu_seconds": medians,
    "overhead_vs_baseline_percent": {name: (seconds / medians["baseline"] - 1) * 100
                                      for name, seconds in medians.items() if name != "baseline"},
    "trials": times,
    "note": "Host BREAK renderer CPU time only; no FM-1 cycle/whole-app load measurement",
}
(OUT / "benchmark.json").write_text(json.dumps(result, indent=2) + "\n")
print(json.dumps(result, indent=2))
