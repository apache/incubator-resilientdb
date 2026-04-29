#!/usr/bin/env python3
"""
Phase-aware result analyzer for adversarial network experiments.

Extends calculate_result.py with:
  - Time-series throughput/latency extraction (per 5s monitoring window)
  - Phase-based analysis (pre-adversarial / adversarial / recovery)
  - CSV output for plotting
  - Cross-protocol comparison support

Usage:
  python3 calculate_adversarial_result.py [--phases phases.json] [--csv output.csv] log_files...

phases.json format:
  {"phases": [
    {"name": "baseline", "start_s": 0, "end_s": 30},
    {"name": "adversarial", "start_s": 30, "end_s": 90},
    {"name": "recovery", "start_s": 90, "end_s": 150}
  ]}
"""

import argparse
import csv
import json
import re
import sys
from collections import defaultdict


def parse_monitor_blocks(filepath):
    """Parse a replica log file and extract per-monitor-window metrics.

    Each monitor block is printed every 5 seconds. We extract:
      - time (seconds since start)
      - txn count (throughput in that window, txn/s = txn_count / 5)
      - latency breakdowns
    """
    records = []
    with open(filepath) as f:
        block_lines = []
        for line in f:
            if "=========== monitor =========" in line:
                # Start of a new block - process previous
                if block_lines:
                    rec = _parse_block(block_lines)
                    if rec:
                        records.append(rec)
                block_lines = [line]
            elif "--------------- monitor ------------" in line:
                block_lines.append(line)
                # End of block
                rec = _parse_block(block_lines)
                if rec:
                    records.append(rec)
                block_lines = []
            elif block_lines:
                block_lines.append(line)
    # Process any trailing block
    if block_lines:
        rec = _parse_block(block_lines)
        if rec:
            records.append(rec)
    return records


def _parse_block(lines):
    """Parse a single monitor block into a dict of metrics."""
    text = " ".join(lines)
    rec = {}

    # Extract key:value pairs
    patterns = {
        "txn": r"txn:(\d+)",
        "time": r"time:(\d+)",
        "queuing_latency": r"queuing latency\s*:\s*([\d.eE+-]+|nan|inf)",
        "execute_prepare_latency": r"execute_prepare latency\s*:\s*([\d.eE+-]+|nan|inf)",
        "verify_latency": r"verify latency\s*:\s*([\d.eE+-]+|nan|inf)",
        "commit_latency": r"commit latency\s*:\s*([\d.eE+-]+|nan|inf)",
        "commit_running_latency": r"commit_running latency\s*:\s*([\d.eE+-]+|nan|inf)",
        "execute_queuing_latency": r"execute_queuing latency\s*:\s*([\d.eE+-]+|nan|inf)",
        "execute_latency": r"execute latency\s*:\s*([\d.eE+-]+|nan|inf)",
        "commit_round_latency": r"commit_round latency\s*:\s*([\d.eE+-]+|nan|inf)",
        "commit_interval_latency": r"commit_interval latency\s*:\s*([\d.eE+-]+|nan|inf)",
        "commit_txn": r"commit_txn latency\s*:\s*([\d.eE+-]+|nan|inf)",
        "commit_block": r"commit_block latency\s*:\s*([\d.eE+-]+|nan|inf)",
    }

    for key, pat in patterns.items():
        m = re.search(pat, text)
        if m:
            try:
                val = float(m.group(1))
                if val != float('inf') and val != float('-inf') and val == val:  # not nan
                    rec[key] = val
            except ValueError:
                pass

    # txn and time should be integers
    for k in ["txn", "time"]:
        if k in rec:
            rec[k] = int(rec[k])

    return rec if "txn" in rec else None


def aggregate_timeseries(all_records):
    """Aggregate records from multiple replicas into a time-series.

    Group by time bucket and sum/average metrics.
    """
    by_time = defaultdict(list)
    for records in all_records:
        for rec in records:
            t = rec.get("time", 0)
            by_time[t].append(rec)

    timeseries = []
    for t in sorted(by_time.keys()):
        recs = by_time[t]
        entry = {"time_s": t}
        # TPS: use max per-replica txn (all replicas commit the same blocks,
        # so summing would double-count). Filter out replicas with txn=0
        # (crashed replicas or clients).
        txn_values = [r.get("txn", 0) for r in recs if r.get("txn", 0) > 0]
        entry["tps"] = max(txn_values) if txn_values else 0
        # Latencies: average across replicas
        latency_keys = [k for k in recs[0].keys() if k.endswith("_latency") or k.startswith("commit_")]
        for key in latency_keys:
            vals = [r[key] for r in recs if key in r and r[key] > 0]
            if vals:
                entry[key] = sum(vals) / len(vals)
        timeseries.append(entry)
    return timeseries


def analyze_phases(timeseries, phases):
    """Analyze metrics within each phase."""
    results = {}
    for phase in phases:
        name = phase["name"]
        start = phase["start_s"]
        end = phase["end_s"]
        phase_data = [e for e in timeseries if start <= e["time_s"] < end]

        if not phase_data:
            results[name] = {"tps_avg": 0, "tps_max": 0, "count": 0}
            continue

        tps_values = [e["tps"] for e in phase_data]
        result = {
            "count": len(phase_data),
            "tps_avg": sum(tps_values) / len(tps_values) if tps_values else 0,
            "tps_max": max(tps_values) if tps_values else 0,
            "tps_min": min(tps_values) if tps_values else 0,
        }

        # Latency averages per phase
        for key in ["queuing_latency", "execute_prepare_latency", "commit_latency",
                     "verify_latency", "commit_round_latency", "commit_running_latency"]:
            vals = [e[key] for e in phase_data if key in e and e[key] > 0]
            if vals:
                result[f"{key}_avg"] = sum(vals) / len(vals)

        results[name] = result
    return results


def write_csv(timeseries, filepath):
    """Write time-series data to CSV for plotting."""
    if not timeseries:
        return
    keys = sorted(timeseries[0].keys())
    with open(filepath, "w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=keys)
        writer.writeheader()
        for entry in timeseries:
            writer.writerow({k: entry.get(k, "") for k in keys})


def main():
    parser = argparse.ArgumentParser(description="Phase-aware adversarial result analyzer")
    parser.add_argument("logs", nargs="+", help="Replica log files")
    parser.add_argument("--phases", help="Phase definition JSON file")
    parser.add_argument("--csv", help="Output CSV file for time-series data")
    args = parser.parse_args()

    print(f"Analyzing {len(args.logs)} log files")

    # Parse all logs
    all_records = []
    for logfile in args.logs:
        records = parse_monitor_blocks(logfile)
        all_records.append(records)
        print(f"  {logfile}: {len(records)} monitor windows")

    # Aggregate time-series
    timeseries = aggregate_timeseries(all_records)
    print(f"\nTime-series: {len(timeseries)} time points")

    if timeseries:
        tps_values = [e["tps"] for e in timeseries]
        if tps_values:
            print(f"  Overall TPS: max={max(tps_values)}, avg={sum(tps_values)/len(tps_values):.0f}")

    # Phase analysis
    if args.phases:
        with open(args.phases) as f:
            phase_def = json.load(f)
        phases = phase_def.get("phases", [])
        phase_results = analyze_phases(timeseries, phases)
        print(f"\n{'='*60}")
        print("Phase Analysis:")
        print(f"{'='*60}")
        for name, result in phase_results.items():
            print(f"\n  Phase: {name}")
            print(f"    Data points: {result['count']}")
            print(f"    TPS: avg={result['tps_avg']:.0f}, max={result['tps_max']:.0f}, min={result.get('tps_min', 0):.0f}")
            for key, val in sorted(result.items()):
                if key.endswith("_avg") and "latency" in key:
                    print(f"    {key}: {val:.6f}")

        # Print comparison summary
        if len(phase_results) >= 2:
            names = list(phase_results.keys())
            baseline = phase_results.get("baseline", phase_results.get(names[0], {}))
            if baseline.get("tps_avg", 0) > 0:
                print(f"\n{'='*60}")
                print("Degradation Ratios (vs baseline):")
                print(f"{'='*60}")
                for name, result in phase_results.items():
                    if name == "baseline" or name == names[0]:
                        continue
                    ratio = result["tps_avg"] / baseline["tps_avg"] if baseline["tps_avg"] > 0 else 0
                    print(f"  {name}: TPS ratio = {ratio:.3f} ({result['tps_avg']:.0f} / {baseline['tps_avg']:.0f})")

    # CSV output
    if args.csv:
        write_csv(timeseries, args.csv)
        print(f"\nTime-series CSV written to: {args.csv}")

    # Also run the original analysis for compatibility
    print(f"\n{'='*60}")
    print("Standard Analysis (compatible with calculate_result.py):")
    print(f"{'='*60}")
    all_tps = []
    all_lat = []
    for logfile in args.logs:
        tps, lat = _read_tps_compat(logfile)
        all_tps += tps
        all_lat += lat

    tps_valid = [v for v in all_tps if v >= 0]
    if tps_valid:
        print(f"max throughput:{max(tps_valid)} average throughput:{sum(tps_valid)/len(tps_valid):.0f} "
              f"replica num:{len(args.logs)}")
    if all_lat:
        lat_valid = [v for v in all_lat if v > 0]
        if lat_valid:
            print(f"max latency:{max(lat_valid)} average latency:{sum(lat_valid)/len(lat_valid):.6f}")


def _read_tps_compat(filepath):
    """Compatible TPS reader matching calculate_result.py."""
    tps = []
    lat = []
    with open(filepath) as f:
        for line in f:
            parts = line.split()
            for r in parts:
                try:
                    if r.split(':')[0] == 'txn':
                        tps.append(int(r.split(':')[1]))
                except (IndexError, ValueError):
                    pass
            if "client latency" in line:
                try:
                    lat.append(float(parts[-1].split(':')[-1]))
                except (IndexError, ValueError):
                    pass
    return tps, lat


if __name__ == "__main__":
    main()
