#!/bin/bash

set -e

OUTPUT="${1:-metrics.json}"
NODES="${2:-4}"

echo "=========================================="
echo "Collecting Metrics"
echo "=========================================="
echo "Output: $OUTPUT"
echo "Nodes: $NODES"
echo "=========================================="

METRICS="{
  \"timestamp\": \"$(date -Iseconds)\",
  \"nodes\": ["

for i in $(seq 0 $((NODES - 1)); do
    NODE_PORT=$((18000 + i * 10))
    
    echo "Collecting metrics from node $i (port $NODE_PORT)..."
    
    NODE_METRICS=$(curl -s "http://localhost:${NODE_PORT}/metrics" 2>/dev/null || echo "")
    
    TX_COUNT=$(echo "$NODE_METRICS" | grep "resdb_transactions_total" | awk '{print $2}' || echo "0")
    TX_LATENCY_P50=$(echo "$NODE_METRICS" | grep "resdb_tx_latency_bucket{le=\"1\"" | awk '{print $2}' || echo "0")
    CHECKPOINT=$(echo "$NODE_METRICS" | grep "resdb_last_checkpoint" | awk '{print $2}' || echo "0")
    
    if [ $i -gt 0 ]; then
        METRICS+=","
    fi
    
    METRICS+="
    {
      \"node_id\": $i,
      \"port\": $NODE_PORT,
      \"tx_count\": $TX_COUNT,
      \"tx_latency_p50\": $TX_LATENCY_P50,
      \"checkpoint\": $CHECKPOINT
    }"
done

METRICS+="
  ],
  \"ipfs\": ["

for i in $(seq 0 $((NODES - 1))); do
    IPFS_PORT=$((5001 + i * 100))
    
    echo "Collecting IPFS metrics from node $i (port $IPFS_PORT)..."
    
    IPFS_STATS=$(curl -s "http://localhost:${IPFS_PORT}/api/v0/repo/stat" 2>/dev/null || echo "{}")
    
    if [ $i -gt 0 ]; then
        METRICS+=","
    fi
    
    METRICS+="
    {
      \"node_id\": $i,
      \"port\": $IPFS_PORT,
      \"stats\": $IPFS_STATS
    }"
done

METRICS+="
  ]
}"

echo "$METRICS" > "$OUTPUT"

echo ""
echo "=========================================="
echo "Metrics collected!"
echo "=========================================="
echo "Output: $OUTPUT"
echo "=========================================="