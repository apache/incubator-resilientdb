#!/bin/bash
# Fix Docker backend npm install issue

set -e

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "🔧 Fixing Docker Backend npm Issue"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

cd /Users/sandhya/UCD/ECS_DDS/graphq-llm

# Step 1: Stop existing containers
echo "Step 1: Stopping existing containers..."
docker-compose -f docker-compose.dev.yml down
echo "✅ Containers stopped"
echo ""

# Step 2: Remove problematic volumes
echo "Step 2: Cleaning up volumes..."
docker volume rm graphq-llm-node-modules 2>/dev/null || true
echo "✅ Volumes cleaned"
echo ""

# Step 3: Rebuild containers
echo "Step 3: Rebuilding containers..."
docker-compose -f docker-compose.dev.yml build --no-cache graphq-llm-backend
echo "✅ Containers rebuilt"
echo ""

# Step 4: Start services
echo "Step 4: Starting services..."
docker-compose -f docker-compose.dev.yml up -d resilientdb
echo "Waiting for ResilientDB (60 seconds)..."
sleep 60

docker-compose -f docker-compose.dev.yml up -d graphq-llm-backend
echo "Waiting for backend (30 seconds)..."
sleep 30
echo ""

# Step 5: Verify
echo "Step 5: Verifying backend..."
for i in {1..20}; do
    if curl -s http://localhost:3001/health | grep -q "ok"; then
        echo "✅ Backend is running!"
        echo ""
        echo "📍 Backend URL: http://localhost:3001"
        exit 0
    fi
    if [ $i -eq 20 ]; then
        echo "❌ Backend failed to start"
        echo ""
        echo "📋 Check logs:"
        docker-compose -f docker-compose.dev.yml logs graphq-llm-backend | tail -30
        exit 1
    fi
    sleep 3
done

