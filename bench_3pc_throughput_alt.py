#!/usr/bin/env python3

import argparse
import math
import statistics
import subprocess
import sys
import time
from typing import List, Tuple


DEFAULT_EXE = "bazel-bin/service/tools/kv/api_tools/kv_service_tools"
DEFAULT_CONFIG = "service/tools/config/interface/service.config"


def non_negative_float(value: str) -> float:
    parsed = float(value)
    if parsed < 0:
        raise argparse.ArgumentTypeError("value must be non-negative")
    return parsed


def positive_float(value: str) -> float:
    parsed = float(value)
    if parsed <= 0:
        raise argparse.ArgumentTypeError("value must be greater than 0")
    return parsed


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Run a time-based external-client benchmark against the 3PC KV path."
        )
    )
    parser.add_argument(
        "--duration-seconds",
        type=positive_float,
        default=100.0,
        help="Measured benchmark duration after warmup.",
    )
    parser.add_argument(
        "--warmup-seconds",
        type=non_negative_float,
        default=5.0,
        help="Warmup duration. Transactions during this window are excluded.",
    )
    parser.add_argument(
        "--config",
        default=DEFAULT_CONFIG,
        help="Client interface config path.",
    )
    parser.add_argument(
        "--exe",
        default=DEFAULT_EXE,
        help="kv_service_tools executable path.",
    )
    parser.add_argument(
        "--key-prefix",
        default="bench_alt",
        help="Prefix for generated benchmark keys.",
    )
    parser.add_argument(
        "--value-prefix",
        default="v",
        help="Prefix for generated benchmark values.",
    )
    return parser.parse_args()


def run_set(
    exe: str,
    config: str,
    key_prefix: str,
    value_prefix: str,
    txn_id: int,
) -> Tuple[bool, float, subprocess.CompletedProcess]:
    start = time.perf_counter()
    result = subprocess.run(
        [
            exe,
            "--config",
            config,
            "--cmd",
            "set",
            "--key",
            f"{key_prefix}{txn_id}",
            "--value",
            f"{value_prefix}{txn_id}",
        ],
        capture_output=True,
        text=True,
    )
    latency_ms = (time.perf_counter() - start) * 1000
    ok = (
        result.returncode == 0
        and "done" in result.stdout
        and "ret = 0" in result.stdout
    )
    return ok, latency_ms, result


def print_failure(
    phase: str,
    txn_id: int,
    failed_transactions: int,
    result: subprocess.CompletedProcess,
) -> None:
    print(
        f"FAIL during {phase} at txn_id={txn_id} code={result.returncode}",
        file=sys.stderr,
    )
    print(f"failed_transactions: {failed_transactions}", file=sys.stderr)
    print(result.stdout[:500], file=sys.stderr)
    print(result.stderr[:500], file=sys.stderr)


def fmt_float(value: float, digits: int = 4) -> str:
    if math.isnan(value):
        return "nan"
    return f"{value:.{digits}f}"


def main() -> int:
    args = parse_args()

    total_start = time.perf_counter()
    warmup_end = total_start + args.warmup_seconds

    txn_id = 0
    warmup_transactions = 0
    measured_transactions = 0
    failed_transactions = 0
    measured_latencies_ms: List[float] = []

    while time.perf_counter() < warmup_end:
        ok, _, result = run_set(
            args.exe,
            args.config,
            args.key_prefix,
            args.value_prefix,
            txn_id,
        )
        if not ok:
            failed_transactions += 1
            print_failure("warmup", txn_id, failed_transactions, result)
            return 1
        warmup_transactions += 1
        txn_id += 1

    measurement_start = time.perf_counter()
    measure_end = measurement_start + args.duration_seconds

    while time.perf_counter() < measure_end:
        ok, latency_ms, result = run_set(
            args.exe,
            args.config,
            args.key_prefix,
            args.value_prefix,
            txn_id,
        )
        if not ok:
            failed_transactions += 1
            print_failure("measurement", txn_id, failed_transactions, result)
            return 1
        measured_transactions += 1
        measured_latencies_ms.append(latency_ms)
        txn_id += 1

    measurement_seconds = time.perf_counter() - measurement_start
    throughput = (
        measured_transactions / measurement_seconds
        if measurement_seconds > 0
        else float("nan")
    )

    avg_latency = (
        statistics.mean(measured_latencies_ms)
        if measured_latencies_ms
        else float("nan")
    )
    median_latency = (
        statistics.median(measured_latencies_ms)
        if measured_latencies_ms
        else float("nan")
    )
    min_latency = min(measured_latencies_ms) if measured_latencies_ms else float("nan")
    max_latency = max(measured_latencies_ms) if measured_latencies_ms else float("nan")

    print(f"warmup_seconds: {args.warmup_seconds:.4f}")
    print(f"measurement_seconds: {measurement_seconds:.4f}")
    print(f"warmup_transactions: {warmup_transactions}")
    print(f"measured_transactions: {measured_transactions}")
    print(f"failed_transactions: {failed_transactions}")
    print(f"throughput_tps_measured: {fmt_float(throughput, 2)}")
    print(f"avg_latency_ms: {fmt_float(avg_latency, 3)}")
    print(f"median_latency_ms: {fmt_float(median_latency, 3)}")
    if len(measured_latencies_ms) > 1:
        print(
            f"stdev_latency_ms: "
            f"{fmt_float(statistics.stdev(measured_latencies_ms), 3)}"
        )
    print(f"min_latency_ms: {fmt_float(min_latency, 3)}")
    print(f"max_latency_ms: {fmt_float(max_latency, 3)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
