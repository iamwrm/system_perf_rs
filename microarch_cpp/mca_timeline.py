#!/usr/bin/env python3
"""Render llvm-mca's modeled schedule as a cycle/resource CLI table.

The issue and completion cycles come from llvm-mca. Resource lane assignment in
this display is cosmetic: operations are grouped as load, multiply/FMA, ALU,
and branch, then placed into display lanes for the issue cycle.
"""

from __future__ import annotations

import argparse
import json
import shutil
import subprocess
import sys
from collections import defaultdict
from pathlib import Path

LANES = [
    ("LOAD-0", "load", 0),
    ("LOAD-1", "load", 1),
    ("MUL/FMA", "mul", 0),
    ("ALU-0", "alu", 0),
    ("ALU-1", "alu", 1),
    ("BRANCH", "branch", 0),
]
PREFIX = {"load": "L", "mul": "M", "alu": "A", "branch": "B"}
COLORS = {
    "load": "\033[44;97m",
    "mul": "\033[41;97m",
    "alu": "\033[43;30m",
    "branch": "\033[45;97m",
}
RESET = "\033[0m"


def find_llvm_mca(requested: str | None) -> str:
    candidates = [
        requested,
        shutil.which("llvm-mca"),
        "/opt/nanobrew/prefix/bin/llvm-mca",
        "/opt/homebrew/opt/llvm/bin/llvm-mca",
        "/usr/local/opt/llvm/bin/llvm-mca",
    ]
    for candidate in candidates:
        if candidate and Path(candidate).is_file():
            return str(candidate)
    raise SystemExit("llvm-mca was not found; pass --llvm-mca PATH")


def classify(instruction: str, info: dict) -> str:
    mnemonic = instruction.strip().split(None, 1)[0].lower()
    if info.get("mayLoad") or info.get("mayStore"):
        return "load"
    if mnemonic.startswith((
        "mul", "imul", "madd", "mla", "fmul", "fma", "vfm", "pmull",
    )):
        return "mul"
    if mnemonic.startswith((
        "b", "j", "cb", "tb", "ret", "call", "bl",
    )):
        return "branch"
    return "alu"


def centered(text: str, width: int) -> str:
    if len(text) > width:
        text = text[: max(1, width - 1)] + "…"
    return text.center(width)


def colored_cell(text: str, width: int, kind: str | None, color: bool) -> str:
    padded = centered(text, width)
    if color and kind:
        return COLORS[kind] + padded + RESET
    return padded


def run_mca(args) -> tuple[dict, str]:
    llvm_mca = find_llvm_mca(args.llvm_mca)
    command = [
        llvm_mca,
        f"-mtriple={args.triple}",
        f"-mcpu={args.cpu}",
        f"--iterations={args.iterations}",
        "--timeline",
        "--json",
        str(Path(args.input).resolve()),
    ]
    completed = subprocess.run(
        command,
        check=True,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    return json.loads(completed.stdout), llvm_mca


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("input", help="assembly accepted by llvm-mca")
    parser.add_argument("--triple", required=True)
    parser.add_argument("--cpu", required=True)
    parser.add_argument("--iterations", type=int, default=2)
    parser.add_argument("--max-cycles", type=int, default=40)
    parser.add_argument("--llvm-mca")
    parser.add_argument(
        "--color", choices=("auto", "always", "never"), default="auto"
    )
    args = parser.parse_args()
    if args.iterations < 1:
        parser.error("--iterations must be positive")
    if args.max_cycles < 0:
        parser.error("--max-cycles cannot be negative")

    document, llvm_mca = run_mca(args)
    region = document["CodeRegions"][0]
    summary = region["SummaryView"]
    instructions = region["Instructions"]
    info = region["InstructionInfoView"]["InstructionList"]
    timeline = region["TimelineView"]["TimelineInfo"]
    block_size = len(instructions)
    use_color = args.color == "always" or (
        args.color == "auto" and sys.stdout.isatty()
    )

    dynamic = []
    issued = defaultdict(lambda: defaultdict(list))
    completed = defaultdict(list)
    for dynamic_index, timing in enumerate(timeline):
        static_index = dynamic_index % block_size
        iteration = dynamic_index // block_size
        kind = classify(instructions[static_index], info[static_index])
        label = f"{PREFIX[kind]}{static_index}.{iteration}"
        operation = {
            "label": label,
            "kind": kind,
            "instruction": instructions[static_index],
            "static": static_index,
            "iteration": iteration,
            **timing,
        }
        dynamic.append(operation)
        issued[timing["CycleIssued"]][kind].append(operation)
        completed[timing["CycleExecuted"]].append(operation)

    max_observed = max(
        [summary["TotalCycles"] - 1]
        + [op["CycleExecuted"] for op in dynamic]
    )
    last_cycle = (max_observed if args.max_cycles == 0 else
                  min(max_observed, args.max_cycles - 1))
    width = 10

    print("Modeled execution timeline")
    print(f"target={args.cpu} ({args.triple}), model={llvm_mca}")
    print(
        f"iterations={summary['Iterations']}, instructions={summary['Instructions']}, "
        f"cycles={summary['TotalCycles']}, IPC={summary['IPC']:.2f}, "
        f"uOps/cycle={summary['uOpsPerCycle']:.2f}"
    )
    print("Issue slots by cycle. A centered dot is an unused display lane.\n")

    header = " Cycle │" + "│".join(centered(name, width) for name, _, _ in LANES)
    header += "│" + centered("COMPLETES", 14)
    print(header)
    print("───────┼" + "┼".join("─" * width for _ in LANES) + "┼" + "─" * 14)

    for cycle in range(last_cycle + 1):
        cells = []
        for _, kind, lane_index in LANES:
            operations = issued[cycle][kind]
            operation = operations[lane_index] if lane_index < len(operations) else None
            if operation is None:
                cells.append(colored_cell("·", width, None, use_color))
            else:
                label = operation["label"]
                lane_count = sum(1 for _, lane_kind, _ in LANES if lane_kind == kind)
                if lane_index == lane_count - 1 and len(operations) > lane_count:
                    label += f"+{len(operations) - lane_count}"
                cells.append(colored_cell(label, width, kind, use_color))
        done = ",".join(op["label"] for op in completed.get(cycle, [])) or "·"
        print(f" {cycle:5d} │" + "│".join(cells) + "│" + centered(done, 14))

    if max_observed > last_cycle:
        print(f" ... timeline clipped at cycle {last_cycle}; use --max-cycles 0 or a larger value")

    print("\nOperation timing")
    print(" op     D   ready   issue   done   retire   dep-wait   port-wait   instruction")
    print(" -----  --  ------  ------  -----  -------  --------  ----------  -----------")
    for op in dynamic:
        dep_wait = op["CycleReady"] - op["CycleDispatched"]
        port_wait = op["CycleIssued"] - op["CycleReady"]
        print(
            f" {op['label']:<5}  {op['CycleDispatched']:2d}"
            f"  {op['CycleReady']:6d}  {op['CycleIssued']:6d}"
            f"  {op['CycleExecuted']:5d}  {op['CycleRetired']:7d}"
            f"  {dep_wait:8d}  {port_wait:10d}  {op['instruction']}"
        )

    print("\nD=dispatch. ready=operands available. issue=sent to an execution unit.")
    print("done=result available. retire=architectural commit.")
    print("This is an llvm-mca model, not a per-cycle hardware trace.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
