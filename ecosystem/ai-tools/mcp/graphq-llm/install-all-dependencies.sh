#!/bin/bash
# Install dependencies for all copied repositories

set -e

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "📦 Installing Dependencies for All Repositories"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

BASE_DIR="/Users/sandhya/UCD/ECS_DDS/graphq-llm"

# ResLens
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "1. Installing ResLens dependencies..."
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
if [ -d "$BASE_DIR/ResLens" ]; then
    cd "$BASE_DIR/ResLens"
    if [ -d "node_modules" ]; then
        echo "✅ ResLens dependencies already installed"
    else
        echo "Installing..."
        npm install
        echo "✅ ResLens dependencies installed"
    fi
else
    echo "⚠️  ResLens directory not found, skipping..."
fi
echo ""

# ResLens-Middleware
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "2. Installing ResLens-Middleware dependencies..."
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
if [ -d "$BASE_DIR/ResLens-Middleware/middleware" ]; then
    cd "$BASE_DIR/ResLens-Middleware/middleware"
    if [ -d "node_modules" ]; then
        echo "✅ ResLens-Middleware dependencies already installed"
    else
        echo "Installing..."
        npm install
        echo "✅ ResLens-Middleware dependencies installed"
    fi
elif [ -d "$BASE_DIR/ResLens-Middleware" ]; then
    cd "$BASE_DIR/ResLens-Middleware"
    if [ -d "node_modules" ]; then
        echo "✅ ResLens-Middleware dependencies already installed"
    else
        echo "Installing..."
        npm install
        echo "✅ ResLens-Middleware dependencies installed"
    fi
else
    echo "⚠️  ResLens-Middleware directory not found, skipping..."
fi
echo ""

# Nexus
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "3. Installing Nexus dependencies..."
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
if [ -d "$BASE_DIR/nexus" ]; then
    cd "$BASE_DIR/nexus"
    if [ -d "node_modules" ]; then
        echo "✅ Nexus dependencies already installed"
    else
        echo "Installing..."
        npm install
        echo "✅ Nexus dependencies installed"
    fi
else
    echo "⚠️  Nexus directory not found, skipping..."
fi
echo ""

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "✅ Dependency Installation Complete!"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "Next steps:"
echo "  1. Run ./fix-reslens.sh to start all services"
echo "  2. Or manually start each service:"
echo "     - ResLens: cd ResLens && npm run dev"
echo "     - Middleware: cd ResLens-Middleware/middleware && PORT=3003 npm start"
echo "     - Nexus: cd nexus && PORT=3002 npm run dev"
echo ""

