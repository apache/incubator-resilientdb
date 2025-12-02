#!/bin/bash
# Aggressive Docker cleanup - removes everything except running containers

set -e

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "🔥 Aggressive Docker Cleanup"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "⚠️  WARNING: This will remove:"
echo "   - All stopped containers"
echo "   - All unused images (not just dangling)"
echo "   - All unused volumes"
echo "   - All build cache"
echo ""
read -p "Are you sure? (yes/no): " -r
if [[ ! $REPLY =~ ^[Yy][Ee][Ss]$ ]]; then
    echo "Cancelled."
    exit 1
fi
echo ""

# Step 1: Stop all containers
echo "Step 1: Stopping all containers..."
cd /Users/sandhya/UCD/ECS_DDS/graphq-llm
docker-compose -f docker-compose.dev.yml down 2>/dev/null || true
docker stop $(docker ps -aq) 2>/dev/null || true
echo "✅ All containers stopped"
echo ""

# Step 2: Remove all containers
echo "Step 2: Removing all containers..."
docker rm $(docker ps -aq) 2>/dev/null || true
echo "✅ All containers removed"
echo ""

# Step 3: Remove all images
echo "Step 3: Removing all images..."
docker rmi $(docker images -aq) 2>/dev/null || true
echo "✅ All images removed"
echo ""

# Step 4: Remove all volumes
echo "Step 4: Removing all volumes..."
docker volume rm $(docker volume ls -q) 2>/dev/null || true
echo "✅ All volumes removed"
echo ""

# Step 5: Prune everything
echo "Step 5: Pruning system..."
docker system prune -a --volumes -f
echo "✅ System pruned"
echo ""

# Step 6: Check disk space
echo "Step 6: Checking disk space..."
df -h | grep -E "Filesystem|/$" || df -h | head -2
echo ""

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "✅ Aggressive cleanup complete!"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "You'll need to rebuild all images:"
echo "  docker-compose -f docker-compose.dev.yml build"
echo ""

