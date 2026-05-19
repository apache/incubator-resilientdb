#!/usr/bin/env python3
# =============================================================================
# analyze.py
# ResilientDB Performance Analyzer
#
# Called automatically by perf_test.sh.
# Parses raw curl timing data, computes metrics, saves result to MongoDB,
# and prints final JSON. Analysis against a chosen time-period baseline is
# generated on demand via the UI (see backend/analyze_record.py).
#
# For a full explanation of how curl timing values are converted into
# latency, throughput, percentiles, and server-side wait time, see:
#
#   docs/metrics_explanation.md
# =============================================================================

import argparse
import json
import math
import datetime
import sys
import urllib.request
import urllib.error


BACKEND_URL = "http://localhost:5000"

# ═══════════════════════════════════════════════════════════════════════════════
# COMMAND-LINE ARGUMENTS
# ═══════════════════════════════════════════════════════════════════════════════
# Parse test metadata and curl timing data from the performance test script.

# ─── Argument parsing ─────────────────────────────────────────────────────────

parser = argparse.ArgumentParser()
parser.add_argument("--runs",    type=int,   required=True)
parser.add_argument("--success", type=int,   required=True)
parser.add_argument("--failed",  type=int,   required=True)
parser.add_argument("--version", type=str,   default="")
parser.add_argument("--raw",     type=str,   required=True)
args = parser.parse_args()

# ═══════════════════════════════════════════════════════════════════════════════
# PARSE CURL TIMING DATA
# ═══════════════════════════════════════════════════════════════════════════════
# Extract HTTP response codes and latency components (connect, pre-transfer,
# start-transfer, total) from raw curl output sent by perf_test.sh. Converts raw 
# curl output into clean Python lists that can be used for statistics.

# ─── Parse curl timing output ─────────────────────────────────────────────────

raw_entries = args.raw.strip("|").split("|")

http_codes     = []
connects       = []
pretransfers   = []
starttransfers = []
totals         = []

for entry in raw_entries:
    parts = entry.strip().split()
    if len(parts) == 5:
        code = parts[0]
        tc, tp, ts, tt = map(float, parts[1:])
        http_codes.append(code)
        connects.append(tc * 1000)
        pretransfers.append(tp * 1000)
        starttransfers.append(ts * 1000)
        totals.append(tt * 1000)

# ═══════════════════════════════════════════════════════════════════════════════
# UTILITY FUNCTIONS FOR STATISTICS
# ═══════════════════════════════════════════════════════════════════════════════
# Helper functions to compute percentiles, basic statistics (mean, stddev, min, max),
# and percentage change calculations for baseline comparisons.

# ─── Stats helpers ────────────────────────────────────────────────────────────

def percentile(sorted_data, p):
    if not sorted_data:
        return 0
    if len(sorted_data) == 1:
        return sorted_data[0]
    idx = max(0, min(math.ceil(len(sorted_data) * p) - 1, len(sorted_data) - 1))
    return sorted_data[idx]

def stats(data):
    if not data:
        return {"mean": 0, "min": 0, "max": 0, "stddev": 0, "p50": 0, "p95": 0, "p99": 0}
    s = sorted(data)
    n = len(s)
    mean = sum(s) / n
    stddev = math.sqrt(sum((x - mean) ** 2 for x in s) / n)
    return {
        "mean":   round(mean, 2),
        "min":    round(s[0], 2),
        "max":    round(s[-1], 2),
        "stddev": round(stddev, 2),
        "p50":    round(percentile(s, 0.50), 2),
        "p95":    round(percentile(s, 0.95), 2),
        "p99":    round(percentile(s, 0.99), 2),
    }

# ═══════════════════════════════════════════════════════════════════════════════
# COMPUTE DERIVED METRICS
# ═══════════════════════════════════════════════════════════════════════════════
# Calculate throughput, latency aggregates, percentiles, and component breakdown
# (server-side wait, TCP overhead, response transfer) from raw curl data.

# ─── Derived metrics ──────────────────────────────────────────────────────────

server_wait_times = [starttransfers[i] - pretransfers[i] for i in range(len(totals))]
transfer_times    = [totals[i] - starttransfers[i] for i in range(len(totals))]

total_elapsed_s   = sum(totals) / 1000
throughput_val    = round(len(totals) / total_elapsed_s, 2) if total_elapsed_s > 0 else 0
avg_latency       = round(sum(totals) / len(totals), 2) if totals else 0
success_rate      = round((args.success / args.runs) * 100, 1) if args.runs > 0 else 0

total_stats       = stats(totals)
server_wait_stats = stats(server_wait_times)
tcp_stats         = stats(connects)
transfer_stats    = stats(transfer_times)

# ═══════════════════════════════════════════════════════════════════════════════
# ASSEMBLE FINAL RESULT OBJECT
# ═══════════════════════════════════════════════════════════════════════════════
# Package all metrics into a single JSON result object for storage in MongoDB.
# Analysis is generated on demand via the UI — see backend/analyze_record.py.

# ─── Assemble final result ────────────────────────────────────────────────────

result = {
    "timestamp":       datetime.datetime.utcnow().strftime("%Y-%m-%dT%H:%M:%SZ"),
    "version":         args.version,
    "runs":            args.runs,
    "success":         args.success,
    "failed":          args.failed,
    "success_rate":    success_rate,
    "throughput_rps":  throughput_val,
    "avg_latency_ms":  avg_latency,
    "total_latency":   total_stats,
    "consensus_time_ms": server_wait_stats,
    "tcp_connect_ms":  tcp_stats,
    "transfer_time_ms": transfer_stats,
    "http_codes":      {code: http_codes.count(code) for code in sorted(set(http_codes))},
}

# ═══════════════════════════════════════════════════════════════════════════════
# PERSIST RESULT TO DATABASE
# ═══════════════════════════════════════════════════════════════════════════════
# Send the complete result object to the backend API for storage in MongoDB.
# Handles network errors gracefully with warnings.

# ─── Save to MongoDB ──────────────────────────────────────────────────────────

try:
    payload = json.dumps(result).encode("utf-8")
    req = urllib.request.Request(
        f"{BACKEND_URL}/api/results",
        data=payload,
        headers={"Content-Type": "application/json"},
        method="POST"
    )
    urllib.request.urlopen(req, timeout=5)
    print("✓ Result saved to database", file=sys.stderr)
except urllib.error.URLError as e:
    print(f"Warning: could not save to database: {e}", file=sys.stderr)
except Exception as e:
    print(f"Warning: unexpected error saving to database: {e}", file=sys.stderr)

# ═══════════════════════════════════════════════════════════════════════════════
# OUTPUT FINAL RESULT
# ═══════════════════════════════════════════════════════════════════════════════
# Print formatted JSON to stdout for consumption by perf_test.sh and the dashboard.

# ─── Print final JSON ─────────────────────────────────────────────────────────

print(json.dumps(result, indent=2))