#!/usr/bin/env python3
# analyze_record.py
# On-demand analysis of an existing performance record against a given baseline.
# Input:  JSON on stdin — { record: {...}, baseline: {...} | null, period_label: "..." }
# Output: Analysis JSON to stdout

import json
import sys


def pct_change(current, baseline):
    if not baseline:
        return None
    return round(((current - baseline) / baseline) * 100, 1)


def analyze(record, baseline, period_label=""):
    avg_latency       = record.get("avg_latency_ms", 0) or 0
    throughput_val    = record.get("throughput_rps", 0) or 0
    success_rate      = record.get("success_rate", 0) or 0
    total_stats       = record.get("total_latency") or {}
    server_wait_stats = record.get("consensus_time_ms") or {}
    tcp_stats         = record.get("tcp_connect_ms") or {}
    transfer_stats    = record.get("transfer_time_ms") or {}

    p50              = total_stats.get("p50", 0) or 0
    p99              = total_stats.get("p99", 0) or 0
    stddev           = total_stats.get("stddev", 0) or 0
    server_wait_mean = server_wait_stats.get("mean", 0) or 0
    tcp_mean         = tcp_stats.get("mean", 0) or 0
    transfer_mean    = transfer_stats.get("mean", 0) or 0

    warnings         = []
    baseline_summary = {}

    if baseline:
        hist_avg_latency     = (baseline.get("avg_latency_ms") or {}).get("mean", 0) or 0
        hist_avg_throughput  = (baseline.get("throughput_rps") or {}).get("mean", 0) or 0
        hist_avg_server_wait = (baseline.get("consensus_time_ms") or {}).get("mean", 0) or 0
        record_count         = baseline.get("count", 0)
        period_start         = baseline.get("period_start")

        baseline_summary = {
            "source":                        "mongodb",
            "record_count":                  record_count,
            "period_start":                  period_start,
            "historical_avg_latency_ms":     round(hist_avg_latency, 2),
            "historical_avg_throughput_rps": round(hist_avg_throughput, 2),
            "historical_avg_server_wait_ms": round(hist_avg_server_wait, 2),
        }

        latency_change     = pct_change(avg_latency,     hist_avg_latency)
        throughput_change  = pct_change(throughput_val,  hist_avg_throughput)
        server_wait_change = pct_change(server_wait_mean, hist_avg_server_wait)
    else:
        warnings.append(f"No baseline data available for the selected period.")
        latency_change = throughput_change = server_wait_change = None
        hist_avg_latency = hist_avg_throughput = 0

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
        period_str = f"the {period_label} " if period_label else "the historical "

        if latency_change is not None:
            if latency_change > 25:
                diagnosis.append(
                    f"Average latency is {latency_change}% higher than {period_str}baseline — possible performance regression."
                )
                recommendations.append(
                    f"A {latency_change}% latency increase warrants a commit-level investigation. "
                    "Use 'git bisect' between the last known-good commit and this one to pinpoint the change that introduced the slowdown. "
                    "Once identified, compare the changed code paths against PBFT phase timing to find the bottleneck."
                )
            elif latency_change < -25:
                diagnosis.append(f"Average latency is {abs(latency_change)}% lower than {period_str}baseline — performance improved.")
            else:
                diagnosis.append(f"Average latency is within the expected {period_str}baseline range.")

        if throughput_change is not None:
            if throughput_change < -25:
                diagnosis.append(f"Throughput is {abs(throughput_change)}% below {period_str}baseline — reduced efficiency.")
                recommendations.append(
                    f"A {abs(throughput_change)}% throughput drop with the same benchmark parameters usually means a new bottleneck appeared in the request path. "
                    "Verify that the number of active replicas, network routes, and consensus configuration match historical runs. "
                    "If configuration is unchanged, profile the request handling loop — look for new synchronisation points or lock contention introduced since the last baseline run."
                )
            elif throughput_change > 25:
                diagnosis.append(f"Throughput is {throughput_change}% above {period_str}baseline — improved performance.")

        if server_wait_change is not None and server_wait_change > 25:
            diagnosis.append(
                f"Server-side wait time is {server_wait_change}% higher than {period_str}baseline. "
                "Increased delay between request send and first byte received — check PBFT phase logs."
            )
            recommendations.append(
                f"A {server_wait_change}% increase in server-side wait time suggests the PBFT consensus path has slowed. "
                "Inspect the pre-prepare → prepare → commit phase logs for the slowest requests. "
                "Check whether signature verification, disk I/O, or network round-trip time between replicas has increased since the last baseline run."
            )

    # Overall status
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

    return {
        "overall_status":              overall_status,
        "warnings":                    warnings,
        "diagnosis":                   diagnosis,
        "recommendations":             recommendations,
        "historical_baseline_summary": baseline_summary,
        "historical_percent_changes":  {
            "avg_latency_change_pct":  latency_change,
            "throughput_change_pct":   throughput_change,
            "server_wait_change_pct":  server_wait_change,
        },
    }


if __name__ == "__main__":
    inp          = json.loads(sys.stdin.read())
    record       = inp["record"]
    baseline     = inp.get("baseline")
    period_label = inp.get("period_label", "")
    print(json.dumps(analyze(record, baseline, period_label)))
