#!/usr/bin/env python3
# =============================================================================
# analyze.py
# ResilientDB Performance Analyzer
#
# Called automatically by perf_test.sh.
# Fetches baseline from MongoDB, runs pattern-based analysis,
# saves result to MongoDB, and prints final JSON.
# =============================================================================

import argparse
import json
import math
import datetime
import sys
import urllib.request
import urllib.error

# ─── Config ──────────────────────────────────────────────────────────────────

BACKEND_URL = "http://localhost:5000"

# ─── Argument parsing ─────────────────────────────────────────────────────────

parser = argparse.ArgumentParser()
parser.add_argument("--runs",    type=int,   required=True)
parser.add_argument("--success", type=int,   required=True)
parser.add_argument("--failed",  type=int,   required=True)
parser.add_argument("--version", type=str,   default="")
parser.add_argument("--raw",     type=str,   required=True)
args = parser.parse_args()

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

def pct_change(current, baseline):
    if not baseline:
        return None
    return round(((current - baseline) / baseline) * 100, 1)

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

p50     = total_stats["p50"]
p95     = total_stats["p95"]
p99     = total_stats["p99"]
stddev  = total_stats["stddev"]

server_wait_mean = server_wait_stats["mean"]
tcp_mean         = tcp_stats["mean"]
transfer_mean    = transfer_stats["mean"]

# ─── Fetch baseline from MongoDB ──────────────────────────────────────────────

baseline        = None
baseline_summary = {}
warnings        = []

try:
    with urllib.request.urlopen(f"{BACKEND_URL}/api/results/baseline", timeout=5) as resp:
        data = json.loads(resp.read().decode())
        baseline = data.get("data")
        if baseline:
            baseline_summary = {
                "source":                        "mongodb",
                "record_count":                  data.get("count"),
                "period_start":                  data.get("period_start"),
                "historical_avg_latency_ms":     round(baseline.get("avg_latency_ms", {}).get("mean", 0), 2),
                "historical_avg_throughput_rps": round(baseline.get("throughput_rps", {}).get("mean", 0), 2),
                "historical_avg_server_wait_ms": round(baseline.get("consensus_time_ms", {}).get("mean", 0), 2),
            }
        else:
            warnings.append("No baseline data in database yet. Analysis will be limited to current run.")
except urllib.error.URLError as e:
    warnings.append(f"Could not reach backend at {BACKEND_URL}: {str(e)}. Skipping baseline comparison.")
except Exception as e:
    warnings.append(f"Unexpected error fetching baseline: {str(e)}")

# ─── Compute % changes vs baseline ───────────────────────────────────────────

hist_avg_latency     = baseline.get("avg_latency_ms", {}).get("mean", 0)     if baseline else 0
hist_avg_throughput  = baseline.get("throughput_rps", {}).get("mean", 0)     if baseline else 0
hist_avg_server_wait = baseline.get("consensus_time_ms", {}).get("mean", 0)  if baseline else 0

latency_change      = pct_change(avg_latency,     hist_avg_latency)     if baseline else None
throughput_change   = pct_change(throughput_val,  hist_avg_throughput)  if baseline else None
server_wait_change  = pct_change(server_wait_mean, hist_avg_server_wait) if baseline else None

# ─── Pattern-based analysis ───────────────────────────────────────────────────

diagnosis       = []
recommendations = []

# Tail latency
p99_ratio = (p99 / p50) if p50 > 0 else 0

if p99_ratio >= 10:
    diagnosis.append(
        f"Severe tail-latency amplification detected: p99 is {round(p99_ratio, 2)}x higher than p50. "
        "This suggests intermittent PBFT consensus stalls, replica synchronisation delays, or uneven batching."
    )
    recommendations.append(
        "Inspect replica logs around the slowest requests. Compare timestamps for pre-prepare, prepare, commit, and execution phases."
    )
elif p99_ratio >= 4:
    diagnosis.append(
        f"Moderate tail-latency amplification: p99 is {round(p99_ratio, 2)}x higher than p50. "
        "Most requests are fast but a small fraction experience slower PBFT completion."
    )
else:
    diagnosis.append("Latency distribution is relatively stable with minimal tail amplification.")

# Server wait share
server_wait_share = round((server_wait_mean / avg_latency) * 100, 1) if avg_latency > 0 else 0

if server_wait_share >= 70:
    diagnosis.append(
        f"Server-side wait time accounts for {server_wait_share}% of total latency. "
        "The primary bottleneck is inside the PBFT execution path. "
        "Possible causes: replica coordination overhead, message sequencing delays, "
        "signature verification cost, or commit-phase synchronisation latency."
    )
elif server_wait_share >= 40:
    diagnosis.append(
        f"Server-side wait accounts for {server_wait_share}% of total latency. "
        "Server processing is a meaningful contributor but not the only source of delay."
    )
elif avg_latency > 0:
    diagnosis.append(
        f"Server-side wait accounts for only {server_wait_share}% of total latency. "
        "The bottleneck may not be inside the server-side PBFT processing path."
    )

# TCP overhead
if tcp_mean > 5:
    diagnosis.append(
        f"TCP connection time is elevated at {tcp_mean}ms. "
        "Connection setup overhead may be affecting results."
    )
    recommendations.append("Consider using persistent connections instead of a new curl connection per request.")
elif tcp_mean > 1:
    diagnosis.append(f"TCP connection time is mildly elevated at {tcp_mean}ms.")
else:
    diagnosis.append("TCP connection overhead is low — network setup is not the dominant bottleneck.")

# Transfer vs server wait
if transfer_mean > server_wait_mean and transfer_mean > 1:
    diagnosis.append(
        "Response transfer time exceeds server-side wait time. "
        "Payload size or client-side transfer overhead may be contributing to total latency."
    )
    recommendations.append("Check response payload size and test with smaller payloads or persistent HTTP connections.")

# Variance
if avg_latency > 0 and stddev > avg_latency:
    diagnosis.append(
        f"High latency variance detected: stddev is {round(stddev / avg_latency, 2)}x the average. "
        "The system is not behaving uniformly across requests — possible replica lag or uneven batching intervals."
    )
    recommendations.append("Repeat the benchmark multiple times to confirm whether variance persists.")
elif avg_latency > 0 and stddev < avg_latency * 0.25:
    diagnosis.append("Latency variance is low — consistent request completion times observed.")

# Throughput
if throughput_val < 100:
    diagnosis.append(f"Throughput is low at {throughput_val} req/s. May reflect high per-request latency or server-side delays.")
elif throughput_val > 1000:
    diagnosis.append(f"High throughput at {throughput_val} req/s under sequential single-client execution.")
else:
    diagnosis.append(f"Moderate throughput at {throughput_val} req/s.")

# Historical comparison
if baseline:
    if latency_change is not None:
        if latency_change > 25:
            diagnosis.append(
                f"Average latency is {latency_change}% higher than the historical baseline — possible performance regression."
            )
            recommendations.append("Compare this commit against the nearest historical commit to locate when the regression appeared.")
        elif latency_change < -25:
            diagnosis.append(f"Average latency is {abs(latency_change)}% lower than the historical baseline — performance improved.")
        else:
            diagnosis.append("Average latency is within the expected historical baseline range.")

    if throughput_change is not None:
        if throughput_change < -25:
            diagnosis.append(f"Throughput is {abs(throughput_change)}% below the historical baseline — reduced efficiency.")
            recommendations.append("Verify that benchmark parameters and payload sizes match historical runs.")
        elif throughput_change > 25:
            diagnosis.append(f"Throughput is {throughput_change}% above the historical baseline — improved performance.")

    if server_wait_change is not None and server_wait_change > 25:
        diagnosis.append(
            f"Server-side wait time is {server_wait_change}% higher than the historical baseline. "
            "Increased delay between request send and first byte received — check PBFT phase logs."
        )

# ─── Overall status ───────────────────────────────────────────────────────────

severity = 0
if latency_change    is not None and latency_change    > 25:  severity += 1
if throughput_change is not None and throughput_change < -25: severity += 1
if avg_latency > 0   and stddev > avg_latency:                severity += 1
if success_rate < 100:                                         severity += 1
if p99_ratio >= 10:                                            severity += 1

if   severity >= 3: overall_status = "possible_regression"
elif severity == 2: overall_status = "needs_attention"
elif severity == 1: overall_status = "minor_warning"
else:               overall_status = "stable"

# ─── Assemble final result ────────────────────────────────────────────────────

analysis_obj = {
    "overall_status":             overall_status,
    "warnings":                   warnings,
    "diagnosis":                  diagnosis,
    "recommendations":            recommendations,
    "historical_baseline_summary": baseline_summary,
    "historical_percent_changes": {
        "avg_latency_change_pct":    latency_change,
        "throughput_change_pct":     throughput_change,
        "server_wait_change_pct":    server_wait_change,
    },
}

result = {
    "timestamp":        datetime.datetime.utcnow().strftime("%Y-%m-%dT%H:%M:%SZ"),
    "version":          args.version,
    "runs":             args.runs,
    "success":          args.success,
    "failed":           args.failed,
    "success_rate":     success_rate,
    "throughput_rps":   throughput_val,
    "avg_latency_ms":   avg_latency,
    "total_latency":    total_stats,
    "consensus_time_ms": server_wait_stats,
    "tcp_connect_ms":   tcp_stats,
    "transfer_time_ms": transfer_stats,
    "http_codes":       {code: http_codes.count(code) for code in sorted(set(http_codes))},
    "analysis":         json.dumps(analysis_obj),
}

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

# ─── Print final JSON ─────────────────────────────────────────────────────────

display = {**result, "analysis": analysis_obj}
print(json.dumps(display, indent=2))