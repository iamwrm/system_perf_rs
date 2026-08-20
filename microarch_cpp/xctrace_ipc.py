#!/usr/bin/env python3
"""Measure approximate loop IPC for pipeline_probe with macOS CPU Counters.

The measured loops contain exactly 18 architectural instructions per iteration:
16 MUL instructions, SUBS, and B.NE.  xctrace supplies hardware core-cycle
counts.  Fixed process startup/printing is included in cycles but not in the
known loop instruction count, so the reported value is a close lower bound on
IPC when the iteration count is large.
"""

from __future__ import annotations

import argparse
import shutil
import subprocess
import tempfile
import xml.etree.ElementTree as ET
from pathlib import Path

INSTRUCTIONS_PER_ITERATION = 18


def export_table(trace: Path, schema: str, output: Path) -> None:
    xpath = f'/trace-toc/run[@number="1"]/data/table[@schema="{schema}"]'
    subprocess.run(
        [
            "xctrace",
            "export",
            "--input",
            str(trace),
            "--xpath",
            xpath,
            "--output",
            str(output),
        ],
        check=True,
        stdout=subprocess.DEVNULL,
    )


def resolved_rows(xml_path: Path):
    root = ET.parse(xml_path).getroot()
    id_map = {e.attrib["id"]: e for e in root.iter() if "id" in e.attrib}

    def resolve(element):
        return id_map.get(element.attrib.get("ref"), element)

    for row in root.iter("row"):
        values = {}
        formats = {}
        for element in row:
            actual = resolve(element)
            values[element.tag] = (actual.text or "").strip()
            formats[element.tag] = actual.attrib.get("fmt", "")
        yield values, formats


def read_cycles(metrics_xml: Path) -> int:
    imprecise = []
    precise = []
    for values, _ in resolved_rows(metrics_xml):
        if values.get("string") != "cycle":
            continue
        cycles = int(values["uint64"])
        if values.get("boolean") == "0":
            imprecise.append(cycles)
        else:
            precise.append(cycles)
    # The two resolutions overlap and have the same total; never add both.
    samples = imprecise or precise
    if not samples:
        raise RuntimeError("No cycle metric found in xctrace export")
    return sum(samples)


def read_core_mix(counters_xml: Path) -> tuple[float, float]:
    durations = {"P": 0, "E": 0}
    for values, formats in resolved_rows(counters_xml):
        if "duration" not in values or "core" not in formats:
            continue
        core_format = formats["core"]
        kind = "P" if "(P Core)" in core_format else (
            "E" if "(E Core)" in core_format else None
        )
        if kind:
            durations[kind] += int(values["duration"])
    total = durations["P"] + durations["E"]
    if total == 0:
        return 0.0, 0.0
    return durations["P"] / total, durations["E"] / total


def measure(binary: Path, chains: int, iterations: int):
    work = Path(tempfile.mkdtemp(prefix=f"pipeline-ipc-{chains}-"))
    trace = work / "run.trace"
    metrics_xml = work / "metrics.xml"
    counters_xml = work / "counters.xml"
    try:
        command = [
            "xctrace",
            "record",
            "--template",
            "CPU Counters",
            "--output",
            str(trace),
            "--launch",
            "--",
            str(binary),
            "--worker",
            str(chains),
            str(iterations),
        ]
        completed = subprocess.run(
            command,
            check=True,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
        )
        worker_line = next(
            (line for line in completed.stdout.splitlines() if line.startswith("worker ")),
            "",
        )
        if worker_line:
            print(f"  {worker_line}")

        export_table(trace, "MetricAggregationForProcess", metrics_xml)
        export_table(trace, "CounterMetricByThread", counters_xml)
        cycles = read_cycles(metrics_xml)
        p_fraction, e_fraction = read_core_mix(counters_xml)
        loop_instructions = iterations * INSTRUCTIONS_PER_ITERATION
        ipc = loop_instructions / cycles
        return cycles, loop_instructions, ipc, p_fraction, e_fraction
    finally:
        shutil.rmtree(work, ignore_errors=True)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--binary", default="./pipeline_probe")
    parser.add_argument("--iterations", type=int, default=100_000_000)
    parser.add_argument("chains", type=int, nargs="*", default=[1, 8])
    args = parser.parse_args()

    binary = Path(args.binary).resolve()
    print("Recording macOS CPU Counters; this can take several seconds per case.")
    results = []
    for chains in args.chains:
        print(f"chains={chains}:")
        result = measure(binary, chains, args.iterations)
        results.append((chains, *result))

    print("\n chains       cycles   known loop instructions   approx IPC   core samples")
    print(" ------   ----------   -----------------------   ----------   ------------")
    for chains, cycles, instructions, ipc, p_fraction, e_fraction in results:
        print(
            f" {chains:6d}   {cycles:10d}   {instructions:23d}   "
            f"{ipc:10.3f}   P {p_fraction:4.0%} / E {e_fraction:4.0%}"
        )

    if len(results) >= 2:
        baseline = results[0][3]
        best = max(result[3] for result in results[1:])
        print(f"\nObserved IPC increase: {best / baseline:.2f}x")
    print("IPC is a lower-bound approximation because fixed startup cycles are included.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
