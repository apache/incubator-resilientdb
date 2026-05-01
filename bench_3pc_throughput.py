#!/usr/bin/env python3
"""Benchmark 3PC KV path: external client measures end-to-end throughput and latency."""
import statistics
import subprocess
import sys
import time

DEFAULT_N = 300

def main() -> int:
    n = int(sys.argv[1]) if len(sys.argv) > 1 else DEFAULT_N
    exe = "bazel-bin/service/tools/kv/api_tools/kv_service_tools"
    cfg = "service/tools/config/interface/service.config"
    lat = []
    t0 = time.perf_counter()
    for i in range(n):
        ts = time.perf_counter()
        r = subprocess.run(
            [
                exe,
                "--config",
                cfg,
                "--cmd",
                "set",
                "--key",
                f"bench{i}",
                "--value",
                f"v{i}",
            ],
            capture_output=True,
            text=True,
        )
        td = time.perf_counter() - ts
        lat.append(td * 1000)
        ok = r.returncode == 0 and "done" in r.stdout and "ret = 0" in r.stdout
        if not ok:
            print(f"FAIL at i={i} code={r.returncode}", file=sys.stderr)
            print(r.stdout[:500], file=sys.stderr)
            print(r.stderr[:500], file=sys.stderr)
            return 1

    total = time.perf_counter() - t0
    print(f"transactions: {n}")
    print(f"wall_seconds: {total:.4f}")
    print(f"throughput_tps_overall: {n / total:.2f}")
    print(f"avg_latency_ms: {statistics.mean(lat):.3f}")
    print(f"median_latency_ms: {statistics.median(lat):.3f}")
    if len(lat) > 1:
        print(f"stdev_latency_ms: {statistics.stdev(lat):.3f}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
