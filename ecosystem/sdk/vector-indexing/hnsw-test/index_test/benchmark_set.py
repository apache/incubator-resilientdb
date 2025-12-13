import subprocess
import time
import os
import sys

# --- CONFIGURATION ---
# Update these paths if your script is not in the root of the repo
BINARY_PATH = "bazel-bin/service/tools/kv/api_tools/kv_service_tools"
CONFIG_PATH = "service/tools/config/interface/service.config"
# ---------------------

def run_benchmark(key, value_file_path):
    # Check if file exists
    if not os.path.exists(value_file_path):
        print(f"Error: File {value_file_path} not found.")
        return

    file_size_mb = os.path.getsize(value_file_path) / (1024 * 1024)
    print(f"Preparing to set key='{key}' with file='{value_file_path}' ({file_size_mb:.2f} MB)...")

    # Construct the command
    # Note: Using the 'value_path' flag (short flag -p) we added earlier
    cmd = [
        BINARY_PATH,
        "--config", CONFIG_PATH,
        "--cmd", "set_with_version",
        "--key", key,
        "--version", "0",
        "--value_path", value_file_path 
    ]

    print("Running command...")
    
    # Start Timer
    start_time = time.time()
    
    try:
        # Run the process
        result = subprocess.run(cmd, capture_output=True, text=True)
        
        # Stop Timer
        end_time = time.time()
        duration = end_time - start_time
        
        if result.returncode != 0:
            print("\n❌ Command failed!")
            print("Error output:", result.stderr)
            print("Standard output:", result.stdout)
        else:
            print(f"\n✅ Success!")
            print(f"Time taken: {duration:.4f} seconds")
            
            # Calculate throughput if duration is non-zero
            if duration > 0:
                throughput = file_size_mb / duration
                print(f"Throughput: {throughput:.2f} MB/s")
            
            # Optional: Print program output (truncated)
            # print("Output:", result.stdout)

    except FileNotFoundError:
        print(f"\n❌ Error: Could not find binary at {BINARY_PATH}")
        print("Did you run 'bazel build ...'?")

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python3 benchmark_set.py <key> <path_to_value_file>")
        print("Example: python3 benchmark_set.py key1 val_10mb.txt")
        sys.exit(1)
        
    target_key = sys.argv[1]
    target_file = sys.argv[2]
    
    run_benchmark(target_key, target_file)