#!/bin/bash

set -e

TPS="${1:-100}"
DURATION="${2:-60}"
PAYLOAD_SIZE="${3:-1024}"
NODE_HOST="${4:-localhost}"
NODE_PORT="${5:-18001}"

echo "=========================================="
echo "ResilientDB Benchmark"
echo "=========================================="
echo "TPS: $TPS"
echo "Duration: ${DURATION}s"
echo "Payload Size: ${PAYLOAD_SIZE}B"
echo "Target: ${NODE_HOST}:${NODE_PORT}"
echo "=========================================="

END_TIME=$(date -d "+${DURATION} seconds" +%s)
REQUEST_COUNT=0
SUCCESS_COUNT=0
FAIL_COUNT=0

echo "Starting benchmark..."

while [ $(date +%s) -lt $END_TIME ]; do
    TIMESTAMP=$(date +%s%N)
    KEY="user_$((RANDOM % 10000))"
    VALUE="value_${TIMESTAMP}_$(head -c $PAYLOAD_SIZE < /dev/urandom | base64)"
    
    RESPONSE=$(curl -s -w "\n%{http_code}" \
        -X POST "http://${NODE_HOST}:${NODE_PORT}/kv" \
        -H "Content-Type: application/json" \
        -d "{\"key\": \"$KEY\", \"value\": \"$VALUE\"}" 2>/dev/null || echo "000")
    
    HTTP_CODE=$(echo "$RESPONSE" | tail -1)
    
    if [ "$HTTP_CODE" = "200" ]; then
        ((SUCCESS_COUNT++))
    else
        ((FAIL_COUNT++))
    fi
    
    ((REQUEST_COUNT++))
    
    if [ $((REQUEST_COUNT % 100)) -eq 0 ]; then
        ELAPSED=$((DURATION - (END_TIME - $(date +%s)))
        CURRENT_TPS=$((REQUEST_COUNT / (DURATION - ELAPSED + 1)))
        echo "[$(date +%T)] Requests: $REQUEST_COUNT, Success: $SUCCESS_COUNT, Failed: $FAIL_COUNT, Current TPS: $CURRENT_TPS"
    fi
    
    sleep "$(echo "scale=6; 1/$TPS" | bc)"
done

echo ""
echo "=========================================="
echo "Benchmark Complete!"
echo "=========================================="
echo "Total Requests: $REQUEST_COUNT"
echo "Successful: $SUCCESS_COUNT"
echo "Failed: $FAIL_COUNT"
echo "Actual TPS: $(echo "scale=2; $REQUEST_COUNT/$DURATION" | bc)"
echo "=========================================="