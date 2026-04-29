#!/usr/bin/env python3
"""
Unified phase-aware analyzer for normal / crash_f / gst_ramp experiments.

Computes cluster-level steady-state TPS using the raw replica logs (not
the misleading single-replica "max throughput" from calculate_result.py).

Key insight: `time:N` in each monitor block is "seconds since replica
start" (not seconds since experiment start). Deploy+client registration
takes an arbitrary amount of time before clients actually send load, so
we auto-detect t_start = first time bucket where cluster TPS > threshold,
and anchor phase windows to that offset.

Usage:
  python3 phase_analyze.py <result_dir> [--scenario normal|crash_f|gst_ramp]

For each replica log, we compute per-replica TPS = txn_in_window / 5.
Cluster TPS at time t = mean(per-replica TPS at t) — NOT sum, because
every replica commits the same final log under BFT, so summing double-
counts by a factor of N.
"""

import argparse
import glob
import os
import re
import sys
from collections import defaultdict


MONITOR_WINDOW_S = 5
TPS_THRESHOLD = 1000  # ignore windows below this (noise)


def parse_monitor_blocks(filepath):
    """Extract (time, txn) pairs from a replica log.

    Replica logs contain binary cert blobs which aren't valid UTF-8, so
    read with errors='replace' to skip undecodable bytes.
    """
    records = []
    try:
        with open(filepath, encoding="utf-8", errors="replace") as f:
            for line in f:
                # Each monitor line looks like:
                #   server call:... txn:NNN ... time:TT ...
                if "txn:" not in line or "time:" not in line:
                    continue
                m_txn = re.search(r"\btxn:(\d+)", line)
                m_time = re.search(r"\btime:(\d+)", line)
                if m_txn and m_time:
                    records.append((int(m_time.group(1)), int(m_txn.group(1))))
    except (IOError, OSError) as e:
        print(f"WARN: cannot read {filepath}: {e}", file=sys.stderr)
    return records


def aggregate_cluster_tps(replica_records):
    """Aggregate per-replica records into system throughput over time.

    In BFT all honest replicas commit the SAME log, so the system
    throughput equals any single alive replica's commit rate.  We take
    the mean of replicas whose txn > 0 in each window (filtering out
    dead replicas AND client log files which report txn=0).
    """
    by_time = defaultdict(list)
    for records in replica_records:
        for t, txn in records:
            by_time[t].append(txn)

    timeseries = []
    for t in sorted(by_time.keys()):
        samples = by_time[t]
        if not samples:
            continue
        # Filter to producers only (txn > 0). Client log files and
        # dead replicas report txn=0 and must be excluded — otherwise
        # the mean is diluted by a factor of (total_files / alive_replicas).
        producers = [s for s in samples if s > 0]
        if not producers:
            timeseries.append((t, 0))
            continue
        cluster_tps = int(sum(producers) / len(producers) / MONITOR_WINDOW_S)
        timeseries.append((t, cluster_tps))
    return timeseries


def find_experiment_start(timeseries, threshold=TPS_THRESHOLD):
    """First time bucket where cluster TPS exceeds threshold."""
    for t, tps in timeseries:
        if tps >= threshold:
            return t
    return None


def phase_stats(timeseries, t_lo, t_hi):
    """Compute avg/max/min TPS in [t_lo, t_hi) window."""
    vals = [tps for t, tps in timeseries if t_lo <= t < t_hi]
    if not vals:
        return {"avg": 0, "max": 0, "min": 0, "n": 0, "data": []}
    return {
        "avg": int(sum(vals) / len(vals)),
        "max": max(vals),
        "min": min(vals),
        "n": len(vals),
        "data": [(t, tps) for t, tps in timeseries if t_lo <= t < t_hi],
    }


def analyze_normal(timeseries):
    """Normal phase: skip first 5s ramp-up, measure steady-state."""
    t_start = find_experiment_start(timeseries)
    if t_start is None:
        return None
    # Use [t_start+5, t_start+55) = 50s steady state window, skipping
    # the first monitor window (ramp-up) and leaving room at the end.
    steady = phase_stats(timeseries, t_start + 5, t_start + 55)
    early = phase_stats(timeseries, t_start, t_start + 5)
    return {
        "scenario": "normal",
        "t_start": t_start,
        "steady": steady,
        "early_ramp": early,
    }


def analyze_crash_f(timeseries, crash_time_s=30):
    """Crash_f phase: baseline before crash, post-crash after transient.

    The current scenario crashes f nodes at T=crash_time_s (default 30s).
    Phases:
      baseline  = [t_start+5, t_start+crash_time_s) — skip 5s ramp-up
      transient = [t_start+crash_time_s, t_start+crash_time_s+10) — crash + view change
      post_crash = [t_start+crash_time_s+10, t_start+crash_time_s+60) — steady state
    """
    t_start = find_experiment_start(timeseries)
    if t_start is None:
        return None
    baseline = phase_stats(timeseries, t_start + 5, t_start + crash_time_s)
    transient = phase_stats(timeseries, t_start + crash_time_s,
                            t_start + crash_time_s + 10)
    post_crash = phase_stats(timeseries, t_start + crash_time_s + 10,
                             t_start + crash_time_s + 60)
    return {
        "scenario": "crash_f",
        "t_start": t_start,
        "crash_time": crash_time_s,
        "baseline": baseline,
        "transient": transient,
        "post_crash": post_crash,
    }


def analyze_gst_ramp(timeseries):
    """GST ramp phase: baseline, high-delay, recovery."""
    t_start = find_experiment_start(timeseries)
    if t_start is None:
        return None
    # Scenario ramps delay 40→200→500→1000→2000→40 over 60s
    # baseline: t_start..t_start+10 (40ms delay)
    # ramp_up: t_start+10..t_start+40 (increasing delay)
    # peak: t_start+40..t_start+50 (2000ms delay)
    # recovery: t_start+50..t_start+60 (back to 40ms)
    baseline = phase_stats(timeseries, t_start, t_start + 10)
    ramp = phase_stats(timeseries, t_start + 10, t_start + 40)
    peak = phase_stats(timeseries, t_start + 40, t_start + 50)
    recovery = phase_stats(timeseries, t_start + 50, t_start + 60)
    return {
        "scenario": "gst_ramp",
        "t_start": t_start,
        "baseline": baseline,
        "ramp": ramp,
        "peak": peak,
        "recovery": recovery,
    }


def analyze_dir(result_dir, scenario):
    """Analyze all replica logs in a directory.

    The result dir contains logs from BOTH replicas and clients (scp
    copies every node's log regardless of role). Client logs have
    txn:0 everywhere, so they'd drag the cluster-mean down. We filter
    to only files whose peak txn exceeds a small threshold — i.e.,
    the nodes that actually committed transactions.
    """
    pattern = os.path.join(result_dir, "result_*_log")
    log_files = sorted(glob.glob(pattern))
    if not log_files:
        return None

    all_records = [parse_monitor_blocks(f) for f in log_files]
    # Filter: keep only files where at least one monitor window
    # reported a nonzero txn count (these are the replicas that
    # actually committed). Clients always have txn:0.
    replica_records = [
        recs for recs in all_records
        if any(txn >= TPS_THRESHOLD for _, txn in recs)
    ]
    if not replica_records:
        return None
    timeseries = aggregate_cluster_tps(replica_records)

    n_producers = len(replica_records)
    if scenario == "normal":
        return analyze_normal(timeseries), timeseries, n_producers
    elif scenario == "crash_f":
        return analyze_crash_f(timeseries), timeseries, n_producers
    elif scenario == "gst_ramp":
        return analyze_gst_ramp(timeseries), timeseries, n_producers
    else:
        raise ValueError(f"Unknown scenario: {scenario}")


def format_phase(name, phase):
    if not phase or phase["n"] == 0:
        return f"  {name:<14}: NO DATA"
    return (f"  {name:<14}: avg={phase['avg']:>12,} TPS  "
            f"max={phase['max']:>12,}  min={phase['min']:>12,}  "
            f"n={phase['n']}")


def main():
    parser = argparse.ArgumentParser(description="Phase-aware analyzer")
    parser.add_argument("result_dir", help="Directory with result_*_log files")
    parser.add_argument("--scenario", choices=["normal", "crash_f", "gst_ramp"],
                        default="crash_f", help="Scenario type")
    parser.add_argument("--show-data", action="store_true",
                        help="Print per-window data points")
    args = parser.parse_args()

    result = analyze_dir(args.result_dir, args.scenario)
    if result is None:
        print(f"ERROR: no result_*_log files in {args.result_dir}", file=sys.stderr)
        sys.exit(1)

    analysis, timeseries, n_replicas = result
    if analysis is None:
        print(f"ERROR: no activity detected in {args.result_dir}", file=sys.stderr)
        sys.exit(1)

    print(f"Directory: {args.result_dir}")
    print(f"Replicas:  {n_replicas}")
    print(f"Scenario:  {analysis['scenario']}")
    print(f"t_start:   {analysis['t_start']}")
    print()

    if args.scenario == "normal":
        print(format_phase("early_ramp", analysis["early_ramp"]))
        print(format_phase("steady", analysis["steady"]))
    elif args.scenario == "crash_f":
        print(format_phase("baseline", analysis["baseline"]))
        print(format_phase("transient", analysis["transient"]))
        print(format_phase("post_crash", analysis["post_crash"]))
        if analysis["baseline"]["avg"] > 0:
            ratio = analysis["post_crash"]["avg"] / analysis["baseline"]["avg"]
            print(f"  recovery ratio (post_crash/baseline): {ratio:>6.2%}")
    elif args.scenario == "gst_ramp":
        print(format_phase("baseline", analysis["baseline"]))
        print(format_phase("ramp_up", analysis["ramp"]))
        print(format_phase("peak_delay", analysis["peak"]))
        print(format_phase("recovery", analysis["recovery"]))

    if args.show_data:
        print()
        print("Per-window cluster TPS:")
        for t, tps in timeseries:
            marker = ""
            if analysis["t_start"] is not None:
                delta = t - analysis["t_start"]
                marker = f"  (Δ={delta:+3d}s)"
            print(f"  t={t:>4}{marker}  tps={tps:>12,}")


if __name__ == "__main__":
    main()
