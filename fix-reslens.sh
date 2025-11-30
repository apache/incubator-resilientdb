#!/bin/bash
# Fix ResLens, ResLens Middleware, Nexus, and GraphQ-LLM Backend

set -e

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "🔧 Fixing ResLens, Middleware, and Nexus"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

# Use local copies if they exist, otherwise use sibling directories
GRAPHQ_LLM_DIR="/Users/sandhya/UCD/ECS_DDS/graphq-llm"
if [ -d "$GRAPHQ_LLM_DIR/ResLens" ]; then
    RESLENS_DIR="$GRAPHQ_LLM_DIR/ResLens"
else
    RESLENS_DIR="/Users/sandhya/UCD/ECS_DDS/ResLens"
fi

if [ -d "$GRAPHQ_LLM_DIR/ResLens-Middleware/middleware" ]; then
    MIDDLEWARE_DIR="$GRAPHQ_LLM_DIR/ResLens-Middleware/middleware"
elif [ -d "$GRAPHQ_LLM_DIR/ResLens-Middleware" ]; then
    MIDDLEWARE_DIR="$GRAPHQ_LLM_DIR/ResLens-Middleware"
else
    MIDDLEWARE_DIR="/Users/sandhya/UCD/ECS_DDS/ResLens-Middleware/middleware"
fi

if [ -d "$GRAPHQ_LLM_DIR/nexus" ]; then
    NEXUS_DIR="$GRAPHQ_LLM_DIR/nexus"
else
    NEXUS_DIR="/Users/sandhya/UCD/ECS_DDS/nexus"
fi

# Step 1: Start all Docker services (ResilientDB, GraphQ-LLM Backend, MCP Server)
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Step 1: Starting Docker Services"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
cd "$GRAPHQ_LLM_DIR"

# Check if ResilientDB is running
echo "Checking ResilientDB (port 5001)..."
if curl -s -X POST http://localhost:5001/graphql -H "Content-Type: application/json" -d '{"query": "{ __typename }"}' > /dev/null 2>&1; then
    echo "✅ ResilientDB is already running"
else
    echo "⚠️  ResilientDB is not running. Starting..."
    docker-compose -f docker-compose.dev.yml up -d resilientdb
    
    echo "Waiting for ResilientDB to be healthy (60 seconds)..."
    for i in {1..20}; do
        if docker-compose -f docker-compose.dev.yml ps resilientdb | grep -q "healthy" 2>/dev/null; then
            echo "✅ ResilientDB is healthy"
            break
        fi
        if [ $i -eq 20 ]; then
            echo "⚠️  ResilientDB may still be starting. Check logs:"
            echo "   docker-compose -f docker-compose.dev.yml logs resilientdb"
        fi
        sleep 3
    done
fi
echo ""

# Check if GraphQ-LLM Backend is running
echo "Checking GraphQ-LLM Backend (port 3001)..."
if curl -s http://localhost:3001/health > /dev/null 2>&1; then
    echo "✅ GraphQ-LLM Backend is already running"
else
    echo "⚠️  GraphQ-LLM Backend is not running. Starting..."
    docker-compose -f docker-compose.dev.yml up -d graphq-llm-backend
    
    echo "Waiting for backend to start (30 seconds)..."
    sleep 30
    
    # Verify backend
    for i in {1..10}; do
        if curl -s http://localhost:3001/health | grep -q "ok" 2>/dev/null; then
            echo "✅ GraphQ-LLM Backend is now running"
            break
        fi
        if [ $i -eq 10 ]; then
            echo "⚠️  Backend may still be starting. Check logs:"
            echo "   docker-compose -f docker-compose.dev.yml logs graphq-llm-backend"
        fi
        sleep 3
    done
fi
echo ""

# Check if MCP Server is running (optional, but start it if not running)
echo "Checking GraphQ-LLM MCP Server..."
if docker-compose -f docker-compose.dev.yml ps graphq-llm-mcp-server | grep -q "Up" 2>/dev/null; then
    echo "✅ GraphQ-LLM MCP Server is already running"
else
    echo "⚠️  GraphQ-LLM MCP Server is not running. Starting..."
    docker-compose -f docker-compose.dev.yml up -d graphq-llm-mcp-server
    echo "✅ GraphQ-LLM MCP Server started"
fi
echo ""

# Step 2: Check if ResLens directory exists
echo "Step 2: Checking ResLens directory..."
if [ ! -d "$RESLENS_DIR" ]; then
    echo "❌ Error: ResLens directory not found at: $RESLENS_DIR"
    echo "   Please either:"
    echo "   1. Copy ResLens into graphq-llm: ./copy-repositories.sh"
    echo "   2. Or clone ResLens as sibling:"
    echo "      cd /Users/sandhya/UCD/ECS_DDS"
    echo "      git clone https://github.com/harish876/ResLens.git"
    exit 1
fi
echo "✅ ResLens directory found at: $RESLENS_DIR"
echo ""

# Step 3: Check and free ports
echo "Step 3: Checking ports..."
# Check port 3002 (Nexus)
if lsof -ti:3002 >/dev/null 2>&1; then
    echo "⚠️  Port 3002 is in use. Killing existing process..."
    lsof -ti:3002 | xargs kill -9 2>/dev/null || true
    sleep 2
    echo "✅ Port 3002 freed"
else
    echo "✅ Port 3002 is free"
fi

# Check port 3003 (Middleware)
if lsof -ti:3003 >/dev/null 2>&1; then
    echo "⚠️  Port 3003 is in use. Killing existing process..."
    lsof -ti:3003 | xargs kill -9 2>/dev/null || true
    sleep 2
    echo "✅ Port 3003 freed"
else
    echo "✅ Port 3003 is free"
fi

# Check port 5173 (ResLens)
if lsof -ti:5173 >/dev/null 2>&1; then
    echo "⚠️  Port 5173 is in use. Killing existing process..."
    lsof -ti:5173 | xargs kill -9 2>/dev/null || true
    sleep 2
    echo "✅ Port 5173 freed"
else
    echo "✅ Port 5173 is free"
fi
echo ""

# Step 4: Navigate to ResLens
cd "$RESLENS_DIR"
echo "Step 4: Navigated to ResLens directory"
echo "📍 Location: $(pwd)"
echo ""

# Step 5: Check package.json
echo "Step 5: Checking package.json..."
if [ ! -f "package.json" ]; then
    echo "❌ Error: package.json not found"
    exit 1
fi
echo "✅ package.json found"
echo ""

# Step 6: Install dependencies if needed
echo "Step 6: Checking dependencies..."
if [ ! -d "node_modules" ]; then
    echo "📦 Installing dependencies (this may take a few minutes)..."
    npm install
    echo "✅ Dependencies installed"
else
    echo "✅ Dependencies already installed"
fi
echo ""

# Step 7: Check for .env file
echo "Step 7: Checking environment configuration..."
if [ ! -f ".env" ]; then
    echo "⚠️  .env file not found. Creating one..."
    cat > .env << 'EOF'
# ResLens Environment Configuration
# Pointing to ResLens Middleware for query statistics

# Primary middleware API (ResLens Middleware)
VITE_MIDDLEWARE_BASE_URL=http://localhost:3003/api/v1

# Secondary/Development middleware API (also ResLens Middleware)
VITE_MIDDLEWARE_SECONDARY_BASE_URL=http://localhost:3003/api/v1

# ResLens Tools URL (if needed)
VITE_DEV_RESLENS_TOOLS_URL=http://localhost:3003
EOF
    echo "✅ .env file created"
else
    echo "✅ .env file exists"
    # Check if VITE_MIDDLEWARE_BASE_URL is set
    if ! grep -q "VITE_MIDDLEWARE_BASE_URL" .env; then
        echo "⚠️  VITE_MIDDLEWARE_BASE_URL not found in .env. Adding it..."
        echo "" >> .env
        echo "VITE_MIDDLEWARE_BASE_URL=http://localhost:3003/api/v1" >> .env
        echo "✅ Added VITE_MIDDLEWARE_BASE_URL"
    fi
fi
echo ""

# Step 8: Check for vite.config.ts
echo "Step 8: Checking Vite configuration..."
if [ -f "vite.config.ts" ]; then
    echo "✅ vite.config.ts found"
    # Check if server.host is configured
    if ! grep -q "host.*true" vite.config.ts; then
        echo "⚠️  server.host not configured. This may cause issues."
        echo "   Consider adding: server: { host: true } to vite.config.ts"
    fi
else
    echo "⚠️  vite.config.ts not found"
fi
echo ""

# Step 8: Clear Vite cache if needed
echo "Step 8: Clearing Vite cache..."
if [ -d "node_modules/.vite" ]; then
    rm -rf node_modules/.vite
    echo "✅ Vite cache cleared"
else
    echo "✅ No Vite cache to clear"
fi
echo ""

# Step 9: Start ResLens Middleware
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Step 9: Starting ResLens Middleware (port 3003)"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
if [ ! -d "$MIDDLEWARE_DIR" ]; then
    echo "❌ Error: ResLens Middleware not found at: $MIDDLEWARE_DIR"
    echo "   Please either:"
    echo "   1. Copy ResLens-Middleware into graphq-llm: ./copy-repositories.sh"
    echo "   2. Or clone ResLens Middleware as sibling:"
    echo "      cd /Users/sandhya/UCD/ECS_DDS"
    echo "      git clone https://github.com/apache/incubator-resilientdb-ResLens-Middleware.git"
    exit 1
fi

cd "$MIDDLEWARE_DIR"
echo "📍 Location: $(pwd)"

# Install dependencies if needed
if [ ! -d "node_modules" ]; then
    echo "📦 Installing dependencies..."
    npm install
    echo "✅ Dependencies installed"
else
    echo "✅ Dependencies already installed"
fi

# Kill any existing middleware processes
echo "Killing any existing middleware processes..."
pkill -f "node.*server.js.*middleware" 2>/dev/null || true
pkill -f "npm.*start.*middleware" 2>/dev/null || true
sleep 2

# Start middleware in background
echo "Starting ResLens Middleware with: PORT=3003 npm start"
PORT=3003 nohup npm start > /tmp/reslens-middleware.log 2>&1 &
MIDDLEWARE_PID=$!

echo "✅ Middleware started with PID: $MIDDLEWARE_PID"
echo "📋 Logs: tail -f /tmp/reslens-middleware.log"
sleep 5

# Verify middleware is running
echo "Verifying middleware is accessible..."
for i in {1..10}; do
    if curl -s http://localhost:3003/api/v1/healthcheck > /dev/null 2>&1; then
        echo "✅ Middleware is accessible at http://localhost:3003"
        break
    fi
    if [ $i -eq 10 ]; then
        echo "⚠️  Middleware may still be starting. Check logs:"
        echo "   tail -f /tmp/reslens-middleware.log"
    fi
    sleep 2
done
echo ""

# Step 10: Start ResLens
echo "Step 9: Starting ResLens..."
echo "🚀 Starting ResLens development server on port 5173..."
echo ""

# Ensure we're in the ResLens directory
cd "$RESLENS_DIR"
echo "📍 Current directory: $(pwd)"

# Kill any existing ResLens processes
echo "Killing any existing ResLens processes..."
pkill -f "vite.*5173" 2>/dev/null || true
pkill -f "npm.*dev" 2>/dev/null || true
sleep 2

# Start ResLens in background
echo "Starting ResLens with: npm run dev"
nohup npm run dev > /tmp/reslens.log 2>&1 &
RESLENS_PID=$!

echo "✅ ResLens started with PID: $RESLENS_PID"
echo "📋 Logs: tail -f /tmp/reslens.log"
echo ""

# Step 11: Wait and verify ResLens
echo "Step 11: Waiting for ResLens to start (10 seconds)..."
sleep 10

echo "Checking if ResLens is accessible..."
for i in {1..10}; do
    if curl -s http://localhost:5173 > /dev/null 2>&1; then
        echo "✅ ResLens is accessible at http://localhost:5173"
        break
    fi
    if [ $i -eq 10 ]; then
        echo "❌ ResLens failed to start"
        echo ""
        echo "📋 Recent logs:"
        tail -20 /tmp/reslens.log
        echo ""
        echo "🔧 Troubleshooting:"
        echo "   1. Check logs: tail -f /tmp/reslens.log"
        echo "   2. Verify dependencies: cd $RESLENS_DIR && npm install"
        echo "   3. Check for errors in package.json scripts"
        exit 1
    fi
    sleep 2
done

# Step 12: Start Nexus
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Step 12: Starting Nexus (port 3002)"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
if [ ! -d "$NEXUS_DIR" ]; then
    echo "❌ Error: Nexus not found at: $NEXUS_DIR"
    echo "   Please either:"
    echo "   1. Copy nexus into graphq-llm: ./copy-repositories.sh"
    echo "   2. Or clone Nexus as sibling:"
    echo "      cd /Users/sandhya/UCD/ECS_DDS"
    echo "      git clone https://github.com/ResilientApp/nexus.git"
    exit 1
fi

cd "$NEXUS_DIR"
echo "📍 Location: $(pwd)"

# Check/update .env file for middleware URL
echo "Checking Nexus .env configuration..."
if [ ! -f ".env" ]; then
    echo "⚠️  .env file not found. Creating one..."
    touch .env
fi

# Add or update RESLENS_MIDDLEWARE_URL
if grep -q "RESLENS_MIDDLEWARE_URL" .env; then
    echo "✅ RESLENS_MIDDLEWARE_URL is already set"
    # Update it to ensure it's correct
    sed -i.bak 's|RESLENS_MIDDLEWARE_URL=.*|RESLENS_MIDDLEWARE_URL=http://localhost:3003|' .env 2>/dev/null || \
    sed -i '' 's|RESLENS_MIDDLEWARE_URL=.*|RESLENS_MIDDLEWARE_URL=http://localhost:3003|' .env 2>/dev/null || \
    echo "⚠️  Could not update .env automatically"
else
    echo "⚠️  RESLENS_MIDDLEWARE_URL not found. Adding it..."
    echo "" >> .env
    echo "# ResLens Middleware URL" >> .env
    echo "RESLENS_MIDDLEWARE_URL=http://localhost:3003" >> .env
    echo "✅ Added RESLENS_MIDDLEWARE_URL to .env"
fi
echo ""

# Install dependencies if needed
if [ ! -d "node_modules" ]; then
    echo "📦 Installing dependencies (this may take a few minutes)..."
    npm install
    echo "✅ Dependencies installed"
else
    echo "✅ Dependencies already installed"
fi

# Kill any existing Nexus processes
echo "Killing any existing Nexus processes..."
pkill -f "next.*dev" 2>/dev/null || true
pkill -f "npm.*dev.*nexus" 2>/dev/null || true
sleep 2

# Start Nexus in background
echo "Starting Nexus with: PORT=3002 npm run dev"
PORT=3002 nohup npm run dev > /tmp/nexus.log 2>&1 &
NEXUS_PID=$!

echo "✅ Nexus started with PID: $NEXUS_PID"
echo "📋 Logs: tail -f /tmp/nexus.log"
sleep 10

# Verify Nexus is running
echo "Verifying Nexus is accessible..."
for i in {1..10}; do
    if curl -s http://localhost:3002 > /dev/null 2>&1; then
        echo "✅ Nexus is accessible at http://localhost:3002"
        break
    fi
    if [ $i -eq 10 ]; then
        echo "⚠️  Nexus may still be starting. Check logs:"
        echo "   tail -f /tmp/nexus.log"
    fi
    sleep 2
done
echo ""

# Step 13: Final verification of all services
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Step 13: Final Verification"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

# Verify ResilientDB
echo "Verifying ResilientDB..."
if curl -s -X POST http://localhost:5001/graphql -H "Content-Type: application/json" -d '{"query": "{ __typename }"}' | grep -q "__typename" 2>/dev/null; then
    echo "✅ ResilientDB GraphQL (port 5001) is responding"
else
    echo "⚠️  ResilientDB GraphQL may not be responding"
fi

# Verify GraphQ-LLM Backend
echo "Verifying GraphQ-LLM Backend..."
if curl -s http://localhost:3001/health | grep -q "ok" 2>/dev/null; then
    echo "✅ GraphQ-LLM Backend (port 3001) is responding"
else
    echo "⚠️  GraphQ-LLM Backend may not be responding"
fi

# Verify ResLens Middleware
echo "Verifying ResLens Middleware..."
if curl -s http://localhost:3003/api/v1/healthcheck | grep -q "UP" 2>/dev/null; then
    echo "✅ ResLens Middleware (port 3003) is responding"
else
    echo "⚠️  ResLens Middleware may not be responding"
fi

# Verify ResLens Frontend
echo "Verifying ResLens Frontend..."
if curl -s http://localhost:5173 > /dev/null 2>&1; then
    echo "✅ ResLens Frontend (port 5173) is accessible"
else
    echo "⚠️  ResLens Frontend may not be accessible"
fi

# Verify Nexus
echo "Verifying Nexus..."
if curl -s http://localhost:3002 > /dev/null 2>&1; then
    echo "✅ Nexus (port 3002) is accessible"
else
    echo "⚠️  Nexus may not be accessible"
fi
echo ""

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "✅ All Services Startup Complete!"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "📍 Docker Services:"
echo "   ResilientDB KV: http://localhost:18000"
echo "   ResilientDB GraphQL: http://localhost:5001"
echo "   GraphQ-LLM Backend: http://localhost:3001"
echo "      Health: http://localhost:3001/health"
echo "      Logs: docker-compose -f docker-compose.dev.yml logs graphq-llm-backend"
echo "   GraphQ-LLM MCP Server: (running in Docker)"
echo "      Logs: docker-compose -f docker-compose.dev.yml logs graphq-llm-mcp-server"
echo ""
echo "📍 Local Services:"
echo "   ResLens Middleware: http://localhost:3003"
echo "      Health: http://localhost:3003/api/v1/healthcheck"
echo "      PID: $MIDDLEWARE_PID"
echo "      Logs: tail -f /tmp/reslens-middleware.log"
echo ""
echo "   ResLens Frontend: http://localhost:5173"
echo "      Dashboard: http://localhost:5173/dashboard"
echo "      Query Stats: http://localhost:5173/dashboard?tab=query_stats"
echo "      PID: $RESLENS_PID"
echo "      Logs: tail -f /tmp/reslens.log"
echo ""
echo "   Nexus: http://localhost:3002"
echo "      GraphQL Tutor: http://localhost:3002/graphql-tutor"
echo "      PID: $NEXUS_PID"
echo "      Logs: tail -f /tmp/nexus.log"
echo ""
echo "🧪 Quick Tests:"
echo "   1. Test ResilientDB:"
echo "      curl -X POST http://localhost:5001/graphql -H 'Content-Type: application/json' -d '{\"query\": \"{ __typename }\"}'"
echo ""
echo "   2. Test GraphQ-LLM Backend:"
echo "      curl http://localhost:3001/health"
echo ""
echo "   3. Test ResLens Middleware:"
echo "      curl http://localhost:3003/api/v1/healthcheck"
echo ""
echo "   4. Test Nexus → Backend:"
echo "      curl -X POST http://localhost:3002/api/graphql-tutor/analyze -H 'Content-Type: application/json' -d '{\"query\": \"{ __typename }\"}'"
echo ""
echo "   5. Test Query Stats:"
echo "      curl http://localhost:3003/api/v1/queryStats?limit=5"
echo ""
echo "🌐 Open in Browser:"
echo "   1. Nexus GraphQL Tutor: http://localhost:3002/graphql-tutor"
echo "   2. ResLens Dashboard: http://localhost:5173/dashboard"
echo "   3. ResLens Query Stats: http://localhost:5173/dashboard?tab=query_stats"
echo ""
echo "📋 View All Logs:"
echo "   Docker: docker-compose -f docker-compose.dev.yml logs -f"
echo "   Middleware: tail -f /tmp/reslens-middleware.log"
echo "   ResLens: tail -f /tmp/reslens.log"
echo "   Nexus: tail -f /tmp/nexus.log"
echo ""

