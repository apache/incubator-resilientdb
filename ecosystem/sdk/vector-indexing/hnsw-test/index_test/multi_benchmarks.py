import subprocess
import sys
import os
import re
from datetime import datetime

# --- CONFIGURATION ---
BENCHMARK_SCRIPT = "benchmark_set.py"
OUTPUT_FILE = "benchmark_results.txt"
# ---------------------

def run_tests(key, value_file, iterations):
    # 1. Setup paths
    script_dir = os.path.dirname(os.path.abspath(__file__))
    output_path = os.path.join(script_dir, OUTPUT_FILE)
    benchmark_script_path = os.path.join(script_dir, BENCHMARK_SCRIPT)

    # 2. Open the file to write results
    with open(output_path, "a") as f: # 'a' for append mode
        header = f"\n{'='*40}\nBENCHMARK RUN: {datetime.now()}\nFile: {value_file} | Iterations: {iterations}\n{'='*40}\n"
        print(header)
        f.write(header)
        
        times = []
        throughputs = []

        for i in range(1, iterations + 1):
            print(f"Running iteration {i}/{iterations}...", end="", flush=True)
            
            # Run the existing benchmark_set.py
            cmd = ["python3", benchmark_script_path, key, value_file]
            result = subprocess.run(cmd, capture_output=True, text=True)
            
            if result.returncode != 0:
                print(" ❌ Failed")
                f.write(f"Iteration {i}: FAILED\nError: {result.stderr}\n")
            else:
                output = result.stdout
                
                # Extract numbers using Regex to store for stats
                # Looking for: "Time taken: 1.2345 seconds" and "Throughput: 45.67 MB/s"
                time_match = re.search(r"Time taken:\s+([\d\.]+)", output)
                thpt_match = re.search(r"Throughput:\s+([\d\.]+)", output)
                
                if time_match and thpt_match:
                    t_val = float(time_match.group(1))
                    tp_val = float(thpt_match.group(1))
                    times.append(t_val)
                    throughputs.append(tp_val)
                    
                    log_line = f"Iteration {i}: {t_val:.4f}s | {tp_val:.2f} MB/s\n"
                    print(f" ✅ ({t_val:.4f}s)")
                    f.write(log_line)
                else:
                    print(" ⚠️ Output format unexpected")
                    f.write(f"Iteration {i}: Output format unexpected\nRaw: {output}\n")

        # 3. Calculate and write averages
        if times:
            avg_time = sum(times) / len(times)
            avg_thpt = sum(throughputs) / len(throughputs)
            summary = (f"\n{'-'*40}\n"
                       f"SUMMARY:\n"
                       f"Average Time:       {avg_time:.4f} seconds\n"
                       f"Average Throughput: {avg_thpt:.2f} MB/s\n"
                       f"{'-'*40}\n")
            print(summary)
            f.write(summary)
        
    print(f"Results saved to: {output_path}")

if __name__ == "__main__":
    if len(sys.argv) < 4:
        print("Usage: python3 run_multiple_benchmarks.py <key> <value_file> <iterations>")
        print("Example: python3 run_multiple_benchmarks.py key1 val_50mb.txt 5")
        sys.exit(1)
        
    run_tests(sys.argv[1], sys.argv[2], int(sys.argv[3]))