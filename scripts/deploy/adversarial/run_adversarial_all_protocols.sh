#!/bin/bash
#
# One-click adversarial experiments for ALL protocols.
# Runs 2 scenarios (crash_f + gst_ramp) for each protocol and collects results.
#
# Usage:
#   ./adversarial/run_adversarial_all_protocols.sh <ip_conf>
#
# Example:
#   cd scripts/deploy
#   ./adversarial/run_adversarial_all_protocols.sh config/kv_performance_server_16.conf

if [ $# -lt 1 ]; then
    echo "Usage: $0 <ip_conf>"
    echo "Example: $0 config/kv_performance_server_16.conf"
    exit 1
fi

IP_CONF=$1
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
DEPLOY_DIR="$(dirname "$SCRIPT_DIR")"
SCENARIO_DIR="${SCRIPT_DIR}/scenarios"
PHASES_DIR="${SCRIPT_DIR}/phases"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
RESULTS_DIR="${DEPLOY_DIR}/adversarial_results/all_protocols_${TIMESTAMP}"

cd "$DEPLOY_DIR"

# Load env for SSH options and key
. ./script/env.sh
. ./script/load_config.sh $IP_CONF

# ============================================================
# Protocol list: name -> script prefix mapping
# ============================================================
# Format: "protocol_display_name:script_prefix"
# script_prefix is used to find performance/<prefix>_adversarial.sh
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

# 2 scenarios to run
SCENARIOS=(
    "crash_f:${SCENARIO_DIR}/crash_f.json:${PHASES_DIR}/crash_phases.json"
    "gst_ramp:${SCENARIO_DIR}/gst_ramp.json:${PHASES_DIR}/gst_phases.json"
)

mkdir -p "$RESULTS_DIR"

SUMMARY="${RESULTS_DIR}/summary.txt"
CSV="${RESULTS_DIR}/comparison.csv"

echo "============================================================" | tee "$SUMMARY"
echo "  Adversarial Experiments - All Protocols"                     | tee -a "$SUMMARY"
echo "============================================================" | tee -a "$SUMMARY"
echo "IP Config : $IP_CONF"                                         | tee -a "$SUMMARY"
echo "Protocols : ${#PROTOCOLS[@]}"                                  | tee -a "$SUMMARY"
echo "Scenarios : crash_f, gst_ramp"                                 | tee -a "$SUMMARY"
echo "Results   : $RESULTS_DIR"                                      | tee -a "$SUMMARY"
echo "Started   : $(date)"                                           | tee -a "$SUMMARY"
echo "============================================================" | tee -a "$SUMMARY"
echo ""                                                              | tee -a "$SUMMARY"

# CSV header
echo "protocol,scenario,max_tps,avg_tps,max_latency,avg_latency,status" > "$CSV"

TOTAL=$((${#PROTOCOLS[@]} * ${#SCENARIOS[@]}))
COUNT=0

cleanup_nodes() {
    echo "  Cleaning up nodes..."
    for ip in ${iplist[@]}; do
        ssh $ssh_options_cloud -p 2222 -i ${key} -n -o BatchMode=yes -o StrictHostKeyChecking=no root@${ip} \
            "killall -9 kv_server_performance 2>/dev/null" &
        ssh $ssh_options_cloud -p 22 -i ${key} -n -o BatchMode=yes -o StrictHostKeyChecking=no root@${ip} \
            "tc qdisc del dev eth0 root 2>/dev/null; iptables -F 2>/dev/null" &
    done
    wait
    sleep 2
}

set +e  # Disable exit-on-error for the entire experiment loop

for proto_entry in "${PROTOCOLS[@]}"; do
    PROTO_NAME="${proto_entry%%:*}"
    SCRIPT_PREFIX="${proto_entry##*:}"
    PERF_SCRIPT="./performance/${SCRIPT_PREFIX}_adversarial.sh"

    if [ ! -f "$PERF_SCRIPT" ]; then
        echo "[SKIP] $PROTO_NAME - script not found: $PERF_SCRIPT" | tee -a "$SUMMARY"
        for scenario_entry in "${SCENARIOS[@]}"; do
            SCENARIO_NAME="${scenario_entry%%:*}"
            echo "$PROTO_NAME,$SCENARIO_NAME,0,0,0,0,SKIP" >> "$CSV"
        done
        continue
    fi

    for scenario_entry in "${SCENARIOS[@]}"; do
        IFS=':' read -r SCENARIO_NAME SCENARIO_FILE PHASES_FILE <<< "$scenario_entry"
        COUNT=$((COUNT + 1))

        echo ""
        echo "============================================================"
        echo "[$COUNT/$TOTAL] $PROTO_NAME - $SCENARIO_NAME"
        echo "============================================================"

        # Clean up before each experiment
        cleanup_nodes

        # Create per-experiment result dir
        EXP_DIR="${RESULTS_DIR}/${PROTO_NAME}_${SCENARIO_NAME}"
        mkdir -p "$EXP_DIR"

        # Run experiment
        echo ">> $PERF_SCRIPT $IP_CONF $SCENARIO_FILE $PHASES_FILE"
        $PERF_SCRIPT "$IP_CONF" "$SCENARIO_FILE" "$PHASES_FILE" > "${EXP_DIR}/run.log" 2>&1
        EXIT_CODE=$?

        if [ $EXIT_CODE -eq 0 ]; then
            # Extract key metrics from the log
            MAX_TPS=$(grep "max throughput:" "${EXP_DIR}/run.log" | tail -1 | grep -oP 'max throughput:\K[0-9.]+' || echo "0")
            AVG_TPS=$(grep "average throughput:" "${EXP_DIR}/run.log" | tail -1 | grep -oP 'average throughput:\K[0-9.]+' || echo "0")
            MAX_LAT=$(grep "max latency:" "${EXP_DIR}/run.log" | tail -1 | grep -oP 'max latency:\K[0-9.]+' || echo "0")
            AVG_LAT=$(grep "average latency:" "${EXP_DIR}/run.log" | tail -1 | grep -oP 'average latency:\K[0-9.]+' || echo "0")

            echo "  OK  | TPS: max=${MAX_TPS} avg=${AVG_TPS} | Latency: avg=${AVG_LAT}" | tee -a "$SUMMARY"
            echo "$PROTO_NAME,$SCENARIO_NAME,$MAX_TPS,$AVG_TPS,$MAX_LAT,$AVG_LAT,OK" >> "$CSV"
        else
            echo "  FAIL (exit=$EXIT_CODE)" | tee -a "$SUMMARY"
            echo "$PROTO_NAME,$SCENARIO_NAME,0,0,0,0,FAIL" >> "$CSV"
        fi

        # Copy any adversarial_results that were generated
        LATEST_ADV=$(ls -td adversarial_results/${SCENARIO_NAME}_* 2>/dev/null | head -1)
        if [ -n "$LATEST_ADV" ] && [ -d "$LATEST_ADV" ]; then
            cp -r "$LATEST_ADV"/* "$EXP_DIR/" 2>/dev/null || true
        fi
    done
done

set -e

# Final cleanup
cleanup_nodes

echo ""
echo "============================================================" | tee -a "$SUMMARY"
echo "  ALL EXPERIMENTS COMPLETE"                                    | tee -a "$SUMMARY"
echo "  Finished: $(date)"                                           | tee -a "$SUMMARY"
echo "============================================================" | tee -a "$SUMMARY"

echo ""
echo "Comparison table (CSV): $CSV"
echo ""
# Print CSV as formatted table
echo "--- Results Summary ---"
column -t -s',' "$CSV" 2>/dev/null || cat "$CSV"

echo ""
echo "Full results: $RESULTS_DIR"
echo "Summary log:  $SUMMARY"
