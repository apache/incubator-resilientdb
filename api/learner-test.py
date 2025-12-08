# import sys, os, time, subprocess

# subprocess.run("bazel build :pybind_kv_so", shell=True)
# sys.path.append(os.getcwd())

from kv_operation import set_value, get_value, get_value_readonly
import sys, os, time, statistics, threading, queue, subprocess, matplotlib.pyplot as plt

# Build and import the kv_operation module
def build_kv_binding():
    subprocess.run("bazel build :pybind_kv_so", shell=True, check=True)
    sys.path.append(os.getcwd())
    from kv_operation import set_value, get_value, get_value_readonly
    return set_value, get_value, get_value_readonly

# Simple stats helper
def summarize_latencies(latencies):
    if not latencies:
        return {}
    return {
        "count": len(latencies),
        "mean": statistics.mean(latencies),
        "median": statistics.median(latencies),
        "p95": statistics.quantiles(latencies, n=20)[18],  # approx 95th
        "p99": statistics.quantiles(latencies, n=100)[98], # approx 99th
        "min": min(latencies),
        "max": max(latencies),
    }

# Generic worker to send operations and record per-op latency
def run_client_worker(op_func, num_requests, key_prefix, thread_id, result_queue):
    latencies = []
    successes = 0
    for i in range(num_requests):
        key = f"{key_prefix}_{thread_id}_{i}"
        # Some experiments will treat op_func as GET, some as SET, etc.
        start = time.time()
        try:
            op_func(key)
            successes += 1
        except Exception as e:
            # Log or ignore for pseudocode
            pass
        end = time.time()
        latencies.append(end - start)
    # Push results back to main thread
    result_queue.put((successes, latencies))

# Experiment 1: Read Latency Comparison
def exp1_read_latency(set_value, get_value, get_value_readonly):
    """
    Compare read latency for PBFT vs learner under different concurrency levels.
    Assumes KV already populated with some keys.
    """
    # Pre-populate a test key so both paths can read something
    base_key = "EXP1_KEY"
    set_value(base_key, "warmup_value")

    client_concurrency_levels = [1, 4, 8, 16, 32]
    num_requests_per_client = 100

    results = []

    for num_clients in client_concurrency_levels:
        print(f"\n=== Exp1: {num_clients} clients ===")

        for mode in ["pbft", "learner"]:
            if mode == "pbft":
                read_func = lambda k: get_value(k)
            else:
                read_func = lambda k: get_value_readonly(k)

            # All clients read same key to stress the path
            def op_func(_ignored_key):
                # Always read base_key
                return read_func(base_key)

            result_queue = queue.Queue()
            threads = []

            start_wall = time.time()
            for cid in range(num_clients):
                t = threading.Thread(
                    target=run_client_worker,
                    args=(op_func, num_requests_per_client, "EXP1", cid, result_queue)
                )
                t.start()
                threads.append(t)

            for t in threads:
                t.join()
            end_wall = time.time()

            # Aggregate latencies
            all_latencies = []
            total_success = 0
            while not result_queue.empty():
                successes, lat = result_queue.get()
                total_success += successes
                all_latencies.extend(lat)

            stats = summarize_latencies(all_latencies)
            throughput = total_success / (end_wall - start_wall)

            results.append({
                "mode": mode,
                "num_clients": num_clients,
                "throughput": throughput,
                "latency_stats": stats,
            })

            print(f"Mode={mode}, clients={num_clients}")
            print(f"  throughput={throughput:.2f} reads/sec")
            print(f"  latency: {stats}")

    return results

def run_mixed_client_worker(
    set_value,
    read_func,
    key_prefix,
    thread_id,
    read_ratio,
    total_ops,
    result_queue,
):
    """
    A worker that issues a mix of reads and writes on a *single existing key*.
    This avoids calling get_value_readonly() on keys that were never written.
    """
    latencies_reads = []
    latencies_writes = []
    num_reads = 0
    num_writes = 0

    # Use one key per client, pre-populated before measurement
    key = f"{key_prefix}_{thread_id}"

    for i in range(total_ops):
        is_read = (i % 100) < int(read_ratio * 100)

        if is_read:
            start = time.time()
            try:
                read_func(key)
                num_reads += 1
            except Exception:
                # Optional: log, but don't kill the worker
                pass
            end = time.time()
            latencies_reads.append(end - start)
        else:
            value = str(i)
            start = time.time()
            try:
                set_value(key, value)
                num_writes += 1
            except Exception:
                pass
            end = time.time()
            latencies_writes.append(end - start)

    result_queue.put({
        "reads": num_reads,
        "writes": num_writes,
        "latencies_reads": latencies_reads,
        "latencies_writes": latencies_writes,
    })

# Experiment 2: Mixed Read/Write Workload
def exp2_mixed_workload(set_value, get_value, get_value_readonly):
    """
    Measure write throughput and latency under different read/write mixes,
    comparing PBFT vs learner for reads.
    Now:
      - pre-populates one key per client,
      - only reads/writes existing keys.
    """
    mixes = [0.0, 0.5, 0.9, 0.99]  # fraction of reads
    num_clients = 8
    total_ops_per_client = 200

    results = []

    # Pre-populate keys so both PBFT and learner know about them
    for cid in range(num_clients):
        key = f"EXP2_{cid}"
        try:
            set_value(key, "0")
        except Exception as e:
            print(f"Pre-populate failed for {key}: {e}")

    for read_ratio in mixes:
        print(f"\n=== Exp2: read_ratio={read_ratio} ===")

        for mode in ["pbft", "learner"]:
            if mode == "pbft":
                read_func = lambda k: get_value(k)
            else:
                read_func = lambda k: get_value_readonly(k)

            result_queue = queue.Queue()
            threads = []

            start_wall = time.time()
            for cid in range(num_clients):
                t = threading.Thread(
                    target=run_mixed_client_worker,
                    args=(
                        set_value,
                        read_func,
                        "EXP2",
                        cid,
                        read_ratio,
                        total_ops_per_client,
                        result_queue,
                    ),
                )
                t.start()
                threads.append(t)

            for t in threads:
                t.join()
            end_wall = time.time()

            # Aggregate
            total_reads = 0
            total_writes = 0
            all_read_lat = []
            all_write_lat = []
            while not result_queue.empty():
                r = result_queue.get()
                total_reads += r["reads"]
                total_writes += r["writes"]
                all_read_lat.extend(r["latencies_reads"])
                all_write_lat.extend(r["latencies_writes"])

            duration = end_wall - start_wall
            read_throughput = total_reads / duration if duration > 0 else 0
            write_throughput = total_writes / duration if duration > 0 else 0

            stats_reads = summarize_latencies(all_read_lat)
            stats_writes = summarize_latencies(all_write_lat)

            results.append({
                "mode": mode,
                "read_ratio": read_ratio,
                "read_throughput": read_throughput,
                "write_throughput": write_throughput,
                "latency_reads": stats_reads,
                "latency_writes": stats_writes,
            })

            print(f"Mode={mode}, read_ratio={read_ratio}")
            print(f"  read_throughput={read_throughput:.2f} rps")
            print(f"  write_throughput={write_throughput:.2f} wps")
            print(f"  read latency: {stats_reads}")
            print(f"  write latency: {stats_writes}")

    return results

# Experiment 3: Staleness and Fallback Rate
def exp3_staleness_and_fallback(set_value, get_value, get_value_readonly):
    """
    Measure how often learner reads differ from PBFT reads (staleness)
    and how often learner fails and forces fallback.
    """
    base_key = "EXP3_KEY"

    num_rounds = 100
    sleep_between_writes = 0.1  # simulate blocks committing over time

    num_equal = 0
    num_stale = 0
    num_fallback = 0
    learner_latencies = []
    pbft_latencies = []

    for i in range(num_rounds):
        # Write a fresh value through PBFT (normal write)
        new_value = f"value_{i}"
        set_value(base_key, new_value)
        time.sleep(sleep_between_writes)

        # Ground truth read via PBFT
        start_p = time.time()
        truth = get_value(base_key)
        end_p = time.time()
        pbft_latencies.append(end_p - start_p)

        # Try learner read
        start_l = time.time()
        try:
            learner_val = get_value_readonly(base_key)
            end_l = time.time()
            learner_latencies.append(end_l - start_l)

            if learner_val == truth:
                num_equal += 1
            else:
                # learner responded but value != PBFT truth ⇒ stale
                num_stale += 1
        except Exception:
            # interpret as fallback / failure to serve from learner
            end_l = time.time()
            learner_latencies.append(end_l - start_l)
            num_fallback += 1

    total = num_rounds
    stats_learner = summarize_latencies(learner_latencies)
    stats_pbft = summarize_latencies(pbft_latencies)

    print("\n=== Exp3: Staleness & Fallback ===")
    print(f"Total trials: {total}")
    print(f"  equal (fresh)   : {num_equal} ({num_equal/total:.2%})")
    print(f"  stale responses : {num_stale} ({num_stale/total:.2%})")
    print(f"  fallbacks       : {num_fallback} ({num_fallback/total:.2%})")
    print(f"  learner latency : {stats_learner}")
    print(f"  PBFT latency    : {stats_pbft}")

    return {
        "total": total,
        "equal": num_equal,
        "stale": num_stale,
        "fallback": num_fallback,
        "latency_learner": stats_learner,
        "latency_pbft": stats_pbft,
    }

# Experiment 4: Fault Injection Sanity Check
def exp4_fault_injection_sanity(set_value, get_value, get_value_readonly):
    """
    Assumes that on the server side you have configured one replica
    to send bad digests to the learner.

    From the client side, we:
      - write monotone increasing integers to a key,
      - read via PBFT as ground truth,
      - read via learner, and
      - check for impossible values.
    """
    num_rounds = 100
    fallback_thresholds = [0.1, 0.01, 0.001, 0.0001]  # seconds
    all_results = []

    for th_idx, th in enumerate(fallback_thresholds):
        key = f"EXP4_KEY_{th_idx}"

        print(f"Testing with fallback threshold: {th} seconds")
        impossible_values = []
        mismatches = 0
        fallbacks = 0

        pbft_latencies = []
        learner_latencies = []
        highest_seen = -1

        for i in range(num_rounds):
            # Write monotone integer
            set_value(key, str(i))
            time.sleep(0.1)

            # Ground truth
            time0 = time.time()
            truth = get_value(key)
            time1 = time.time()
            # Took pbft this time to get value
            pbft_latencies.append(time1 - time0)

            time2 = time.time()
            val = get_value_readonly(key)
            time3 = time.time()       
            # Took learner this time to get value
            learner_latency = time3 - time2
            learner_latencies.append(learner_latency)
            
            if learner_latency > th:
                # If learner took too long, consider it a fallback
                fallbacks += 1

            if val != truth:
                mismatches += 1
                    # If you never wrote val at all, this is impossible
            try:
                int_val = int(val)
            except ValueError:
                # non-integer, clearly impossible in this setup
                impossible_values.append(val)
                continue
            
            if int_val < 0 or int_val > i:
                impossible_values.append(val)

            # Rollback check: learner should not move backwards in value
            if int_val < highest_seen:
                impossible_values.append(
                    f"rollback: was {highest_seen}, now {int_val}"
                )
            else:
                highest_seen = max(highest_seen, int_val)

        print("\n=== Exp4: Fault Injection (Timing-based) ===")
        print(f"Threshold:             {th} seconds")
        print(f"Total rounds:          {num_rounds}")
        print(f"Fallback reads (slow): {fallbacks}")
        print(f"Mismatches total:      {mismatches}")
        print(f"Impossible values:     {impossible_values}")
        print(f"Learner latencies:     {summarize_latencies(learner_latencies)}")
        print(f"PBFT latencies:        {summarize_latencies(pbft_latencies)}")

        all_results.append({
            "threshold": th,
            "num_rounds": num_rounds,
            "mismatches": mismatches,
            "fallbacks": fallbacks,
            "impossible_values": impossible_values,
            "learner_latencies": summarize_latencies(learner_latencies),
            "pbft_latencies": summarize_latencies(pbft_latencies),
        })

    return all_results

def plot_exp1_latency(exp1_results, metric="median"):
    """
    exp1_results: list of dicts from exp1_read_latency()
        each dict has:
            "mode": "pbft" or "learner"
            "num_clients": int
            "throughput": float
            "latency_stats": {"median": ..., "p95": ..., ...}

    metric: which stat to plot: "median", "p95", "p99", etc.
    """
    modes = ["pbft", "learner"]
    num_clients = sorted(set(r["num_clients"] for r in exp1_results))

    plt.figure()
    for mode in modes:
        ys = []
        for c in num_clients:
            entry = next(
                r for r in exp1_results
                if r["mode"] == mode and r["num_clients"] == c
            )
            ys.append(entry["latency_stats"][metric])
        plt.plot(num_clients, ys, marker="o", label=mode)

    plt.xlabel("Number of clients")
    plt.ylabel(f"Read latency ({metric}) [seconds]")
    plt.title(f"Experiment 1: {metric} read latency vs clients")
    plt.legend()
    plt.grid(True)
    plt.tight_layout()
    
    filename = os.path.join(PLOTS_DIR, f"exp1_latency_{metric}.pdf")
    plt.savefig(filename, dpi=300)
    

def plot_exp1_throughput(exp1_results):
    modes = ["pbft", "learner"]
    num_clients = sorted(set(r["num_clients"] for r in exp1_results))

    plt.figure()
    for mode in modes:
        ys = []
        for c in num_clients:
            entry = next(
                r for r in exp1_results
                if r["mode"] == mode and r["num_clients"] == c
            )
            ys.append(entry["throughput"])
        plt.plot(num_clients, ys, marker="o", label=mode)

    plt.xlabel("Number of clients")
    plt.ylabel("Read throughput [ops/sec]")
    plt.title("Experiment 1: read throughput vs clients")
    plt.legend()
    plt.grid(True)
    plt.tight_layout()

    filename = os.path.join(PLOTS_DIR, "exp1_throughput.pdf")
    plt.savefig(filename, dpi=300)
    

def plot_exp2_write_throughput(exp2_results):
    """
    exp2_results: list of dicts from exp2_mixed_workload(), each dict:
        "mode": "pbft" or "learner"
        "read_ratio": float
        "read_throughput": float
        "write_throughput": float
        "latency_reads": {...}
        "latency_writes": {...}
    """
    modes = ["pbft", "learner"]
    read_ratios = sorted(set(r["read_ratio"] for r in exp2_results))

    plt.figure()
    for mode in modes:
        ys = []
        for rr in read_ratios:
            entry = next(
                r for r in exp2_results
                if r["mode"] == mode and r["read_ratio"] == rr
            )
            ys.append(entry["write_throughput"])
        plt.plot(read_ratios, ys, marker="o", label=mode)

    plt.xlabel("Read ratio (fraction of operations that are reads)")
    plt.ylabel("Write throughput [ops/sec]")
    plt.title("Experiment 2: write throughput vs read ratio")
    plt.legend()
    plt.grid(True)
    plt.tight_layout()

    filename = os.path.join(PLOTS_DIR, "exp2_write_throughput.pdf")
    plt.savefig(filename, dpi=300)
    


def plot_exp2_read_throughput(exp2_results):
    modes = ["pbft", "learner"]
    read_ratios = sorted(set(r["read_ratio"] for r in exp2_results))

    plt.figure()
    for mode in modes:
        ys = []
        for rr in read_ratios:
            entry = next(
                r for r in exp2_results
                if r["mode"] == mode and r["read_ratio"] == rr
            )
            ys.append(entry["read_throughput"])
        plt.plot(read_ratios, ys, marker="o", label=mode)

    plt.xlabel("Read ratio (fraction of operations that are reads)")
    plt.ylabel("Read throughput [ops/sec]")
    plt.title("Experiment 2: read throughput vs read ratio")
    plt.legend()
    plt.grid(True)
    plt.tight_layout()

    filename = os.path.join(PLOTS_DIR, "exp2_read_throughput.pdf")
    plt.savefig(filename, dpi=300)
    

def plot_exp3_staleness(exp3_results):
    """
    Visual summary of how learner behaves:
      - fraction of fresh reads (equal to PBFT)
      - fraction of stale reads (value != PBFT)
      - fraction of fallbacks (learner couldn't serve)
    """
    total = exp3_results["total"]
    equal = exp3_results["equal"]
    stale = exp3_results["stale"]
    fallback = exp3_results["fallback"]

    labels = ["Fresh (equal)", "Stale", "Fallback"]
    counts = [equal, stale, fallback]
    fractions = [c / total if total > 0 else 0 for c in counts]

    plt.figure()
    x = range(len(labels))
    plt.bar(x, fractions)
    plt.xticks(x, labels, rotation=15)
    plt.ylim(0, 1.0)

    plt.ylabel("Fraction of reads")
    plt.title("Experiment 3: Learner read outcomes (fresh vs stale vs fallback)")
    plt.grid(axis="y")
    plt.tight_layout()

    filename = os.path.join(PLOTS_DIR, "exp3_staleness.pdf")
    plt.savefig(filename, dpi=300)
    

def plot_exp3_latency(exp3_results):
    """
    Compare learner vs PBFT latency from Experiment 3
    using median and p95 stats.
    """
    stats_learner = exp3_results["latency_learner"]
    stats_pbft = exp3_results["latency_pbft"]

    metrics = ["median", "p95"]
    x = range(len(metrics))

    pbft_vals = [stats_pbft[m] for m in metrics]
    learner_vals = [stats_learner[m] for m in metrics]

    width = 0.35

    plt.figure()
    plt.bar([xi - width/2 for xi in x], pbft_vals, width, label="pbft")
    plt.bar([xi + width/2 for xi in x], learner_vals, width, label="learner")

    plt.xticks(list(x), metrics)
    plt.ylabel("Latency [seconds]")
    plt.title("Experiment 3: Latency comparison (PBFT vs learner)")
    plt.legend()
    plt.grid(axis="y")
    plt.tight_layout()

    filename = os.path.join(PLOTS_DIR, "exp3_latency.pdf")
    plt.savefig(filename, dpi=300)
    

def plot_exp4_fault_summary(exp4_results):
    """
    Visual summary of learner behavior under faulty replica digests.

    exp4_results is a list of dicts, each like:
      {
        "threshold": th,
        "num_rounds": num_rounds,
        "mismatches": mismatches,
        "fallbacks": fallbacks,
        "impossible_values": [...],
        "learner_latencies": {...},
        "pbft_latencies": {...},
      }
    """
    import matplotlib.pyplot as plt
    import os

    # Extract per-threshold stats
    thresholds = [r["threshold"] for r in exp4_results]
    mismatches = [r["mismatches"] for r in exp4_results]
    fallbacks = [r["fallbacks"] for r in exp4_results]
    impossible_counts = [len(r["impossible_values"]) for r in exp4_results]

    x = list(range(len(thresholds)))
    width = 0.25

    plt.figure()

    # Grouped bars: one group per threshold
    plt.bar([i - width for i in x], mismatches, width, label="Mismatches")
    plt.bar(x, fallbacks, width, label="Fallbacks")
    plt.bar([i + width for i in x], impossible_counts, width, label="Impossible values")

    # X-axis labels = thresholds
    plt.xticks(x, [str(th) for th in thresholds], rotation=15)
    plt.xlabel("Fallback threshold (seconds)")
    plt.ylabel("Count")
    plt.title("Experiment 4: Learner behavior under faulty digest injection")
    plt.legend()
    plt.grid(axis="y")
    plt.tight_layout()

    filename = os.path.join(PLOTS_DIR, "exp4_fault_summary.pdf")
    plt.savefig(filename, dpi=300)
    plt.close()


PLOTS_DIR = "plots"

def ensure_plots_dir():
    os.makedirs(PLOTS_DIR, exist_ok=True)

if __name__ == "__main__":
    ensure_plots_dir()

    set_value, get_value, get_value_readonly = build_kv_binding()

    # Run experiments
    # exp1_results = exp1_read_latency(set_value, get_value, get_value_readonly)
    # plot_exp1_latency(exp1_results, metric="median")
    # plot_exp1_latency(exp1_results, metric="p95")
    # plot_exp1_throughput(exp1_results)

    # exp2_results = exp2_mixed_workload(set_value, get_value, get_value_readonly)
    # plot_exp2_write_throughput(exp2_results)
    # plot_exp2_read_throughput(exp2_results)

    # exp3_results = exp3_staleness_and_fallback(set_value, get_value, get_value_readonly)
    # plot_exp3_staleness(exp3_results)
    # plot_exp3_latency(exp3_results)

    # Only run exp4 when there is a faulty-replica setup ready
    exp4_results = exp4_fault_injection_sanity(set_value, get_value, get_value_readonly)
    plot_exp4_fault_summary(exp4_results)








