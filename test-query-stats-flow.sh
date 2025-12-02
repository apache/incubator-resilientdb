#!/bin/bash
# Test the complete query stats flow

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "🧪 Testing Query Stats Flow"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

MIDDLEWARE_URL="http://localhost:3003"
NEXUS_URL="http://localhost:3002"
BACKEND_URL="http://localhost:3001"

# Step 1: Check if all services are running
echo "Step 1: Checking services..."
echo ""

echo "Checking GraphQ-LLM Backend..."
if curl -s "${BACKEND_URL}/health" | grep -q "ok"; then
    echo "✅ Backend is running"
else
    echo "❌ Backend is NOT running"
    exit 1
fi

echo "Checking ResLens Middleware..."
if curl -s "${MIDDLEWARE_URL}/api/v1/healthcheck" | grep -q "UP"; then
    echo "✅ Middleware is running"
else
    echo "❌ Middleware is NOT running"
    exit 1
fi

echo "Checking Nexus..."
if curl -s "${NEXUS_URL}" > /dev/null 2>&1; then
    echo "✅ Nexus is running"
else
    echo "❌ Nexus is NOT running"
    exit 1
fi
echo ""

# Step 2: Test storing query stats directly
echo "Step 2: Testing direct query stats storage..."
STORE_RESPONSE=$(curl -s -X POST "${MIDDLEWARE_URL}/api/v1/queryStats/store" \
  -H "Content-Type: application/json" \
  -d '{
    "query": "{ getTransaction(id: \"test123\") { id asset } }",
    "efficiency": {
      "score": 85,
      "estimatedTime": "5ms",
      "complexity": "low",
      "resourceUsage": "Low"
    },
    "explanation": {
      "explanation": "This is a test query for flow verification",
      "complexity": "low"
    },
    "optimizations": [],
    "timestamp": "'$(date -u +"%Y-%m-%dT%H:%M:%S.%3NZ")'"
  }')

if echo "$STORE_RESPONSE" | grep -q "success"; then
    echo "✅ Query stats stored successfully"
    echo "   Response: $STORE_RESPONSE"
else
    echo "❌ Failed to store query stats"
    echo "   Response: $STORE_RESPONSE"
    exit 1
fi
echo ""

# Step 3: Test retrieving query stats
echo "Step 3: Testing query stats retrieval..."
GET_RESPONSE=$(curl -s "${MIDDLEWARE_URL}/api/v1/queryStats?limit=5")
if echo "$GET_RESPONSE" | grep -q "success"; then
    echo "✅ Query stats retrieved successfully"
    STAT_COUNT=$(echo "$GET_RESPONSE" | grep -o '"count":[0-9]*' | cut -d':' -f2)
    echo "   Stats count: $STAT_COUNT"
    if [ "$STAT_COUNT" -gt 0 ]; then
        echo "   ✅ Query stats are available"
    else
        echo "   ⚠️  No query stats found (this is OK if no queries analyzed yet)"
    fi
else
    echo "❌ Failed to retrieve query stats"
    echo "   Response: $GET_RESPONSE"
    exit 1
fi
echo ""

# Step 4: Test Nexus → Middleware flow
echo "Step 4: Testing Nexus → Middleware flow..."
echo "Sending test query to Nexus API..."
NEXUS_RESPONSE=$(curl -s -X POST "${NEXUS_URL}/api/graphql-tutor/analyze" \
  -H "Content-Type: application/json" \
  -d '{"query": "{ getTransaction(id: \"flow-test\") { id } }"}')

if echo "$NEXUS_RESPONSE" | grep -q "explanation\|efficiency"; then
    echo "✅ Nexus API responded successfully"
    echo "   (This should have triggered middleware storage)"
else
    echo "⚠️  Nexus API may have issues"
    echo "   Response: $NEXUS_RESPONSE" | head -5
fi
echo ""

# Wait a moment for middleware to process
echo "Waiting 2 seconds for middleware to process..."
sleep 2

# Step 5: Verify stats were stored from Nexus
echo "Step 5: Verifying stats from Nexus..."
FINAL_STATS=$(curl -s "${MIDDLEWARE_URL}/api/v1/queryStats?limit=10")
FINAL_COUNT=$(echo "$FINAL_STATS" | grep -o '"count":[0-9]*' | cut -d':' -f2)
echo "   Total stats count: $FINAL_COUNT"

if [ "$FINAL_COUNT" -gt 0 ]; then
    echo "✅ Query stats are being stored"
    
    # Check if our test query is there
    if echo "$FINAL_STATS" | grep -q "flow-test"; then
        echo "✅ Nexus → Middleware flow is working!"
    else
        echo "⚠️  Test query not found, but stats exist (may need to analyze more queries)"
    fi
else
    echo "⚠️  No query stats found"
    echo "   This could mean:"
    echo "   1. Nexus is not sending stats to middleware"
    echo "   2. Check Nexus logs: tail -f /tmp/nexus.log"
    echo "   3. Check middleware logs: tail -f /tmp/reslens-middleware.log"
fi
echo ""

# Step 6: Test aggregated stats
echo "Step 6: Testing aggregated statistics..."
AGG_RESPONSE=$(curl -s "${MIDDLEWARE_URL}/api/v1/queryStats/aggregated")
if echo "$AGG_RESPONSE" | grep -q "success"; then
    echo "✅ Aggregated stats retrieved"
    TOTAL=$(echo "$AGG_RESPONSE" | grep -o '"totalQueries":[0-9]*' | cut -d':' -f2)
    AVG=$(echo "$AGG_RESPONSE" | grep -o '"avgEfficiency":[0-9.]*' | cut -d':' -f2)
    echo "   Total queries: $TOTAL"
    echo "   Average efficiency: $AVG"
    
    # Check complexity distribution
    if echo "$AGG_RESPONSE" | grep -q "complexityDistribution"; then
        echo "   ✅ Complexity distribution available"
        echo "$AGG_RESPONSE" | grep -A 5 "complexityDistribution" | head -6
    else
        echo "   ⚠️  Complexity distribution not found"
    fi
else
    echo "❌ Failed to get aggregated stats"
    echo "   Response: $AGG_RESPONSE"
fi
echo ""

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "✅ Flow Test Complete"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "📋 Next Steps:"
echo "   1. If stats are not showing in ResLens, check:"
echo "      - ResLens .env has VITE_MIDDLEWARE_BASE_URL=http://localhost:3003/api/v1"
echo "      - ResLens src/static/url.ts points to correct URL"
echo "      - Restart ResLens after .env changes"
echo ""
echo "   2. Check logs:"
echo "      - Nexus: tail -f /tmp/nexus.log | grep -i middleware"
echo "      - Middleware: tail -f /tmp/reslens-middleware.log"
echo ""

