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
# Install compatible versions from the start to avoid conflicts
python3 -m pip install --quiet \
    requests base58 cryptography pysha3 \
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

# Fix legacy tx_Dict typo that breaks GraphQL mutations
sed -i 's/tx_Dict/tx_dict/g' resdb_driver/transaction.py 2>/dev/null || true

# Install compatible versions
echo "📦 Installing compatible package versions..."
# strawberry-graphql 0.69.0 requires click<8.0,>=7.0 and graphql-core
# Flask 2.0.3 works with click<8.0
# Install compatible versions together to avoid conflicts
python3 -m pip install --quiet --force-reinstall \
    'click==7.1.2' \
    'strawberry-graphql==0.69.0' \
    'flask==2.0.3' \
    'werkzeug==2.0.3' \
    'flask-cors==3.0.10' \
    'graphql-core==3.1.6' 2>&1 | grep -v "^$" || true

# Fix app.py to use port 5001 instead of 8000
echo "🔧 Configuring GraphQL server to use port 5001..."
sed -i 's/app.run(port="8000")/app.run(host="0.0.0.0", port=5001)/' app.py 2>/dev/null || true

# Fix JSONScalar.parse_value to handle string inputs
echo "🔧 Fixing JSONScalar.parse_value to parse JSON strings..."
python3 << 'PYEOF'
import re

with open('app.py', 'r') as f:
    content = f.read()

# Update JSONScalar.parse_value
old_pattern = r'(@staticmethod\s+def parse_value\(value: Any\) -> Any:\s+return value  # Accept JSON as is)'
new_method = '''@staticmethod
    def parse_value(value: Any) -> Any:
        import json
        # If value is a string, parse it as JSON
        if isinstance(value, str):
            try:
                return json.loads(value)
            except (json.JSONDecodeError, TypeError):
                return value
        # If value is already a dict/list, return as is
        return value  # Accept JSON as is'''

content = re.sub(old_pattern, new_method, content, flags=re.MULTILINE)

# Update postTransaction to handle string assets
old_post = r'(def postTransaction\(self, data: PrepareAsset\) -> CommitTransaction:\s+prepared_token_tx = db\.transactions\.prepare\(\s+operation=data\.operation,\s+signers=data\.signerPublicKey,\s+recipients=\[\(\[data\.recipientPublicKey\], data\.amount\)\],\s+asset=data\.asset,\s+\))'
new_post = '''def postTransaction(self, data: PrepareAsset) -> CommitTransaction:
        # Ensure asset is a dict (JSONScalar might pass it as string)
        asset = data.asset
        if isinstance(asset, str):
            import json
            try:
                asset = json.loads(asset)
            except (json.JSONDecodeError, TypeError):
                pass  # Keep as-is if parsing fails
        
        prepared_token_tx = db.transactions.prepare(
            operation=data.operation,
            signers=data.signerPublicKey,
            recipients=[([data.recipientPublicKey], data.amount)],
            asset=asset,
        )'''

content = re.sub(old_post, new_post, content, flags=re.MULTILINE | re.DOTALL)

with open('app.py', 'w') as f:
    f.write(content)

print('✅ Fixed JSONScalar and postTransaction')
PYEOF

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

