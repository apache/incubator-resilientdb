#!/bin/bash

# Configuration: Do not stop on error immediately to allow capturing failure state
# set -e is intentionally omitted

echo "=== Stress Test Started: Adding 15 items sequentially ==="

# Base word list for generating random unique sentences
words=("Blockchain" "Database" "Resilient" "Consensus" "Python" "Vector" "Search" "Index" "Node" "Performance" "Latency" "Throughput" "Security" "Encryption" "Network")

for i in {1..15}
do
    # Generate a unique text string using a random word and the current loop index
    rand_idx=$((RANDOM % 15))
    text="Test entry #$i: ${words[$rand_idx]} related data with random seed $RANDOM"
    
    echo "---------------------------------------------------"
    echo "[Step $i/15] Adding data: '$text'"
    
    # 1. Add data to ResilientDB (vector_add.py)
    # This generates the HNSW index locally and uploads the binary files to ResDB
    python3 vector_add.py --value "$text"
    
    # Check if the python script executed successfully
    if [ $? -ne 0 ]; then
        echo "❌ [CRITICAL FAIL] vector_add.py crashed at step $i."
        echo "   -> The upload process likely failed."
        exit 1
    fi

    # 2. Retrieve and verify the data immediately (vector_get.py)
    # This downloads the binary files from ResDB and attempts to load the index
    echo "[Check] Verifying index integrity..."
    python3 vector_get.py --value "Test entry #$i" --k_matches 1 > /dev/null
    
    # Check if the retrieval script executed successfully
    if [ $? -ne 0 ]; then
        echo "❌ [CRITICAL FAIL] vector_get.py crashed at step $i!"
        echo "   -> The index file retrieved from ResDB is likely corrupted."
        echo "   -> This confirms the 'binary-to-text' saving issue."
        exit 1
    else
        echo "✅ [OK] Retrieve successful. Index is valid."
    fi
    
    # Wait slightly to be gentle on the local server
    sleep 1
done

echo "==================================================="
echo "🎉 Congratulations! The system survived the stress test."
echo "   It successfully handled 15 sequential adds and reloads."
echo "==================================================="
