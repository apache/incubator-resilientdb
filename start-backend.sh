#!/bin/bash
# Start GraphQ-LLM Backend and ResilientDB

set -e

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "🚀 Starting GraphQ-LLM Backend"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

cd /Users/sandhya/UCD/ECS_DDS/graphq-llm

# Step 1: Check if backend is already running
echo "Step 1: Checking if backend is already running..."
if curl -s http://localhost:3001/health > /dev/null 2>&1; then
    echo "✅ Backend is already running"
    exit 0
fi
echo ""

# Step 2: Start ResilientDB first
echo "Step 2: Starting ResilientDB..."
docker-compose -f docker-compose.dev.yml up -d resilientdb

echo "Waiting for ResilientDB to start (60 seconds)..."
sleep 60

# Verify ResilientDB
echo "Verifying ResilientDB..."
if curl -s http://localhost:18000/v1/transactions/test > /dev/null 2>&1; then
    echo "✅ ResilientDB KV service is accessible"
else
    echo "⚠️  ResilientDB KV service may still be starting"
fi

# Check GraphQL server
sleep 10
if curl -s -X POST http://localhost:5001/graphql \
    -H "Content-Type: application/json" \
    -d '{"query": "{ __typename }"}' | grep -q "__typename"; then
    echo "✅ ResilientDB GraphQL server is running"
else
    echo "⚠️  GraphQL server may still be starting"
fi
echo ""

# Step 3: Start GraphQ-LLM Backend
echo "Step 3: Starting GraphQ-LLM Backend..."
docker-compose -f docker-compose.dev.yml up -d graphq-llm-backend

echo "Waiting for backend to start (30 seconds)..."
sleep 30

# Step 4: Verify backend
echo "Step 4: Verifying backend..."
for i in {1..20}; do
    if curl -s http://localhost:3001/health | grep -q "ok"; then
        echo "✅ GraphQ-LLM Backend is now running"
        echo ""
        echo "📍 Backend URL: http://localhost:3001"
        echo "📍 Health Check: http://localhost:3001/health"
        echo ""
        echo "🧪 Test: curl http://localhost:3001/health"
        exit 0
    fi
    if [ $i -eq 20 ]; then
        echo "❌ Backend failed to start"
        echo ""
        echo "📋 Check logs:"
        echo "   docker-compose -f docker-compose.dev.yml logs graphq-llm-backend"
        echo ""
        echo "🔧 Troubleshooting:"
        echo "   1. Check if port 3001 is in use: lsof -i:3001"
        echo "   2. Check Docker containers: docker-compose -f docker-compose.dev.yml ps"
        echo "   3. View logs: docker-compose -f docker-compose.dev.yml logs graphq-llm-backend"
        exit 1
    fi
    sleep 3
done

