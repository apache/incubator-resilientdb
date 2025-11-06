#!/bin/bash
# Setup script for ResilientDB GraphQL server
# This script installs dependencies and fixes Python 3.8 compatibility issues

set -e

echo "🔧 Setting up ResilientDB GraphQL Server..."
echo "=========================================="
echo ""

cd /app/ecosystem/graphql

# Install system dependencies
echo "📦 Installing system dependencies..."
apt-get update -qq
apt-get install -y -qq python3-pip python3-venv python3-dev 2>&1 | grep -v "^$" || true

# Upgrade pip
echo "⬆️  Upgrading pip..."
python3 -m pip install --upgrade pip --quiet 2>&1 | grep -v "^$" || true

# Install Python dependencies
echo "📚 Installing Python dependencies..."
python3 -m pip install --quiet \
    requests flask flask-cors ariadne base58 cryptography pysha3 \
    cryptoconditions python-rapidjson 2>&1 | grep -v "^$" || true

# Fix Python 3.8 compatibility issues in resdb_driver
echo "🔧 Fixing Python 3.8 compatibility issues..."

# Fix pool.py - add List import and fix type hints
if ! grep -q "from typing import List" resdb_driver/pool.py 2>/dev/null; then
    sed -i '1a from typing import List' resdb_driver/pool.py
fi
sed -i 's/list\[Connection\]/List[Connection]/g' resdb_driver/pool.py 2>/dev/null || true

# Fix driver.py - add Dict, List imports and fix type hints
if ! grep -q "from typing import.*Dict" resdb_driver/driver.py 2>/dev/null; then
    sed -i '/^from typing import/s/$/, Dict, Optional/' resdb_driver/driver.py 2>/dev/null || true
    sed -i 's/from typing import Any, Union$/from typing import Any, Union, List, Dict, Optional/' resdb_driver/driver.py 2>/dev/null || true
fi
sed -i 's/list\[Union/List[Union/g; s/list\[str/List[str/g; s/list\[dict/List[dict/g; s/dict\[str/Dict[str/g; s/dict\[/Dict[/g; s/list\[/List[/g' resdb_driver/driver.py 2>/dev/null || true

# Fix all other Python files in resdb_driver
for f in resdb_driver/*.py; do
    if [ -f "$f" ]; then
        # Fix type hints
        sed -i 's/dict\[str/Dict[str/g; s/list\[str/List[str/g; s/dict\[/Dict[/g; s/list\[/List[/g' "$f" 2>/dev/null || true
        # Add imports if needed
        if grep -q "Dict\[" "$f" && ! grep -q "from typing import.*Dict" "$f"; then
            sed -i '/^from typing import/s/$/, Dict, Optional/' "$f" 2>/dev/null || true
        fi
        if grep -q "List\[" "$f" && ! grep -q "from typing import.*List" "$f"; then
            sed -i '/^from typing import/s/$/, List/' "$f" 2>/dev/null || true
        fi
    fi
done

# Install compatible versions
echo "📦 Installing compatible package versions..."
python3 -m pip install --quiet 'click>=7.0,<8.0' 'strawberry-graphql==0.69.0' 2>&1 | grep -v "^$" || true

echo ""
echo "✅ GraphQL setup complete!"
echo ""
echo "🚀 Starting GraphQL server on port 5001..."
echo ""

# Start GraphQL server in background
python3 app.py > /tmp/graphql.log 2>&1 &
GRAPHQL_PID=$!

# Wait a moment for server to start
sleep 3

# Check if server started successfully
if ps -p $GRAPHQL_PID > /dev/null 2>&1; then
    echo "✅ GraphQL server started (PID: $GRAPHQL_PID)"
    echo "📊 Endpoint: http://localhost:5001/graphql"
    echo "📝 Logs: /tmp/graphql.log"
    echo ""
else
    echo "⚠️  GraphQL server may not have started. Check logs:"
    echo "   tail -f /tmp/graphql.log"
    echo ""
fi

# Keep script running to keep server alive
wait $GRAPHQL_PID

