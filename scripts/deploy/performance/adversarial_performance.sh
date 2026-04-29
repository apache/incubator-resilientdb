#!/bin/bash
#
# Adversarial Performance Test Runner
#
# Usage:
#   ./performance/adversarial_performance.sh <ip_conf> <scenario_json> [phases_json]
#
# Example:
#   export server=//benchmark/protocols/fides:kv_server_performance
#   export TEMPLATE_PATH=$PWD/config/fides.config
#   ./performance/adversarial_performance.sh config/kv_performance_server_16.conf adversarial/scenarios/gst_ramp.json
#
# Environment variables:
#   server          - Bazel target for the protocol binary
#   TEMPLATE_PATH   - Protocol config file path
#   DEPLOY_DELAY_MS - Initial delay (default: 40)
#   DEPLOY_JITTER_MS - Initial jitter (default: 0)

set -e

if [ $# -lt 2 ]; then
    echo "Usage: $0 <ip_conf> <scenario_json> [phases_json]"
    echo ""
    echo "Arguments:"
    echo "  ip_conf       - IP configuration file (e.g., config/kv_performance_server_16.conf)"
    echo "  scenario_json - Adversarial scenario file (e.g., adversarial/scenarios/gst_ramp.json)"
    echo "  phases_json   - (Optional) Phase definitions for result analysis"
    exit 1
fi

IP_CONF=$1
SCENARIO=$2
PHASES_JSON=${3:-""}

# Load environment
. ./script/env.sh
. ./script/load_config.sh $IP_CONF

server_name=$(echo "$server" | awk -F':' '{print $NF}')
server_bin=${server_name}

echo "============================================"
echo "Adversarial Performance Test"
echo "============================================"
echo "Protocol binary: $server"
echo "Config template: $TEMPLATE_PATH"
echo "IP config: $IP_CONF"
echo "Scenario: $SCENARIO"
echo "Nodes: ${#iplist[@]}"
echo "Clients: $client_num"
echo "============================================"

# Step 0: Clean up stale bazel processes
echo "[Step 0] Cleaning up stale bazel processes..."
# First remove stale PID files (must happen BEFORE bazel shutdown, which hangs on zombies)
find ~/.cache/bazel -name "server.pid.txt" -exec sh -c '
  pid=$(cat "$1" 2>/dev/null)
  if [ -n "$pid" ] && ! kill -0 "$pid" 2>/dev/null; then
    echo "  Removing stale PID file: $1 (pid=$pid)"
    rm -f "$1"
  fi
' _ {} \; 2>/dev/null || true
# Then shutdown with a timeout to avoid hanging
timeout 5 bazel shutdown 2>/dev/null || true

# Step 1: Kill any existing servers
echo "[Step 1] Killing existing servers..."
for ip in ${iplist[@]}; do
    ssh $ssh_options_cloud -p 2222 -i ${key} -n -o BatchMode=yes -o StrictHostKeyChecking=no root@${ip} "killall -9 ${server_bin}" 2>/dev/null &
done
wait

# Step 2: Deploy
echo "[Step 2] Deploying..."
set +e
./script/deploy.sh $IP_CONF
DEPLOY_EXIT=$?
set -e
if [ $DEPLOY_EXIT -ne 0 ]; then
    echo "ERROR: Deployment failed (exit code: $DEPLOY_EXIT)!"
    exit 1
fi

# Step 2.5: Check tc availability (with 30s timeout per node to avoid hanging on broken apt)
echo "[Step 2.5] Checking tc availability on remote nodes..."
for ip in ${iplist[@]}; do
    timeout 30 ssh $ssh_options_cloud -p 2222 -i ${key} -n -o BatchMode=yes -o StrictHostKeyChecking=no root@${ip} \
        "which tc >/dev/null 2>&1 || echo 'WARN: tc not available on ${ip}'" &
done
wait

# Step 3: Start clients (with 30s timeout per client to avoid hanging on unreachable nodes)
echo "[Step 3] Starting clients..."
. ./script/load_config.sh $IP_CONF

[ -f "${BAZEL_WORKSPACE_PATH}/bazel-bin/benchmark/protocols/pbft/kv_service_tools" ] || bazel build //benchmark/protocols/pbft:kv_service_tools
${BAZEL_WORKSPACE_PATH}/bazel-bin/benchmark/protocols/pbft/kv_service_tools

for((i=1;;i++)); do
    config_file=$PWD/config_out/client${i}.config
    if [ ! -f "$config_file" ]; then
        break
    fi
    echo "Starting client with config: $config_file"
    timeout 30 ${BAZEL_WORKSPACE_PATH}/bazel-bin/benchmark/protocols/pbft/kv_service_tools $config_file || echo "client $i timed out"
done

# Step 4: Run adversarial scenario (in background)
echo "[Step 4] Running adversarial scenario..."
SCENARIO_DURATION=$(python3 -c "import json; print(json.load(open('$SCENARIO'))['duration_s'])")
echo "Scenario duration: ${SCENARIO_DURATION}s"

python3 adversarial/adversarial_scheduler.py \
    --scenario "$SCENARIO" \
    --ip-conf "$IP_CONF" \
    --key "${key}" \
    --server-bin "${server_bin}" \
    --ssh-port 2222 \
    --tc-port 22 \
    --tc-user Shaokang &
SCHEDULER_PID=$!

# Wait for scenario to complete
wait $SCHEDULER_PID
echo "Adversarial scenario complete."

# Step 5: Cleanup tc rules on host
echo "[Step 5] Cleaning up network rules..."
for ip in ${iplist[@]}; do
    ssh $ssh_options_cloud -p 22 -i ${key} -n -o BatchMode=yes -o StrictHostKeyChecking=no Shaokang@${ip} "DEV=\$(ip -4 route show default | awk '{print \$5}' | head -1) && sudo tc qdisc del dev \$DEV root 2>/dev/null" &
done
wait

# Step 6: Stop servers
echo "[Step 6] Stopping servers..."
for ip in ${iplist[@]}; do
    ssh $ssh_options_cloud -p 2222 -i ${key} -n -o BatchMode=yes -o StrictHostKeyChecking=no root@${ip} "killall -9 ${server_bin}" 2>/dev/null &
done
wait

# Step 7: Collect results
echo "[Step 7] Collecting results..."
RESULT_DIR="adversarial_results/$(basename $SCENARIO .json)_$(date +%Y%m%d_%H%M%S)"
mkdir -p "$RESULT_DIR"

for ip in ${iplist[@]}; do
    scp $ssh_options_cloud -P 2222 -i ${key} root@${ip}:/root/${server_bin}.log "${RESULT_DIR}/result_${ip}_log" 2>/dev/null &
done
wait

# Step 8: Analyze results
echo "[Step 8] Analyzing results..."
set +e

# Standard analysis
python3 performance/calculate_result.py ${RESULT_DIR}/result_*_log > "${RESULT_DIR}/results_standard.log" 2>&1
echo "Standard results:"
cat "${RESULT_DIR}/results_standard.log"

# Adversarial analysis with time-series CSV
ANALYSIS_ARGS="--csv ${RESULT_DIR}/timeseries.csv"
if [ -n "$PHASES_JSON" ] && [ -f "$PHASES_JSON" ]; then
    ANALYSIS_ARGS="$ANALYSIS_ARGS --phases $PHASES_JSON"
fi
python3 performance/calculate_adversarial_result.py $ANALYSIS_ARGS ${RESULT_DIR}/result_*_log > "${RESULT_DIR}/results_adversarial.log" 2>&1
echo ""
echo "Adversarial results:"
cat "${RESULT_DIR}/results_adversarial.log"
set -e

# Save scenario metadata
cp "$SCENARIO" "${RESULT_DIR}/scenario.json"
if [ -n "$PHASES_JSON" ]; then
    cp "$PHASES_JSON" "${RESULT_DIR}/phases.json"
fi

echo ""
echo "============================================"
echo "Results saved to: $RESULT_DIR"
echo "  - results_standard.log    (standard analysis)"
echo "  - results_adversarial.log (phase-aware analysis)"
echo "  - timeseries.csv          (for plotting)"
echo "  - result_*_log            (raw replica logs)"
echo "============================================"
