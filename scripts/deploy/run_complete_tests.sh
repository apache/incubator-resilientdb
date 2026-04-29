#!/bin/bash
# Complete test suite: normal + crash_f + gst_ramp for all 12 protocols
# With circuit breaker network fix, 32 nodes (16 replicas + 16 clients)
set +e
cd /root/code_dev/asf-resilientdb/scripts/deploy

export DEPLOY_DELAY_MS=0
export READY_TIMEOUT=120

PROTOCOLS="pbft hs damysus achilles tusk bullshark mysticeti shoalpp rcc fides sync_fides hybridset"
IP_CONF="config/kv_server.conf"
RESULT_DIR="final_results_$(date +%Y%m%d_%H%M%S)"
mkdir -p "$RESULT_DIR"
LOG="$RESULT_DIR/master.log"

log() { echo "[$(date +%H:%M:%S)] $*" | tee -a "$LOG"; }

cleanup_all() {
  for ip in $(grep "^128" $IP_CONF); do
    ssh -p 2222 -i ~/.ssh/id_rsa -o ConnectTimeout=3 -o StrictHostKeyChecking=no -o BatchMode=yes root@${ip} \
      "killall -9 kv_server_performance 2>/dev/null; rm -f /root/core.* /root/kv_server_performance.log" 2>/dev/null &
    ssh -p 22 -i ~/.ssh/id_rsa -o ConnectTimeout=3 -o StrictHostKeyChecking=no -o BatchMode=yes Shaokang@${ip} \
      'DEV=$(ip -4 route show default | awk "{print \$5}" | head -1) && sudo tc qdisc del dev $DEV root 2>/dev/null' 2>/dev/null &
  done
  wait 2>/dev/null
  pkill -9 -f "kv_service_tools" 2>/dev/null
  sleep 2
}

run_test() {
  local proto=$1
  local scenario=$2
  local outfile="$RESULT_DIR/${proto}_${scenario}.log"
  local script
  # Per-protocol timeout: 8 min for normal, 10 min for adversarial
  local test_timeout=480
  [ "$scenario" != "normal" ] && test_timeout=600

  if [ "$scenario" = "normal" ]; then
    script="./performance/${proto}_performance.sh"
    [ -f "$script" ] || return
    log "=== $scenario: $proto ==="
    cleanup_all
    timeout $test_timeout $script $IP_CONF > "$outfile" 2>&1
    local rc=$?
    if [ $rc -eq 124 ]; then
      log "  TIMEOUT after ${test_timeout}s"
    fi
  else
    script="./performance/${proto}_adversarial.sh"
    [ -f "$script" ] || return
    local phases_name=$(echo $scenario | sed 's/_f$//' | sed 's/_ramp$//' | sed 's/_nodelay$//')
    local phases_file="adversarial/phases/${phases_name}_phases.json"
    [ -f "$phases_file" ] || phases_file=""
    log "=== $scenario: $proto ==="
    cleanup_all
    timeout $test_timeout $script $IP_CONF adversarial/scenarios/${scenario}.json $phases_file > "$outfile" 2>&1
    local rc=$?
    if [ $rc -eq 124 ]; then
      log "  TIMEOUT after ${test_timeout}s"
    fi
  fi
  cleanup_all

  local result=$(grep -a "max throughput:" "$outfile" 2>/dev/null | grep -v "throughput:0 " | head -1)
  [ -z "$result" ] && result=$(grep -a "max throughput:" "$outfile" 2>/dev/null | tail -1)
  log "  $result"
}

log "Starting complete test suite"
log "Config: $(grep -c '^128' $IP_CONF) IPs, $(grep 'client_num' $IP_CONF)"
log ""

# Phase 1: Normal
log "========== PHASE 1: Normal Performance =========="
for proto in $PROTOCOLS; do
  run_test "$proto" "normal"
done

# Phase 2: Crash-f (no delay version for cleaner results)
log ""
log "========== PHASE 2: Crash-f =========="
for proto in $PROTOCOLS; do
  run_test "$proto" "crash_f_nodelay"
done

# Phase 3: GST Ramp
log ""
log "========== PHASE 3: GST Ramp =========="
for proto in $PROTOCOLS; do
  run_test "$proto" "gst_ramp"
done

# Summary
log ""
log "=========================================="
log "COMPLETE"
log "=========================================="
log "All tests done at $(date)"
log "Results in: $RESULT_DIR"
