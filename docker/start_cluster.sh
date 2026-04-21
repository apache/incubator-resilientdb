#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

RESDB_IMAGE="${RESDB_IMAGE:-resilientdb:latest}"
IPFS_IMAGE="${IPFS_IMAGE:-ipfs/kubo:latest}"
STORAGE_BACKEND="${STORAGE_BACKEND:-LEVELDB_ONLY}"
TIERED_ENABLED="${TIERED_ENABLED:-false}"
COLD_THRESHOLD="${COLD_THRESHOLD:-2}"
NODES="${1:-4}"

echo "=========================================="
echo "ResilientDB Cluster Startup"
echo "=========================================="
echo "Nodes: $NODES"
echo "Storage Backend: $STORAGE_BACKEND"
echo "Tiered Enabled: $TIERED_ENABLED"
echo "Cold Threshold: $COLD_THRESHOLD"
echo "=========================================="

cd "$SCRIPT_DIR"

export RESDB_IMAGE
export IPFS_IMAGE
export STORAGE_BACKEND
export TIERED_ENABLED
export COLD_THRESHOLD

echo "Starting Docker Compose..."
docker-compose up -d

echo "Waiting for services to be ready..."
sleep 10

echo ""
echo "=========================================="
echo "Cluster started successfully!"
echo "=========================================="
echo ""
echo "Services:"
echo "  resilientdb-0: http://localhost:18000 (gRPC: 18001)"
echo "  resilientdb-1: http://localhost:18010 (gRPC: 18011)"
echo "  resilientdb-2: http://localhost:18020 (gRPC: 18021)"
echo "  resilientdb-3: http://localhost:18030 (gRPC: 18031)"
echo "  ipfs-0:      http://localhost:5001"
echo "  ipfs-1:      http://localhost:5011"
echo "  ipfs-2:      http://localhost:5021"
echo "  ipfs-3:      http://localhost:5031"
echo "  prometheus:   http://localhost:9090"
echo ""
echo "To enable tiered storage:"
echo "  STORAGE_BACKEND=TIERED TIERED_ENABLED=true ./start_cluster.sh"
echo ""
echo "To stop:"
echo "  docker-compose down"
echo "=========================================="