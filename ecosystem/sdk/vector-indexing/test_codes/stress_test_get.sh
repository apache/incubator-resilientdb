#!/bin/bash

# Configuration
OUTPUT_FILE="stress_test_get_results.txt"
NUM_ITERATIONS=500
K_MATCHES_INT=10
# Clear the result file initially
> "$OUTPUT_FILE"

echo "=== Stress Test Started: Getting $NUM_ITERATIONS items sequentially ===" | tee -a "$OUTPUT_FILE"
echo "Date: $(date)" | tee -a "$OUTPUT_FILE"

# Base word list
words=("Blockchain" "Database" "Resilient" "Consensus" "Python" "Vector" "Search" "Index" "Node" "Performance" "Latency" "Throughput" "Security" "Encryption" "Network" "Scalability" "Fault-tolerance" "Replication" "Sharding" "Caching" "Load-balancing" "Monitoring" "Logging" "Alerting" "Backup" "Recovery" "Cloud" "Container" "Orchestration" "Microservices" "API" "SDK" "Framework" "Library" "Algorithm" "Data-structure" "Optimization" "Parallelism" "Concurrency" "Threading" "Asynchronous" "Synchronous" "Event-driven" "Message-queue" "Pub-sub" "Websocket" "RESTful" "GraphQL" "JSON" "XML" "YAML" "CSV" "SQL" "NoSQL" "ORM" "CLI" "GUI" "UX" "UI" "DevOps" "CI/CD" "Testing" "Unit-test" "Integration-test" "E2E-test" "Mocking" "Stubbing" "Profiling" "Debugging" "Version-control" "Git" "Branching" "Merging" "Pull-request" "Code-review" "Documentation" "Tutorial" "Example" "Sample" "Template" "Boilerplate" "Best-practices" "Design-patterns" "Architecture" "UML" "ERD" "Flowchart" "Diagram")

# Initialize counters
total_duration=0
batch_duration=0

for i in $(seq 1 $NUM_ITERATIONS)
do
    # Generate unique text
    rand_idx=$((RANDOM % ${#words[@]}))
		text="${words[$rand_idx]}"
    # 1. Write Step info to FILE (Always do this so the log is complete)
    echo "---------------------------------------------------" >> "$OUTPUT_FILE"
    echo "[Step $i/$NUM_ITERATIONS] Processing..." >> "$OUTPUT_FILE"
    echo "Getting data: '$text'" >> "$OUTPUT_FILE"

    # 2. Write Step info to CONSOLE (Only every 25 steps)
    if (( i % 25 == 0 )); then
        echo "[Step $i/$NUM_ITERATIONS] Processing..."
    fi
    
    # ---------------------------------------------------------
    # EXECUTION
    # ---------------------------------------------------------

    # --- TIMING START ---
    start_time=$(date +%s%3N)

    # Run Python script
    # Redirect standard output (>>) and errors (2>&1) ONLY to the file
    python3 kv_vector.py --get "$text" --k_matches "$K_MATCHES_INT" >> "$OUTPUT_FILE" 2>&1
    
    exit_code=$?

    # --- TIMING END ---
    end_time=$(date +%s%3N)
    duration=$((end_time - start_time))
    
    total_duration=$((total_duration + duration))
    batch_duration=$((batch_duration + duration))

    # Check execution success
    if [ $exit_code -ne 0 ]; then
        # If it fails, print to BOTH screen and file immediately
        msg="❌ [CRITICAL FAIL] vector_get.py crashed at step $i."
        echo "$msg" | tee -a "$OUTPUT_FILE"
        exit 1
    else
        # Log the individual duration ONLY to the file
        echo "⏱️  Get operation took: ${duration} ms" >> "$OUTPUT_FILE"
    fi

    # --- BATCH REPORT (Every 50 items) ---
    # Print this to BOTH screen and file
    if (( i % 50 == 0 )); then
        avg_batch=$((batch_duration / 50))
        echo "" | tee -a "$OUTPUT_FILE"
        echo "📊 [BATCH REPORT] Items $((i-49)) to $i" | tee -a "$OUTPUT_FILE"
        echo "   -> Average Latency: ${avg_batch} ms" | tee -a "$OUTPUT_FILE"
        echo "" | tee -a "$OUTPUT_FILE"
        
        # Reset batch counter
        batch_duration=0
    fi
done

# Final Results to BOTH screen and file
echo "===================================================" | tee -a "$OUTPUT_FILE"
echo "🎉 Congratulations! The system survived the stress test." | tee -a "$OUTPUT_FILE"
echo "   Total time spent on 'get' operations: ${total_duration} ms" | tee -a "$OUTPUT_FILE"

overall_avg=$((total_duration / NUM_ITERATIONS))
echo "   Overall Average Latency: ${overall_avg} ms" | tee -a "$OUTPUT_FILE"
echo "===================================================" | tee -a "$OUTPUT_FILE"