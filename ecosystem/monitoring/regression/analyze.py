#!/usr/bin/env python3
# =============================================================================
# analyze.py
# ResilientDB Performance Analyzer
#
# Called automatically by perf_test.sh.
# Fetches baseline from MongoDB, runs pattern-based analysis,
# saves result to MongoDB, and prints final JSON.
#
# For a full explanation of how curl timing values are converted into
# latency, throughput, percentiles, server-side wait time, and baseline
# comparisons, see:
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

def pct_change(current, baseline):
    if not baseline:
        return None
    return round(((current - baseline) / baseline) * 100, 1)

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
# FETCH BASELINE FROM DATABASE
# ═══════════════════════════════════════════════════════════════════════════════
# Request historical baseline metrics from the backend to enable regression detection
# and performance trend analysis.

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

# ═══════════════════════════════════════════════════════════════════════════════
# COMPUTE PERCENTAGE CHANGES VS BASELINE
# ═══════════════════════════════════════════════════════════════════════════════
# Calculate deltas in latency, throughput, and server-side wait time relative to
# the historical baseline for regression detection.

# ─── Compute % changes vs baseline ───────────────────────────────────────────

hist_avg_latency     = baseline.get("avg_latency_ms", {}).get("mean", 0)     if baseline else 0
hist_avg_throughput  = baseline.get("throughput_rps", {}).get("mean", 0)     if baseline else 0
hist_avg_server_wait = baseline.get("consensus_time_ms", {}).get("mean", 0)  if baseline else 0

latency_change      = pct_change(avg_latency,     hist_avg_latency)     if baseline else None
throughput_change   = pct_change(throughput_val,  hist_avg_throughput)  if baseline else None
server_wait_change  = pct_change(server_wait_mean, hist_avg_server_wait) if baseline else None

# ═══════════════════════════════════════════════════════════════════════════════
# PATTERN-BASED PERFORMANCE ANALYSIS
# ═══════════════════════════════════════════════════════════════════════════════
# Apply heuristic pattern matching to detect performance anomalies and bottlenecks:
# - Tail latency amplification (p99/p50 ratio)
# - Server-side wait dominance
# - TCP overhead
# - Transfer vs server-side wait imbalance
# - Latency variance
# - Throughput anomalies
# - Historical regression detection
#
# Each pattern generates diagnosis insights and actionable recommendations.

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
        "Tail-latency outliers at this scale usually point to replica stalls or batch timeouts. "
        "Enable per-phase PBFT logging and compare timestamps for pre-prepare, prepare, commit, and execute across replicas. "
        "Try reducing the batch timeout (e.g. --batch-timeout 10ms) and re-run to see if the outliers shrink."
    )
elif p99_ratio >= 4:
    diagnosis.append(
        f"Moderate tail-latency amplification: p99 is {round(p99_ratio, 2)}x higher than p50. "
        "Most requests are fast but a small fraction experience slower PBFT completion."
    )
    recommendations.append(
        "Tail latency outliers at this level are often caused by uneven batch flushing or periodic GC pauses. "
        "Add a warmup phase before measuring (run and discard the first 10–20% of requests) and check if the p99 improves. "
        "You can also try tuning --batch-size or --batch-timeout to reduce the frequency of slow flushes."
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
    recommendations.append(
        "TCP setup cost at this level will inflate every request's latency. "
        "Switch to persistent HTTP connections (curl --keep-alive or an HTTP/1.1 client with connection pooling) "
        "to eliminate the per-request handshake overhead and get a cleaner read on server-side latency."
    )
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
    recommendations.append(
        "When transfer time dominates, the bottleneck is in the response size or the network path back to the client, not the server logic. "
        "Measure the response payload size (e.g. with curl -w '%{size_download}') and test with a stripped-down response. "
        "Switching to persistent HTTP connections will also cut repeated handshake overhead."
    )

# Variance
if avg_latency > 0 and stddev > avg_latency:
    diagnosis.append(
        f"High latency variance detected: stddev is {round(stddev / avg_latency, 2)}x the average. "
        "The system is not behaving uniformly across requests — possible replica lag or uneven batching intervals."
    )
    recommendations.append(
        "High variance usually means the system is in a cold state, hitting GC pauses, or experiencing intermittent replica lag. "
        "Run the benchmark at least three times and compare — if the variance is consistent, it points to a structural issue. "
        "Add a warmup phase (10–20% extra requests before measurement) and pin the client to a single CPU core to reduce scheduling noise."
    )
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
            recommendations.append(
                f"A {latency_change}% latency increase warrants a commit-level investigation. "
                "Use 'git bisect' between the last known-good commit and this one to pinpoint the change that introduced the slowdown. "
                "Once identified, compare the changed code paths against PBFT phase timing to find the bottleneck."
            )
        elif latency_change < -25:
            diagnosis.append(f"Average latency is {abs(latency_change)}% lower than the historical baseline — performance improved.")
        else:
            diagnosis.append("Average latency is within the expected historical baseline range.")

    if throughput_change is not None:
        if throughput_change < -25:
            diagnosis.append(f"Throughput is {abs(throughput_change)}% below the historical baseline — reduced efficiency.")
            recommendations.append(
                f"A {abs(throughput_change)}% throughput drop with the same benchmark parameters usually means a new bottleneck appeared in the request path. "
                "Verify that the number of active replicas, network routes, and consensus configuration match historical runs. "
                "If configuration is unchanged, profile the request handling loop — look for new synchronisation points or lock contention introduced since the last baseline run."
            )
        elif throughput_change > 25:
            diagnosis.append(f"Throughput is {throughput_change}% above the historical baseline — improved performance.")

    if server_wait_change is not None and server_wait_change > 25:
        diagnosis.append(
            f"Server-side wait time is {server_wait_change}% higher than the historical baseline. "
            "Increased delay between request send and first byte received — check PBFT phase logs."
        )
        recommendations.append(
            f"A {server_wait_change}% increase in server-side wait time suggests the PBFT consensus path has slowed. "
            "Inspect the pre-prepare → prepare → commit phase logs for the slowest requests. "
            "Check whether signature verification, disk I/O, or network round-trip time between replicas has increased since the last baseline run."
        )

# ═══════════════════════════════════════════════════════════════════════════════
# COMPUTE OVERALL STATUS
# ═══════════════════════════════════════════════════════════════════════════════
# Aggregate severity signals from all pattern analyses to assign a final run status:
# stable, minor_warning, needs_attention, or possible_regression.

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

# ═══════════════════════════════════════════════════════════════════════════════
# ASSEMBLE FINAL RESULT OBJECT
# ═══════════════════════════════════════════════════════════════════════════════
# Package all metrics, analysis, recommendations, and warnings into a single JSON
# result object for storage and display in the monitoring dashboard.

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

display = {**result, "analysis": analysis_obj}
print(json.dumps(display, indent=2))