#!/bin/bash
#
# Run all adversarial experiments for a given protocol.
#
# Usage:
#   ./adversarial/run_all_adversarial.sh <protocol> <ip_conf>
#
# Example:
#   ./adversarial/run_all_adversarial.sh fides config/kv_performance_server_16.conf
#
# Protocols: fides, damysus, achilles

set -e

if [ $# -lt 2 ]; then
    echo "Usage: $0 <protocol> <ip_conf>"
    echo "Protocols: fides, damysus, achilles"
    exit 1
fi

PROTOCOL=$1
IP_CONF=$2
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
DEPLOY_DIR="$(dirname "$SCRIPT_DIR")"
SCENARIO_DIR="${SCRIPT_DIR}/scenarios"
PHASES_DIR="${SCRIPT_DIR}/phases"

cd "$DEPLOY_DIR"

echo "============================================"
echo "Running all adversarial experiments"
echo "Protocol: $PROTOCOL"
echo "IP Config: $IP_CONF"
echo "============================================"

# Define experiments to run
declare -A SCENARIOS
SCENARIOS["crash_f"]="${SCENARIO_DIR}/crash_f.json"
SCENARIOS["crash_incremental"]="${SCENARIO_DIR}/crash_incremental.json"
SCENARIOS["partition_majority"]="${SCENARIO_DIR}/partition_majority.json"
SCENARIOS["partition_cycle"]="${SCENARIO_DIR}/partition_cycle.json"
SCENARIOS["gst_ramp"]="${SCENARIO_DIR}/gst_ramp.json"
SCENARIOS["gst_sudden"]="${SCENARIO_DIR}/gst_sudden.json"
SCENARIOS["high_jitter"]="${SCENARIO_DIR}/high_jitter.json"
SCENARIOS["leader_targeted"]="${SCENARIO_DIR}/leader_targeted.json"
SCENARIOS["single_node_isolation"]="${SCENARIO_DIR}/single_node_isolation.json"

declare -A PHASES
PHASES["crash_f"]="${PHASES_DIR}/crash_phases.json"
PHASES["crash_incremental"]="${PHASES_DIR}/crash_phases.json"
PHASES["partition_majority"]="${PHASES_DIR}/partition_phases.json"
PHASES["partition_cycle"]="${PHASES_DIR}/partition_phases.json"
PHASES["gst_ramp"]="${PHASES_DIR}/gst_phases.json"
PHASES["gst_sudden"]="${PHASES_DIR}/gst_phases.json"

PERF_SCRIPT="./performance/${PROTOCOL}_adversarial.sh"
if [ ! -f "$PERF_SCRIPT" ]; then
    echo "ERROR: Performance script not found: $PERF_SCRIPT"
    echo "Available scripts:"
    ls ./performance/*_adversarial.sh 2>/dev/null || echo "  (none)"
    exit 1
fi

RESULTS_SUMMARY="adversarial_results/${PROTOCOL}_summary_$(date +%Y%m%d_%H%M%S).txt"
mkdir -p adversarial_results

echo "Protocol: $PROTOCOL" > "$RESULTS_SUMMARY"
echo "IP Config: $IP_CONF" >> "$RESULTS_SUMMARY"
echo "Date: $(date)" >> "$RESULTS_SUMMARY"
echo "" >> "$RESULTS_SUMMARY"

TOTAL=${#SCENARIOS[@]}
COUNT=0

for scenario_name in "${!SCENARIOS[@]}"; do
    COUNT=$((COUNT + 1))
    scenario_file="${SCENARIOS[$scenario_name]}"
    phases_file="${PHASES[$scenario_name]:-}"

    echo ""
    echo "============================================"
    echo "[$COUNT/$TOTAL] Running: $scenario_name"
    echo "============================================"

    if [ ! -f "$scenario_file" ]; then
        echo "  SKIP: Scenario file not found: $scenario_file"
        continue
    fi

    ARGS="$IP_CONF $scenario_file"
    if [ -n "$phases_file" ] && [ -f "$phases_file" ]; then
        ARGS="$ARGS $phases_file"
    fi

    echo ">> $PERF_SCRIPT $ARGS"

    if $PERF_SCRIPT $ARGS; then
        echo "  PASSED: $scenario_name" | tee -a "$RESULTS_SUMMARY"
    else
        echo "  FAILED: $scenario_name (exit code: $?)" | tee -a "$RESULTS_SUMMARY"
    fi

    # Brief pause between experiments for cleanup
    sleep 5
done

echo ""
echo "============================================"
echo "All experiments complete!"
echo "Summary: $RESULTS_SUMMARY"
echo "============================================"
cat "$RESULTS_SUMMARY"
