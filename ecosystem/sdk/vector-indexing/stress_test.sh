#!/bin/bash

# Configuration: Do not stop on error immediately to allow capturing failure state
# set -e is intentionally omitted

echo "=== Stress Test Started: Adding 15 items sequentially ==="

# Base word list for generating random unique sentences
words=("Blockchain" "Database" "Resilient" "Consensus" "Python" "Vector" "Search" "Index" "Node" "Performance" "Latency" "Throughput" "Security" "Encryption" "Network")

# Initialize total time counter
total_duration=0

for i in {1..15}
do
    # Generate a unique text string using a random word and the current loop index
    rand_idx=$((RANDOM % 15))
    text="Test entry #$i: ${words[$rand_idx]} related data with random seed $RANDOM"
    
    echo "---------------------------------------------------"
    echo "[Step $i/15] Adding data: '$text'"
    
    # --- TIMING START ---
    start_time=$(date +%s%3N)

    # 1. Add data to ResilientDB (vector_add.py)
    python3 kv_vector.py --add "$text"
    
    # Capture exit code immediately
    exit_code=$?

    # --- TIMING END ---
    end_time=$(date +%s%3N)
    duration=$((end_time - start_time))
    total_duration=$((total_duration + duration))

    # Check execution success
    if [ $exit_code -ne 0 ]; then
        echo "❌ [CRITICAL FAIL] vector_add.py crashed at step $i."
        echo "   -> The upload process likely failed."
        exit 1
    else
        echo "⏱️  Add operation took: ${duration} ms"
    fi

    # 2. Retrieve and verify the data immediately (vector_get.py)
    echo "[Check] Verifying index integrity..."
    python3 kv_vector.py --get "Test entry #$i" --k_matches 1 > /dev/null
    
    # Check if the retrieval script executed successfully
    if [ $? -ne 0 ]; then
        echo "❌ [CRITICAL FAIL] vector_get.py crashed at step $i!"
        echo "   -> The index file retrieved from ResDB is likely corrupted."
        exit 1
    else
        echo "✅ [OK] Retrieve successful. Index is valid."
    fi
    
    # Wait slightly to be gentle on the local server
    sleep 1
done

echo "==================================================="
echo "🎉 Congratulations! The system survived the stress test."
echo "   Total time spent on 'add' operations: ${total_duration} ms"
echo "==================================================="