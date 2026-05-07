#!/bin/bash

ENDPOINT="127.0.0.1:18000/v1/transactions/commit"
RUNS=${1:-500}
CONCURRENT=${2:-1}

SUCCESS=0
FAILED=0
RAW_TIMES=""

echo "Running $RUNS requests (concurrency parameter: $CONCURRENT)..." >&2
echo "Note: This script currently sends requests sequentially. CONCURRENT is recorded but not implemented." >&2

for i in $(seq 1 $RUNS); do

  RESULT=$(curl -s -o /dev/null \
    -w "%{http_code} %{time_connect} %{time_pretransfer} %{time_starttransfer} %{time_total}" \
    -X POST \
    -H "Content-Type: application/json" \
    -d "{\"id\":\"perfkey$i\",\"value\":\"perfval$i\"}" \
    "$ENDPOINT")

  HTTP_CODE=$(echo "$RESULT" | awk '{print $1}')

  if [ "$HTTP_CODE" = "200" ] || [ "$HTTP_CODE" = "201" ]; then
    SUCCESS=$((SUCCESS + 1))
  else
    FAILED=$((FAILED + 1))
  fi

  RAW_TIMES="$RAW_TIMES|$RESULT"

done

python3 - <<PYEOF

import json
import datetime
import math

raw = "$RAW_TIMES".strip("|").split("|")

runs = $RUNS
success = $SUCCESS
failed = $FAILED
concurrent = $CONCURRENT

http_codes = []
connects = []
pretransfers = []
starttransfers = []
totals = []

#
# Parse curl timing output
#

for entry in raw:

    parts = entry.strip().split()

    if len(parts) == 5:

        code = parts[0]
        tc, tp, ts, tt = map(float, parts[1:])

        http_codes.append(code)
        connects.append(tc * 1000)
        pretransfers.append(tp * 1000)
        starttransfers.append(ts * 1000)
        totals.append(tt * 1000)

#
# Percentile helper
#

def percentile(sorted_data, p):

    if not sorted_data:
        return 0

    if len(sorted_data) == 1:
        return sorted_data[0]

    idx = math.ceil((len(sorted_data) * p)) - 1
    idx = max(0, min(idx, len(sorted_data) - 1))

    return sorted_data[idx]

#
# Statistical summary
#

def stats(data):

    if not data:
        return {
            "mean": 0,
            "min": 0,
            "max": 0,
            "stddev": 0,
            "p50": 0,
            "p95": 0,
            "p99": 0,
        }

    s = sorted(data)
    n = len(s)

    mean = sum(s) / n

    variance = sum((x - mean) ** 2 for x in s) / n

    stddev = math.sqrt(variance)

    return {
        "mean": round(mean, 2),
        "min": round(s[0], 2),
        "max": round(s[-1], 2),
        "stddev": round(stddev, 2),
        "p50": round(percentile(s, 0.50), 2),
        "p95": round(percentile(s, 0.95), 2),
        "p99": round(percentile(s, 0.99), 2),
    }

#
# Derived timing categories
#
# Important:
# curl does NOT directly measure PBFT consensus time.
# time_starttransfer - time_pretransfer is better described as
# client-observed server-side wait time.
#
# This may include:
# - PBFT consensus processing
# - request queueing
# - application execution
# - serialization/deserialization
# - server scheduling delay
#

server_wait_times = [
    starttransfers[i] - pretransfers[i]
    for i in range(len(totals))
]

tcp_times = connects

transfer_times = [
    totals[i] - starttransfers[i]
    for i in range(len(totals))
]

#
# Throughput calculation
#
# Since this script currently sends requests sequentially,
# total_elapsed_s is approximated by the sum of request times.
#

total_elapsed_s = sum(totals) / 1000

throughput = (
    len(totals) / total_elapsed_s
    if total_elapsed_s > 0
    else 0
)

#
# Generate statistics
#

total_stats = stats(totals)
server_wait_stats = stats(server_wait_times)
tcp_stats = stats(tcp_times)
transfer_stats = stats(transfer_times)

avg_latency = (
    sum(totals) / len(totals)
    if totals
    else 0
)

success_rate = (
    (success / runs) * 100
    if runs > 0
    else 0
)

#
# Main benchmark output
#

result = {
    "timestamp": datetime.datetime.utcnow().strftime('%Y-%m-%dT%H:%M:%SZ'),
    "endpoint": "$ENDPOINT",
    "runs_requested": runs,
    "runs_parsed": len(totals),
    "success": success,
    "failed": failed,
    "success_rate": round(success_rate, 1),
    "concurrency_parameter": concurrent,
    "concurrency_note": "Requests are currently sent sequentially; CONCURRENT is recorded but not implemented.",
    "throughput_rps": round(throughput, 2),
    "avg_latency_ms": round(avg_latency, 2),
    "http_codes": {
        code: http_codes.count(code)
        for code in sorted(set(http_codes))
    },
    "total_latency_ms": total_stats,
    "server_wait_time_ms": server_wait_stats,
    "tcp_connect_ms": tcp_stats,
    "transfer_time_ms": transfer_stats,
}

#
# Pattern-Based Intelligent Analysis
#

analysis = []
warnings = []
diagnosis = []
recommendations = []

p50 = total_stats.get("p50", 0)
p95 = total_stats.get("p95", 0)
p99 = total_stats.get("p99", 0)
stddev = total_stats.get("stddev", 0)

server_wait_mean = server_wait_stats.get("mean", 0)
server_wait_p95 = server_wait_stats.get("p95", 0)
server_wait_p99 = server_wait_stats.get("p99", 0)

tcp_mean = tcp_stats.get("mean", 0)
transfer_mean = transfer_stats.get("mean", 0)

throughput_val = result["throughput_rps"]

#
# Basic benchmark validity checks
#

if concurrent > 1:
    warnings.append(
        "The CONCURRENT parameter is currently recorded but not actually used by the Bash loop. "
        "Requests are still being sent sequentially, so throughput should be interpreted as single-client sequential throughput."
    )

if success < runs:
    warnings.append(
        f"Only {success} out of {runs} requests returned HTTP 200/201. "
        "Latency and throughput results may be affected by failed or unexpected responses."
    )

if len(totals) < runs:
    warnings.append(
        f"Only {len(totals)} timing samples were parsed out of {runs} requested runs. "
        "Some curl outputs may have failed or been malformed."
    )

if not totals:
    warnings.append(
        "No valid curl timing samples were collected. Analysis is limited."
    )

#
# Latency distribution analysis
#

if p50 > 0:
    p95_ratio = p95 / p50
    p99_ratio = p99 / p50
else:
    p95_ratio = 0
    p99_ratio = 0

if p99_ratio >= 10:
    diagnosis.append(
        f"Severe tail-latency amplification detected: p99 latency is {round(p99_ratio, 2)}x higher than p50. "
        "Most requests complete much faster than the slowest requests, suggesting intermittent stalls, uneven request processing, or temporary backlog."
    )
    recommendations.append(
        "Inspect replica logs around the slowest requests and compare timestamps for pre-prepare, prepare, commit, and execution phases."
    )

elif p99_ratio >= 4:
    diagnosis.append(
        f"Moderate tail-latency amplification detected: p99 latency is {round(p99_ratio, 2)}x higher than p50. "
        "The system is usually responsive, but a small fraction of requests experience noticeably slower completion."
    )
    recommendations.append(
        "Run a longer benchmark and check whether the high p99 values occur periodically, during batching, or randomly."
    )

elif p99 > 0:
    diagnosis.append(
        "Latency distribution is relatively stable. Median, p95, and p99 latency do not show major tail amplification."
    )

#
# Server-side wait analysis
#

if avg_latency > 0:
    server_wait_share = round((server_wait_mean / avg_latency) * 100, 1)
else:
    server_wait_share = 0

if server_wait_share >= 70:
    diagnosis.append(
        f"Server-side wait time accounts for about {server_wait_share}% of total observed latency. "
        "This suggests most delay occurs after the request is sent and before the server begins responding."
    )
    recommendations.append(
        "Use internal ResilientDB metrics, Prometheus counters, or PBFT phase logs to separate consensus time from queuing, execution, and serialization overhead."
    )

elif server_wait_share >= 40:
    diagnosis.append(
        f"Server-side wait time accounts for about {server_wait_share}% of total observed latency. "
        "This indicates that server processing is a meaningful contributor, but not necessarily the only source of delay."
    )

elif avg_latency > 0:
    diagnosis.append(
        f"Server-side wait time accounts for only about {server_wait_share}% of total observed latency. "
        "The bottleneck may not be primarily inside the server-side request processing path."
    )

#
# Network and connection overhead analysis
#

if tcp_mean > 5:
    diagnosis.append(
        f"TCP connection time is elevated at {tcp_mean} ms on average. "
        "This suggests connection setup overhead may be affecting benchmark results."
    )
    recommendations.append(
        "Consider using persistent connections or a benchmark client that reuses connections instead of opening a new curl connection for every request."
    )

elif tcp_mean > 1:
    diagnosis.append(
        f"TCP connection time is mildly elevated at {tcp_mean} ms on average. "
        "This is not necessarily the main bottleneck, but it may add noise to short-latency measurements."
    )

else:
    diagnosis.append(
        "TCP connection overhead is low, so network connection setup does not appear to dominate latency."
    )

#
# Transfer/response overhead analysis
#

if transfer_mean > server_wait_mean and transfer_mean > 1:
    diagnosis.append(
        "Response transfer time is larger than server-side wait time. "
        "This may indicate response handling, payload size, or client-side transfer overhead is contributing to total latency."
    )
    recommendations.append(
        "Check response payload size and compare results with smaller payloads or persistent HTTP connections."
    )

#
# Variance analysis
#

if avg_latency > 0 and stddev > avg_latency:
    diagnosis.append(
        f"Latency variance is high: standard deviation is {round(stddev / avg_latency, 2)}x the average latency. "
        "This means the system is not behaving uniformly across requests."
    )
    recommendations.append(
        "Repeat the benchmark multiple times and compare whether variance persists across runs or appears only in isolated trials."
    )

elif avg_latency > 0 and stddev < avg_latency * 0.25:
    diagnosis.append(
        "Latency variance is low, indicating consistent request completion times during this benchmark."
    )

#
# Throughput interpretation
#

if throughput_val < 100:
    diagnosis.append(
        f"Throughput is low at {throughput_val} requests/sec. "
        "This may reflect high per-request latency, sequential request execution, or server-side processing delays."
    )

elif throughput_val > 1000:
    diagnosis.append(
        f"Throughput is high at {throughput_val} requests/sec, but because the current Bash loop is sequential, "
        "this should be interpreted as rapid single-client turnaround rather than true multi-client scalability."
    )

else:
    diagnosis.append(
        f"Throughput is moderate at {throughput_val} requests/sec under the current benchmark configuration."
    )

#
# Historical Baseline Data
#
# Replace these values with your real baseline runs.
#

historical_runs = [
    {
        "commit": "c9de6cfb",
        "avg_latency_ms": 0.31,
        "throughput_rps": 3206.09,
        "p99": 0.77,
        "server_wait_mean": 0.22,
    },
    {
        "commit": "bbe485ae",
        "avg_latency_ms": 0.34,
        "throughput_rps": 2961.91,
        "p99": 0.99,
        "server_wait_mean": 0.23,
    },
    {
        "commit": "d827c259",
        "avg_latency_ms": 0.04,
        "throughput_rps": 28460.84,
        "p99": 0.09,
        "server_wait_mean": 0.00,
    },
    {
        "commit": "8416beb3",
        "avg_latency_ms": 0.81,
        "throughput_rps": 1231.35,
        "p99": 17.14,
        "server_wait_mean": 0.55,
    },
]

#
# Historical helper functions
#

def avg_field(field):
    return sum(x[field] for x in historical_runs) / len(historical_runs)

def min_field(field):
    return min(x[field] for x in historical_runs)

def max_field(field):
    return max(x[field] for x in historical_runs)

def pct_change(current, baseline):
    if baseline == 0:
        return None
    return round(((current - baseline) / baseline) * 100, 1)

#
# Historical averages
#

if historical_runs:

    hist_avg_latency = avg_field("avg_latency_ms")
    hist_avg_throughput = avg_field("throughput_rps")
    hist_avg_p99 = avg_field("p99")
    hist_avg_server_wait = avg_field("server_wait_mean")

    latency_change = pct_change(avg_latency, hist_avg_latency)
    throughput_change = pct_change(throughput_val, hist_avg_throughput)
    p99_change = pct_change(p99, hist_avg_p99)
    server_wait_change = pct_change(server_wait_mean, hist_avg_server_wait)

    baseline_summary = {
        "historical_commits": [x["commit"] for x in historical_runs],
        "historical_avg_latency_ms": round(hist_avg_latency, 2),
        "historical_latency_range_ms": [
            round(min_field("avg_latency_ms"), 2),
            round(max_field("avg_latency_ms"), 2),
        ],
        "historical_avg_throughput_rps": round(hist_avg_throughput, 2),
        "historical_throughput_range_rps": [
            round(min_field("throughput_rps"), 2),
            round(max_field("throughput_rps"), 2),
        ],
        "historical_avg_p99_ms": round(hist_avg_p99, 2),
        "historical_p99_range_ms": [
            round(min_field("p99"), 2),
            round(max_field("p99"), 2),
        ],
        "historical_avg_server_wait_ms": round(hist_avg_server_wait, 2),
        "historical_server_wait_range_ms": [
            round(min_field("server_wait_mean"), 2),
            round(max_field("server_wait_mean"), 2),
        ],
    }

    #
    # Historical interpretation
    #

    if latency_change is not None:
        if latency_change > 25:
            diagnosis.append(
                f"Average latency is {latency_change}% higher than the historical baseline average. "
                "This suggests a possible performance regression relative to previous commits."
            )
            recommendations.append(
                "Compare this commit against the nearest historical commit by date to determine whether the regression is recent or part of a longer trend."
            )

        elif latency_change < -25:
            diagnosis.append(
                f"Average latency is {abs(latency_change)}% lower than the historical baseline average. "
                "This suggests improved request turnaround relative to previous commits."
            )

        else:
            diagnosis.append(
                "Average latency is within the expected historical baseline range."
            )

    if throughput_change is not None:
        if throughput_change < -25:
            diagnosis.append(
                f"Throughput is {abs(throughput_change)}% lower than the historical baseline average. "
                "This may indicate reduced request-processing efficiency."
            )
            recommendations.append(
                "Check whether the same benchmark parameters, payload size, and server configuration were used for all historical runs."
            )

        elif throughput_change > 25:
            diagnosis.append(
                f"Throughput is {throughput_change}% higher than the historical baseline average. "
                "This may indicate improved performance, but should be validated with repeated runs."
            )

    if p99_change is not None and p99_change > 50:
        diagnosis.append(
            f"p99 latency is {p99_change}% higher than the historical baseline average. "
            "The main regression appears to be in tail latency rather than only average latency."
        )
        recommendations.append(
            "Prioritize investigating slow outlier requests rather than only optimizing average latency."
        )

    if server_wait_change is not None and server_wait_change > 25:
        diagnosis.append(
            f"Server-side wait time is {server_wait_change}% higher than the historical baseline average. "
            "This points toward increased delay after the client sends the request and before the server begins responding."
        )
        recommendations.append(
            "Use ResilientDB internal metrics, Prometheus counters, or PBFT phase logs to confirm whether the extra wait comes from consensus, queuing, execution, or serialization."
        )

else:

    latency_change = None
    throughput_change = None
    p99_change = None
    server_wait_change = None
    baseline_summary = {}

    warnings.append(
        "No historical baseline data was provided, so regression analysis is limited to the current run."
    )

#
# Overall classification
#

severity_score = 0

if latency_change is not None and latency_change > 25:
    severity_score += 1

if throughput_change is not None and throughput_change < -25:
    severity_score += 1

if p99_change is not None and p99_change > 50:
    severity_score += 1

if avg_latency > 0 and stddev > avg_latency:
    severity_score += 1

if success_rate < 100:
    severity_score += 1

if severity_score >= 3:
    overall_status = "possible_regression"
elif severity_score == 2:
    overall_status = "needs_attention"
elif severity_score == 1:
    overall_status = "minor_warning"
else:
    overall_status = "stable"

#
# Add intelligent explanations
#

analysis.append({
    "overall_status": overall_status,
    "warnings": warnings,
    "diagnosis": diagnosis,
    "recommendations": recommendations,
    "historical_baseline_summary": baseline_summary,
    "historical_percent_changes": {
        "avg_latency_change_pct": latency_change,
        "throughput_change_pct": throughput_change,
        "p99_change_pct": p99_change,
        "server_wait_change_pct": server_wait_change,
    },
    "interpretation_note": (
        "Curl timing cannot directly prove PBFT consensus latency. "
        "server_wait_time_ms represents client-observed delay between request preparation and first byte received. "
        "This may include PBFT consensus, queuing, execution, serialization, or server scheduling overhead."
    )
})

result["analysis"] = analysis

#
# Print final JSON
#

print(json.dumps(result, indent=2))

PYEOF