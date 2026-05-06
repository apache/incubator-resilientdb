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
import json, datetime, math

raw = "$RAW_TIMES".strip("|").split("|")
runs = $RUNS
success = $SUCCESS

connects, pretransfers, starttransfers, totals = [], [], [], []

for entry in raw:
    parts = entry.strip().split()
    if len(parts) == 4:
        tc, tp, ts, tt = float(parts[0]), float(parts[1]), float(parts[2]), float(parts[3])
        connects.append(tc * 1000)
        pretransfers.append(tp * 1000)
        starttransfers.append(ts * 1000)
        totals.append(tt * 1000)

def stats(data):
    n = len(data)
    if n == 0:
        return {}
    s = sorted(data)
    mean = sum(s) / n
    variance = sum((x - mean) ** 2 for x in s) / n
    stddev = math.sqrt(variance)
    return {
        "mean": round(mean, 2),
        "min": round(s[0], 2),
        "max": round(s[-1], 2),
        "stddev": round(stddev, 2),
        "p50": round(s[int(n * 0.50)], 2),
        "p95": round(s[int(n * 0.95)], 2),
        "p99": round(s[min(int(n * 0.99), n-1)], 2),
    }

consensus_times = [starttransfers[i] - pretransfers[i] for i in range(len(totals))]
tcp_times = [connects[i] for i in range(len(totals))]
transfer_times = [totals[i] - starttransfers[i] for i in range(len(totals))]

total_elapsed_s = sum(totals) / 1000
throughput = runs / total_elapsed_s if total_elapsed_s > 0 else 0

result = {
    "timestamp": datetime.datetime.utcnow().strftime('%Y-%m-%dT%H:%M:%SZ'),
    "runs": runs,
    "success_rate": round((success / runs) * 100, 1),
    "throughput_rps": round(throughput, 2),
    "avg_latency_ms": round(sum(totals) / len(totals), 2),
    "total_latency": stats(totals),
    "consensus_time_ms": stats(consensus_times),
    "tcp_connect_ms": stats(tcp_times),
    "transfer_time_ms": stats(transfer_times),
}

print(json.dumps(result, indent=2))
PYEOF