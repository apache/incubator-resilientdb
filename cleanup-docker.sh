#!/bin/bash
# Clean up Docker to free disk space

set -e

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "🧹 Cleaning Up Docker to Free Disk Space"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

# Step 1: Check current disk space
echo "Step 1: Checking disk space..."
df -h | grep -E "Filesystem|/$" || df -h | head -2
echo ""

# Step 2: Stop running containers
echo "Step 2: Stopping running containers..."
cd /Users/sandhya/UCD/ECS_DDS/graphq-llm
docker-compose -f docker-compose.dev.yml down 2>/dev/null || true
echo "✅ Containers stopped"
echo ""

# Step 3: Remove unused containers
echo "Step 3: Removing unused containers..."
docker container prune -f
echo "✅ Unused containers removed"
echo ""

# Step 4: Remove unused images
echo "Step 4: Removing unused images..."
docker image prune -a -f
echo "✅ Unused images removed"
echo ""

# Step 5: Remove unused volumes (be careful - this removes data!)
echo "Step 5: Removing unused volumes..."
echo "⚠️  WARNING: This will remove unused volumes (data will be lost)"
read -p "Continue? (y/N): " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    docker volume prune -f
    echo "✅ Unused volumes removed"
else
    echo "⏭️  Skipping volume cleanup"
fi
echo ""

# Step 6: Remove build cache
echo "Step 6: Removing build cache..."
docker builder prune -a -f
echo "✅ Build cache removed"
echo ""

# Step 7: Check disk space again
echo "Step 7: Checking disk space after cleanup..."
df -h | grep -E "Filesystem|/$" || df -h | head -2
echo ""

# Step 8: Show Docker disk usage
echo "Step 8: Docker disk usage:"
docker system df
echo ""

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "✅ Docker cleanup complete!"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "You can now try building again:"
echo "  docker-compose -f docker-compose.dev.yml build graphq-llm-backend"
echo ""

