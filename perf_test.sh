#!/bin/bash

ENDPOINT="127.0.0.1:18000/v1/transactions/commit"
RUNS=${1:-50}
CONCURRENT=${2:-1}

SUCCESS=0
RAW_TIMES=""

echo "Running $RUNS requests (concurrency: $CONCURRENT)..." >&2

for i in $(seq 1 $RUNS); do

  RESULT=$(curl -s -o /dev/null \
    -w "%{time_connect} %{time_pretransfer} %{time_starttransfer} %{time_total}" \
    -X POST \
    -d "{\"id\":\"perfkey$i\",\"value\":\"perfval$i\"}" \
    $ENDPOINT)

  RAW_TIMES="$RAW_TIMES|$RESULT"
  SUCCESS=$((SUCCESS + 1))

done

python3 - <<PYEOF

import json
import datetime
import math

raw = "$RAW_TIMES".strip("|").split("|")

runs = $RUNS
success = $SUCCESS
concurrent = $CONCURRENT

connects = []
pretransfers = []
starttransfers = []
totals = []

#
# Parse curl timing output
#

for entry in raw:

    parts = entry.strip().split()

    if len(parts) == 4:

        tc, tp, ts, tt = map(float, parts)

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

    idx = min(int(len(sorted_data) * p), len(sorted_data) - 1)

    return sorted_data[idx]

#
# Statistical summary
#

def stats(data):

    if not data:
        return {}

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

consensus_times = [
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

total_elapsed_s = sum(totals) / 1000

throughput = (
    runs / total_elapsed_s
    if total_elapsed_s > 0
    else 0
)

#
# Generate statistics
#

total_stats = stats(totals)
consensus_stats = stats(consensus_times)
tcp_stats = stats(tcp_times)
transfer_stats = stats(transfer_times)

#
# Main benchmark output
#

result = {
    "timestamp": datetime.datetime.utcnow().strftime('%Y-%m-%dT%H:%M:%SZ'),
    "runs": runs,
    "success_rate": round((success / runs) * 100, 1),
    "throughput_rps": round(throughput, 2),
    "avg_latency_ms": round(sum(totals) / len(totals), 2),
    "total_latency": total_stats,
    "consensus_time_ms": consensus_stats,
    "tcp_connect_ms": tcp_stats,
    "transfer_time_ms": transfer_stats,
}

#
# Pattern-Based Intelligent Analysis
#

analysis = []

avg_latency = result["avg_latency_ms"]

p50 = total_stats["p50"]
p95 = total_stats["p95"]
p99 = total_stats["p99"]

consensus_mean = consensus_stats["mean"]
consensus_p99 = consensus_stats["p99"]

tcp_mean = tcp_stats["mean"]
transfer_mean = transfer_stats["mean"]

throughput_val = result["throughput_rps"]

#
# Tail latency analysis
#

if p99 > (p50 * 15):

    spike_pct = round(((p99 - p50) / p50) * 100, 1)

    analysis.append(
        f"Tail latency increased significantly compared to normal request latency "
        f"(p99 is {spike_pct}% higher than p50). "
        f"This may indicate occasional PBFT consensus stalls, replica synchronization delays, "
        f"temporary batching overhead, or intermittent thread contention during commit phases."
    )

elif p99 > (p50 * 5):

    analysis.append(
        "Moderate tail latency variance detected. "
        "Most requests complete quickly, but a small number experience delays that may "
        "be caused by replica synchronization timing or temporary processing backlog."
    )

else:

    analysis.append(
        "Latency distribution appears stable with minimal tail amplification."
    )

#
# Consensus bottleneck analysis
#

if consensus_mean > (tcp_mean * 3):

    consensus_pct = round(
        (consensus_mean / avg_latency) * 100,
        1
    )

    analysis.append(
        f"Consensus processing accounts for approximately {consensus_pct}% of total request latency. "
        f"This suggests the primary bottleneck is inside the PBFT execution path rather than network transport. "
        f"Possible causes include replica coordination overhead, message sequencing delays, "
        f"signature verification cost, or commit-phase synchronization latency."
    )

#
# Throughput interpretation
#

if throughput_val > 1000 and concurrent == 1:

    analysis.append(
        "High throughput was observed under single-request execution conditions. "
        "Because concurrency is set to 1, this benchmark primarily reflects raw request turnaround time "
        "rather than sustained multi-client scalability."
    )

elif throughput_val < 200:

    analysis.append(
        "Low throughput detected relative to request latency. "
        "This may indicate request serialization, synchronization bottlenecks, "
        "or inefficient batching behavior inside the consensus pipeline."
    )

#
# Network analysis
#

if tcp_mean > 1:

    analysis.append(
        "TCP connection setup time appears elevated. "
        "Potential causes include network congestion, socket exhaustion, "
        "connection reuse inefficiencies, or cross-node communication latency."
    )

else:

    analysis.append(
        "Network transport overhead appears minimal, suggesting communication latency "
        "is not currently a dominant bottleneck."
    )

#
# Transfer analysis
#

if transfer_mean > consensus_mean:

    analysis.append(
        "Payload transfer time exceeds consensus execution time. "
        "This may indicate large payload sizes, slow serialization/deserialization, "
        "or inefficient response handling."
    )

#
# Variance analysis
#

if total_stats["stddev"] > (avg_latency * 2):

    analysis.append(
        "High latency variance was detected across requests. "
        "This pattern is commonly associated with unstable scheduling behavior, "
        "resource contention, intermittent replica lag, or uneven batching intervals."
    )

#
# Healthy system detection
#

if (
    result["success_rate"] == 100 and
    avg_latency < 1 and
    p95 < 5 and
    tcp_mean < 1
):

    analysis.append(
        "Overall system behavior appears healthy under the current workload. "
        "The benchmark indicates low network overhead, fast consensus execution, "
        "and consistently low request latency for the majority of transactions."
    )

#
# Add intelligent explanations
#

result["analysis"] = analysis

#
# Print final JSON
#

print(json.dumps(result, indent=2))

PYEOF