<!--
  ~ Licensed to the Apache Software Foundation (ASF) under one
  ~ or more contributor license agreements.  See the NOTICE file
  ~ distributed with this work for additional information
  ~ regarding copyright ownership.  The ASF licenses this file
  ~ to you under the Apache License, Version 2.0 (the
  ~ "License"); you may not use this file except in compliance
  ~ with the License.  You may obtain a copy of the License at
  ~
  ~   http://www.apache.org/licenses/LICENSE-2.0
  ~
  ~ Unless required by applicable law or agreed to in writing,
  ~ software distributed under the License is distributed on an
  ~ "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
  ~ KIND, either express or implied.  See the License for the
  ~ specific language governing permissions and limitations
  ~ under the License.
-->

# Testing Guide for ResilientDB MCP Server

This guide provides individual commands to test the MCP server and GraphQL functionalities.

## Prerequisites

1. ResilientDB running with:
   - GraphQL server on port 8000
   - HTTP/Crow server on port 18000
2. Python 3.11+ installed
3. Dependencies installed: `pip install -r requirements.txt`

## 1. Test GraphQL Server (Port 8000)

### Check if GraphQL server is running

```bash
curl -X POST http://localhost:8000/graphql \
  -H "Content-Type: application/json" \
  -d '{"query": "{ __schema { queryType { name } } }"}'
```

Expected: Should return schema information without errors.

### Test getTransaction Query

First, create a transaction using HTTP API to get a transaction ID:

```bash
# Create a transaction via HTTP API
curl -X POST -d '{"id":"test-tx-1","value":"test data"}' \
  http://localhost:18000/v1/transactions/commit
```

Then query it via GraphQL (Note: This may not work if the transaction was created via HTTP, as GraphQL is for asset transactions):

```bash
# Get transaction by ID
curl -X POST http://localhost:8000/graphql \
  -H "Content-Type: application/json" \
  -d '{
    "query": "{ getTransaction(id: \"YOUR_TRANSACTION_ID\") { id version amount uri type publicKey operation metadata asset signerPublicKey } }"
  }'
```

Replace `YOUR_TRANSACTION_ID` with an actual asset transaction ID from ResilientDB.

### Test postTransaction Mutation

You'll need to generate cryptographic keys first. For testing, you can use the ResilientDB key generation tools or create keys manually.

```bash
# Post a new asset transaction
curl -X POST http://localhost:8000/graphql \
  -H "Content-Type: application/json" \
  -d '{
    "query": "mutation PostTransaction($data: PrepareAsset!) { postTransaction(data: $data) { id } }",
    "variables": {
      "data": {
        "operation": "CREATE",
        "amount": 100,
        "signerPublicKey": "YOUR_SIGNER_PUBLIC_KEY",
        "signerPrivateKey": "YOUR_SIGNER_PRIVATE_KEY",
        "recipientPublicKey": "YOUR_RECIPIENT_PUBLIC_KEY",
        "asset": {
          "data": "test asset data"
        }
      }
    }
  }'
```

Replace the placeholder keys with actual cryptographic keys (for example, generated via the GraphQL repo’s key tools or `generate_keys_utility.py` in this project).

When using the **MCP server via Claude**, you normally **do not need to generate keys manually**:
- The `generateKeys` MCP tool can be called explicitly to get signer/recipient keypairs.
- The `postTransaction` MCP tool will **auto-generate keys** if they are not provided in the arguments.

### Inspect GraphQL Schema

```bash
# Get all available queries
curl -X POST http://localhost:8000/graphql \
  -H "Content-Type: application/json" \
  -d '{
    "query": "{ __type(name: \"Query\") { fields { name description args { name type { name kind ofType { name } } } type { name kind ofType { name } } } } }"
  }' | python3 -m json.tool

# Get all available mutations
curl -X POST http://localhost:8000/graphql \
  -H "Content-Type: application/json" \
  -d '{
    "query": "{ __type(name: \"Mutation\") { fields { name description args { name type { name kind ofType { name } } } type { name kind ofType { name } } } } }"
  }' | python3 -m json.tool

# Get PrepareAsset input type
curl -X POST http://localhost:8000/graphql \
  -H "Content-Type: application/json" \
  -d '{
    "query": "{ __type(name: \"PrepareAsset\") { inputFields { name type { name kind ofType { name } } } } }"
  }' | python3 -m json.tool
```

## 2. Test HTTP REST API (Port 18000)

### Test set (Store key-value)

```bash
# Store a key-value pair
curl -X POST -d '{"id":"test-key-1","value":"test value 1"}' \
  http://localhost:18000/v1/transactions/commit
```

Expected output: `id: test-key-1`

### Test get (Retrieve key-value)

```bash
# Retrieve a value by key
curl http://localhost:18000/v1/transactions/test-key-1
```

Expected: Returns the stored value as JSON.

### Test multiple operations

```bash
# Store multiple values
curl -X POST -d '{"id":"user-1","value":"Alice"}' \
  http://localhost:18000/v1/transactions/commit

curl -X POST -d '{"id":"user-2","value":"Bob"}' \
  http://localhost:18000/v1/transactions/commit

# Retrieve them
curl http://localhost:18000/v1/transactions/user-1
curl http://localhost:18000/v1/transactions/user-2
```

## 3. Test MCP Server (Debugging Only)

**Important Note:** The MCP server is **automatically started by Claude Desktop** when you configure it. You do NOT need to start it manually for normal use.

The commands below are **only for debugging purposes** to verify the server can start without errors:

### Verify Server Can Start (Debugging)

```bash
# From the project directory
# This will start the server and wait for input on stdio
# Press Ctrl+C to stop it
python server.py
```

**Expected:** Server should start without errors and wait for MCP protocol messages on stdio. If you see errors, check your configuration and dependencies.

**Note:** In normal operation, Claude Desktop starts the server automatically. You only need to run this manually if you're debugging startup issues.

### Test with Python script

Create a test script to call the MCP server tools:

```bash
# Create a simple test script
cat > test_mcp_tools.py << 'EOF'
import asyncio
import json
from graphql_client import GraphQLClient

async def test_tools():
    client = GraphQLClient()
    
    # Test set operation
    print("Testing set operation...")
    result = await client.set_key_value("test-key", "test-value")
    print(f"Set result: {json.dumps(result, indent=2)}")
    
    # Test get operation
    print("\nTesting get operation...")
    result = await client.get_key_value("test-key")
    print(f"Get result: {json.dumps(result, indent=2)}")
    
    # Test getTransaction (requires valid transaction ID)
    # print("\nTesting getTransaction...")
    # result = await client.get_transaction("YOUR_TRANSACTION_ID")
    # print(f"GetTransaction result: {json.dumps(result, indent=2)}")

if __name__ == "__main__":
    asyncio.run(test_tools())
EOF

# Run the test script
python test_mcp_tools.py
```

## 4. Connect to Claude Desktop

**Important:** The MCP server is **automatically started by Claude Desktop** when you configure it. You do NOT need to manually start `server.py`. Claude Desktop will launch it automatically when needed.

### Step 1: Locate Claude Desktop Configuration

**macOS:**
```bash
# Configuration file location
~/Library/Application Support/Claude/claude_desktop_config.json
```

**Windows:**
```
%APPDATA%\Claude\claude_desktop_config.json
```

**Linux:**
```bash
~/.config/Claude/claude_desktop_config.json
```

### Step 2: Edit Configuration File

Open the configuration file and add the MCP server configuration:

```bash
# On macOS
open ~/Library/Application\ Support/Claude/claude_desktop_config.json

# Or edit with your preferred editor
nano ~/Library/Application\ Support/Claude/claude_desktop_config.json
```

### Step 3: Add MCP Server Configuration

Add this configuration to the `mcpServers` section:

```json
{
  "mcpServers": {
    "resilientdb": {
      "command": "python",
      "args": ["/absolute/path/to/ResilientDB-MCP/server.py"],
      "env": {
        "RESILIENTDB_GRAPHQL_URL": "http://localhost:8000/graphql",
        "RESILIENTDB_HTTP_URL": "http://localhost:18000"
      }
    }
  }
}
```

**Important:** Replace `/absolute/path/to/ResilientDB-MCP/server.py` with the actual absolute path to your server.py file.

**Example for macOS:**
```json
{
  "mcpServers": {
    "resilientdb": {
      "command": "python",
      "args": ["/Users/rahul/data/workspace/kanagrah/ResilientDB-MCP/server.py"],
      "env": {
        "RESILIENTDB_GRAPHQL_URL": "http://localhost:8000/graphql",
        "RESILIENTDB_HTTP_URL": "http://localhost:18000"
      }
    }
  }
}
```

### Step 4: Find Your Absolute Path

```bash
# Get the absolute path to server.py
pwd
# Output: /Users/rahul/data/workspace/kanagrah/ResilientDB-MCP

# Full path to server.py
realpath server.py
# Or
readlink -f server.py
```

### Step 5: Restart Claude Desktop

1. **Quit Claude Desktop completely** (not just close the window)
   - macOS: Cmd+Q or right-click dock icon → Quit
   - Windows: Close all windows and exit from system tray
   - Linux: Close all windows

2. **Restart Claude Desktop**
   - Claude Desktop will **automatically start the MCP server** when it launches
   - The server runs as a subprocess managed by Claude Desktop

3. **Verify Connection**
   - Open Claude Desktop
   - Check if the MCP server is connected (usually shown in the status or settings)
   - Look for any error messages in the Claude Desktop logs
   - If there are errors, check that the path to `server.py` is correct and Python is accessible

### Step 6: Test in Claude Desktop

Once connected, you can test the tools by asking Claude:

1. **Test set operation:**
   ```
   Store a key-value pair with key "test" and value "hello world"
   ```

2. **Test get operation:**
   ```
   Get the value for key "test"
   ```

3. **Test getTransaction:**
   ```
   Get transaction details for transaction ID "YOUR_TRANSACTION_ID"
   ```

4. **Test postTransaction:**
   ```
   Post a new asset transaction with operation "CREATE", amount 100, and asset data {"data": "test"}
   ```
   (Note: This will require you to provide the cryptographic keys)

## 5. Verify Server Configuration

### Check if server can start (Debugging Only)

**Note:** This is only for debugging. In normal use, Claude Desktop starts the server automatically.

```bash
# Run the server briefly to check for startup errors
# Press Ctrl+C immediately after it starts
timeout 2 python server.py 2>&1 || true
```

**Expected:** Server should start without import errors or configuration errors. If you see errors, fix them before configuring Claude Desktop.

### Check if dependencies are installed

```bash
# Verify all dependencies are installed
python -c "import mcp; import httpx; import dotenv; print('All dependencies OK')"
```

### Check environment variables

```bash
# Verify environment variables (if .env file exists)
python -c "from config import Config; print(f'GraphQL URL: {Config.GRAPHQL_URL}'); print(f'HTTP URL: {Config.HTTP_URL}')"
```

## 6. Troubleshooting

### Server won't start

```bash
# Check Python version (should be 3.11+)
python --version

# Check if MCP SDK is installed
pip list | grep mcp

# Install missing dependencies
pip install -r requirements.txt
```

### GraphQL connection errors

```bash
# Test GraphQL endpoint directly
curl -v http://localhost:8000/graphql

# Check if ResilientDB GraphQL server is running
netstat -an | grep 8000
# Or on macOS/Linux
lsof -i :8000
```

### HTTP connection errors

```bash
# Test HTTP endpoint directly
curl -v http://localhost:18000/v1/transactions/test

# Check if ResilientDB HTTP server is running
netstat -an | grep 18000
# Or on macOS/Linux
lsof -i :18000
```

### Claude Desktop connection issues

1. **Check Claude Desktop logs:**
   - macOS: `~/Library/Logs/Claude/`
   - Windows: `%APPDATA%\Claude\logs\`
   - Linux: `~/.config/Claude/logs/`

2. **Verify configuration file syntax:**
   ```bash
   # Validate JSON syntax
   python -m json.tool ~/Library/Application\ Support/Claude/claude_desktop_config.json
   ```

3. **Check file permissions:**
   ```bash
   # Make sure server.py is executable
   chmod +x server.py
   
   # Check if Python is in PATH
   which python
   ```

## 7. Quick Test Checklist

- [ ] GraphQL server responds on port 8000
- [ ] HTTP server responds on port 18000
- [ ] Can store key-value via HTTP API
- [ ] Can retrieve key-value via HTTP API
- [ ] MCP server can start without errors (for debugging)
- [ ] Claude Desktop configuration file is valid JSON
- [ ] Claude Desktop can connect to MCP server
- [ ] Can use `set` tool in Claude Desktop
- [ ] Can use `get` tool in Claude Desktop
- [ ] Can use `getTransaction` tool in Claude Desktop (with valid transaction ID)
- [ ] Can use `postTransaction` tool in Claude Desktop (with valid keys)

## Example Test Session

```bash
# 1. Start ResilientDB (if not already running)
# (Follow ResilientDB setup instructions)

# 2. Test HTTP API
curl -X POST -d '{"id":"demo-key","value":"demo value"}' \
  http://localhost:18000/v1/transactions/commit

curl http://localhost:18000/v1/transactions/demo-key

# 3. Test GraphQL
curl -X POST http://localhost:8000/graphql \
  -H "Content-Type: application/json" \
  -d '{"query": "{ __schema { queryType { name } } }"}'

# 4. Configure Claude Desktop
# (Edit claude_desktop_config.json as shown in section 4)
# Claude Desktop will automatically start the server when you restart it

# 5. Test in Claude Desktop
# (Ask Claude to use the tools)
```

## Additional Resources

- [ResilientDB Documentation](https://resilientdb.incubator.apache.org/)
- [MCP Protocol Documentation](https://modelcontextprotocol.io/)
- [Claude Desktop Setup Guide](https://docs.anthropic.com/claude/docs/claude-desktop)

