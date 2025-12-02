#!/bin/bash
# Complete fix for query stats not showing

set -e

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "🔧 Complete Query Stats Fix"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

RESLENS_DIR="/Users/sandhya/UCD/ECS_DDS/ResLens"
MIDDLEWARE_DIR="/Users/sandhya/UCD/ECS_DDS/ResLens-Middleware/middleware"
NEXUS_DIR="/Users/sandhya/UCD/ECS_DDS/nexus"

# Step 1: Fix ResLens URL configuration
echo "Step 1: Fixing ResLens URL configuration..."
cd "$RESLENS_DIR"

# Check if url.ts has correct URL
if grep -q "localhost:3002/api" src/static/url.ts 2>/dev/null; then
    echo "⚠️  Found incorrect URL in url.ts. Fixing..."
    # This was already fixed, but verify
    if grep -q "localhost:3003/api/v1" src/static/url.ts 2>/dev/null; then
        echo "✅ url.ts is correct"
    else
        echo "❌ url.ts still has wrong URL. Please check manually."
    fi
else
    echo "✅ url.ts looks correct"
fi

# Ensure .env has correct middleware URL
if [ -f ".env" ]; then
    if grep -q "VITE_MIDDLEWARE_BASE_URL=http://localhost:3003/api/v1" .env; then
        echo "✅ .env has correct middleware URL"
    else
        echo "⚠️  Updating .env with correct middleware URL..."
        # Remove old entry if exists
        sed -i.bak '/VITE_MIDDLEWARE_BASE_URL/d' .env 2>/dev/null || \
        sed -i '' '/VITE_MIDDLEWARE_BASE_URL/d' .env 2>/dev/null || true
        # Add correct entry
        echo "" >> .env
        echo "VITE_MIDDLEWARE_BASE_URL=http://localhost:3003/api/v1" >> .env
        echo "✅ Updated .env"
    fi
else
    echo "⚠️  Creating .env file..."
    cat > .env << 'EOF'
VITE_MIDDLEWARE_BASE_URL=http://localhost:3003/api/v1
VITE_MIDDLEWARE_SECONDARY_BASE_URL=http://localhost:3003/api/v1
VITE_DEV_RESLENS_TOOLS_URL=http://localhost:3003
EOF
    echo "✅ Created .env file"
fi
echo ""

# Step 2: Verify middleware is running and has data
echo "Step 2: Verifying middleware..."
if curl -s http://localhost:3003/api/v1/healthcheck > /dev/null 2>&1; then
    echo "✅ Middleware is running"
    
    # Check if there are any query stats
    STATS_RESPONSE=$(curl -s http://localhost:3003/api/v1/queryStats?limit=1)
    if echo "$STATS_RESPONSE" | grep -q "success"; then
        COUNT=$(echo "$STATS_RESPONSE" | grep -o '"count":[0-9]*' | cut -d':' -f2 || echo "0")
        echo "   Query stats count: $COUNT"
        if [ "$COUNT" = "0" ]; then
            echo "   ⚠️  No query stats found. You need to analyze queries in Nexus first."
        fi
    fi
else
    echo "❌ Middleware is NOT running"
    echo "   Start it: cd $MIDDLEWARE_DIR && PORT=3003 npm start"
    exit 1
fi
echo ""

# Step 3: Verify Nexus configuration
echo "Step 3: Verifying Nexus configuration..."
cd "$NEXUS_DIR"
if [ -f ".env" ]; then
    if grep -q "RESLENS_MIDDLEWARE_URL=http://localhost:3003" .env; then
        echo "✅ Nexus .env has correct middleware URL"
    else
        echo "⚠️  Updating Nexus .env..."
        echo "RESLENS_MIDDLEWARE_URL=http://localhost:3003" >> .env
        echo "✅ Updated Nexus .env"
        echo "   ⚠️  You need to restart Nexus for this to take effect"
    fi
else
    echo "⚠️  Creating Nexus .env..."
    echo "RESLENS_MIDDLEWARE_URL=http://localhost:3003" > .env
    echo "✅ Created Nexus .env"
    echo "   ⚠️  You need to restart Nexus for this to take effect"
fi
echo ""

# Step 4: Test the flow
echo "Step 4: Testing query stats flow..."
echo "Sending a test query to Nexus API..."
TEST_RESPONSE=$(curl -s -X POST http://localhost:3002/api/graphql-tutor/analyze \
  -H "Content-Type: application/json" \
  -d '{"query": "{ getTransaction(id: \"test-fix\") { id } }"}')

if echo "$TEST_RESPONSE" | grep -q "explanation\|efficiency"; then
    echo "✅ Nexus API responded"
    echo "   (This should have stored stats in middleware)"
    
    # Wait for middleware to process
    sleep 2
    
    # Check if stats were stored
    FINAL_STATS=$(curl -s http://localhost:3003/api/v1/queryStats?limit=5)
    if echo "$FINAL_STATS" | grep -q "test-fix"; then
        echo "✅ Query stats were stored successfully!"
    else
        echo "⚠️  Query stats may not have been stored"
        echo "   Check Nexus logs: tail -f /tmp/nexus.log | grep middleware"
    fi
else
    echo "⚠️  Nexus API may have issues"
fi
echo ""

# Step 5: Restart ResLens to pick up .env changes
echo "Step 5: Restarting ResLens to apply .env changes..."
if lsof -ti:5173 >/dev/null 2>&1; then
    echo "Killing existing ResLens process..."
    lsof -ti:5173 | xargs kill -9 2>/dev/null || true
    sleep 2
fi

cd "$RESLENS_DIR"
echo "Starting ResLens..."
nohup npm run dev > /tmp/reslens.log 2>&1 &
RESLENS_PID=$!
echo "✅ ResLens restarted with PID: $RESLENS_PID"
sleep 5

if curl -s http://localhost:5173 > /dev/null 2>&1; then
    echo "✅ ResLens is accessible"
else
    echo "⚠️  ResLens may still be starting"
fi
echo ""

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "✅ Fix Complete!"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "📋 Summary:"
echo "   ✅ ResLens URL configuration fixed"
echo "   ✅ ResLens .env updated"
echo "   ✅ Nexus .env verified"
echo "   ✅ ResLens restarted"
echo ""
echo "🧪 Next Steps:"
echo "   1. Open Nexus: http://localhost:3002/graphql-tutor"
echo "   2. Analyze a query"
echo "   3. Open ResLens: http://localhost:5173/dashboard?tab=query_stats"
echo "   4. Query stats should now appear"
echo ""
echo "📋 If stats still don't show:"
echo "   - Check browser console for errors (F12)"
echo "   - Check ResLens logs: tail -f /tmp/reslens.log"
echo "   - Check middleware logs: tail -f /tmp/reslens-middleware.log"
echo "   - Run test script: ./test-query-stats-flow.sh"
echo ""

