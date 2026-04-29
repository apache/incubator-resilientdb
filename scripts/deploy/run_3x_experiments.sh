#!/bin/bash
#
# Run each protocol 3 times in LAN mode and record results.
# Usage: ./run_3x_experiments.sh <ip_conf>
#

set +e

if [ $# -lt 1 ]; then
    echo "Usage: $0 <ip_conf>"
    exit 1
fi

IP_CONF=$1
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

. ./script/env.sh
. ./script/load_config.sh "$IP_CONF"

TIMESTAMP=$(date +%Y%m%d_%H%M%S)
RESULTS_DIR="${SCRIPT_DIR}/experiment_results/3x_${TIMESTAMP}"
mkdir -p "$RESULTS_DIR"

PROTOCOLS=(
    "pbft:pbft"
    "hs:hs"
    "rcc:rcc"
    "tusk:tusk"
    "bullshark:bullshark"
    "mysticeti:mysticeti"
    "shoalpp:shoalpp"
    "fides:fides"
    "damysus:damysus"
    "achilles:achilles"
    "hybridset:hybridset"
)

RUNS=3

CSV="${RESULTS_DIR}/all_results.csv"
SUMMARY="${RESULTS_DIR}/summary.txt"
echo "protocol,run,max_tps,avg_tps,max_latency,avg_latency,status" > "$CSV"

TOTAL=$((${#PROTOCOLS[@]} * RUNS))
COUNT=0

cleanup_nodes() {
    for ip in ${iplist[@]}; do
        ssh $ssh_options_cloud -p 2222 -i ${key} -n -o BatchMode=yes -o StrictHostKeyChecking=no root@${ip} \
            "killall -9 kv_server_performance 2>/dev/null" 2>/dev/null &
        ssh $ssh_options_cloud -p 22 -i ${key} -n -o BatchMode=yes -o StrictHostKeyChecking=no root@${ip} \
            "tc qdisc del dev eth0 root 2>/dev/null" 2>/dev/null &
    done
    wait
    sleep 2
}

echo "============================================================" | tee "$SUMMARY"
echo "  3x Performance Experiments - All Protocols (LAN)"            | tee -a "$SUMMARY"
echo "============================================================" | tee -a "$SUMMARY"
echo "IP Config  : $IP_CONF"                                        | tee -a "$SUMMARY"
echo "Replicas   : $((${#iplist[@]} - client_num))"                  | tee -a "$SUMMARY"
echo "Clients    : $client_num"                                      | tee -a "$SUMMARY"
echo "Protocols  : ${#PROTOCOLS[@]}"                                 | tee -a "$SUMMARY"
echo "Runs/proto : $RUNS"                                            | tee -a "$SUMMARY"
echo "Started    : $(date)"                                          | tee -a "$SUMMARY"
echo "============================================================" | tee -a "$SUMMARY"
echo "" | tee -a "$SUMMARY"

for proto_entry in "${PROTOCOLS[@]}"; do
    PROTO_NAME="${proto_entry%%:*}"
    SCRIPT_PREFIX="${proto_entry##*:}"
    PERF_SCRIPT="./performance/${SCRIPT_PREFIX}_performance.sh"

    echo "" | tee -a "$SUMMARY"
    echo ">>> Protocol: $PROTO_NAME <<<" | tee -a "$SUMMARY"

    if [ ! -f "$PERF_SCRIPT" ]; then
        echo "  SKIP - script not found: $PERF_SCRIPT" | tee -a "$SUMMARY"
        for run in $(seq 1 $RUNS); do
            echo "$PROTO_NAME,$run,0,0,0,0,SKIP" >> "$CSV"
        done
        continue
    fi

    for run in $(seq 1 $RUNS); do
        COUNT=$((COUNT + 1))
        echo "------------------------------------------------------------"
        echo "[$COUNT/$TOTAL] $PROTO_NAME - Run $run/$RUNS (LAN)"
        echo "------------------------------------------------------------"

        cleanup_nodes

        EXP_DIR="${RESULTS_DIR}/${PROTO_NAME}/run${run}"
        mkdir -p "$EXP_DIR"

        export DEPLOY_DELAY_MS=0
        export DEPLOY_JITTER_MS=0

        echo ">> Running: $PERF_SCRIPT $IP_CONF (run $run)"
        $PERF_SCRIPT "$IP_CONF" > "${EXP_DIR}/run.log" 2>&1
        EXIT_CODE=$?

        if [ $EXIT_CODE -eq 0 ] && [ -f results.log ]; then
            cp results.log "${EXP_DIR}/results.log"

            MAX_TPS=$(grep -oP 'max throughput:\K[0-9.]+' results.log | tail -1 || echo "0")
            AVG_TPS=$(grep -oP 'average throughput:\K[0-9.]+' results.log | tail -1 || echo "0")
            MAX_LAT=$(grep -oP 'max latency:\K[0-9.]+' results.log | tail -1 || echo "0")
            AVG_LAT=$(grep -oP 'average latency:\K[0-9.]+' results.log | tail -1 || echo "0")

            MAX_TPS=${MAX_TPS:-0}
            AVG_TPS=${AVG_TPS:-0}
            MAX_LAT=${MAX_LAT:-0}
            AVG_LAT=${AVG_LAT:-0}

            echo "  Run $run OK | TPS: max=${MAX_TPS} avg=${AVG_TPS} | Lat: max=${MAX_LAT} avg=${AVG_LAT}" | tee -a "$SUMMARY"
            echo "$PROTO_NAME,$run,$MAX_TPS,$AVG_TPS,$MAX_LAT,$AVG_LAT,OK" >> "$CSV"

            # Save breakdown
            if grep -q "Breakdown Analysis" results.log; then
                grep -A 20 "Breakdown Analysis" results.log > "${EXP_DIR}/breakdown.log"
            fi
        else
            echo "  Run $run FAIL (exit=$EXIT_CODE)" | tee -a "$SUMMARY"
            echo "$PROTO_NAME,$run,0,0,0,0,FAIL" >> "$CSV"
        fi

        sleep 5
    done
done

cleanup_nodes

echo "" | tee -a "$SUMMARY"
echo "============================================================" | tee -a "$SUMMARY"
echo "  ALL 3x EXPERIMENTS COMPLETE"                                 | tee -a "$SUMMARY"
echo "  Finished: $(date)"                                           | tee -a "$SUMMARY"
echo "============================================================" | tee -a "$SUMMARY"

echo ""
echo "--- Results Summary ---"
column -t -s',' "$CSV" 2>/dev/null || cat "$CSV"
echo ""
echo "Full results: $RESULTS_DIR"
echo "CSV file:     $CSV"
