# ResilientDB MCP Server

A Model Context Protocol (MCP) server for interacting with ResilientDB, a high-performance blockchain platform. This server allows Large Language Models (LLMs) like Claude to interact with ResilientDB through GraphQL queries and HTTP REST API.

## Overview

This MCP server bridges the gap between AI agents (like Claude Desktop) and ResilientDB by providing a standardized interface for:
- **GraphQL Operations**: Asset transactions on the blockchain (port 8000)
- **HTTP REST API Operations**: Key-value storage operations (port 18000 - Crow server)

**Note:** For midterm, this implementation focuses on GraphQL and HTTP REST API integration. Smart contract operations (ResContract CLI) are temporarily disabled.

## Features

### GraphQL Operations (Port 8000)
- `createAccount`: Create new accounts in ResilientDB (if supported)
- `getTransaction`: Retrieve asset transaction details by ID (blockchain transactions)
- `postTransaction`: Post new asset transactions to the blockchain (requires PrepareAsset with crypto keys)
- `updateTransaction`: Update existing transactions (note: blockchain transactions are typically immutable)

### Key-Value Operations (Port 18000 - HTTP REST API)
- `get`: Retrieve values by key using HTTP REST API (Crow server)
- `set`: Store key-value pairs using HTTP REST API (Crow server)

**Note:** For midterm, smart contract operations (compile, deploy, execute) are temporarily removed. Focus is on GraphQL and HTTP REST API integration.

**Important Architecture Notes:**
- **GraphQL (port 8000)**: Used for blockchain asset transactions
- **HTTP/Crow (port 18000)**: Used for key-value operations

## Installation

### Prerequisites

- Python 3.11 or higher
- ResilientDB instance running (see [ResilientDB Installation](https://github.com/apache/incubator-resilientdb))
- ResContract CLI installed (for smart contract operations)
- Access to ResilientDB GraphQL endpoint

### Local Installation

1. Clone the repository:
```bash
git clone https://github.com/rahulkanagaraj786/ResilientDB-MCP.git
cd ResilientDB-MCP
```

2. Install dependencies:
```bash
pip install -r requirements.txt
```

3. Configure environment variables:
```bash
cp .env.example .env
# Edit .env with your ResilientDB configuration
```

4. Update `.env` file with your settings:
```env
RESILIENTDB_GRAPHQL_URL=http://localhost:8000/graphql
RESILIENTDB_HTTP_URL=http://localhost:18000
```

### Docker Installation

1. Build the Docker image:
```bash
docker build -t mcp/resilientdb -f Dockerfile .
```

2. Run the container:
```bash
docker run -i --rm mcp/resilientdb
```

## Configuration

### Environment Variables

| Variable | Description | Default |
|----------|-------------|---------|
| `RESILIENTDB_GRAPHQL_URL` | GraphQL endpoint URL (port 8000 for asset transactions) | `http://localhost:8000/graphql` |
| `RESILIENTDB_HTTP_URL` | HTTP/Crow server URL (port 18000 for KV operations) | `http://localhost:18000` |
| `RESILIENTDB_API_KEY` | Optional API key for authentication | None |
| `RESILIENTDB_AUTH_TOKEN` | Optional auth token | None |
| `REQUEST_TIMEOUT` | Request timeout in seconds | `30` |
| `TRANSACTION_POLL_INTERVAL` | Polling interval for transactions | `1.0` |
| `MAX_POLL_ATTEMPTS` | Maximum polling attempts | `30` |

**Important Notes:**
- GraphQL (port 8000) is used for **asset transactions** (blockchain)
- HTTP/Crow (port 18000) is used for **key-value operations** (simple storage)

## Usage with Claude Desktop

Add the MCP server to your Claude Desktop configuration:

1. Open Claude Desktop settings
2. Edit the MCP servers configuration file (usually `claude_desktop.json`)
3. Add the following configuration:

### For Local Installation:
```json
{
  "mcpServers": {
    "resilientdb": {
      "command": "python",
      "args": ["/path/to/ResilientDB-MCP/server.py"],
      "env": {
        "RESILIENTDB_GRAPHQL_URL": "http://localhost:8000/graphql",
        "RESILIENTDB_HTTP_URL": "http://localhost:18000"
      }
    }
  }
}
```

### For Docker Installation:
```json
{
  "mcpServers": {
    "resilientdb": {
      "command": "docker",
      "args": ["run", "-i", "--rm", "mcp/resilientdb"]
    }
  }
}
```

4. Restart Claude Desktop

## Available Tools

### createAccount
Create a new account in ResilientDB.

**Parameters:**
- `accountId` (optional): Account ID. If not provided, server will generate one.

**Example:**
```json
{
  "accountId": "my-account-123"
}
```

### getTransaction
Get asset transaction details by transaction ID (GraphQL - port 8000).

**Parameters:**
- `transactionId` (required): Transaction ID to retrieve

**Example:**
```json
{
  "transactionId": "tx-123456"
}
```

**Note:** This is for blockchain asset transactions, not KV transactions.

### postTransaction
Post a new asset transaction to ResilientDB (GraphQL - port 8000).

**Parameters:**
- `data` (required): Transaction data in PrepareAsset format with crypto keys and signatures

**Example:**
```json
{
  "data": {
    "operation": "CREATE",
    "asset": {
      "data": {...}
    },
    "outputs": [...],
    "inputs": [...]
  }
}
```

**Note:** This requires PrepareAsset format with cryptographic keys. For simple KV operations, use the `set` tool instead.

### updateTransaction
Update an existing transaction.

**Parameters:**
- `transactionId` (required): Transaction ID to update
- `data` (required): Updated transaction data

**Example:**
```json
{
  "transactionId": "tx-123456",
  "data": {
    "status": "completed"
  }
}
```

### get
Retrieve a value from ResilientDB by key (HTTP REST API - port 18000).

**Parameters:**
- `key` (required): Key to retrieve

**Example:**
```json
{
  "key": "my-key"
}
```

**Note:** This uses HTTP REST API (Crow server on port 18000).

### set
Store a key-value pair in ResilientDB (HTTP REST API - port 18000).

**Parameters:**
- `key` (required): Key to store
- `value` (required): Value to store (can be any JSON-serializable value)

**Example:**
```json
{
  "key": "my-key",
  "value": "my-value"
}
```

**Note:** This uses HTTP REST API (Crow server on port 18000).

## Architecture

The MCP server acts as a mediator between the MCP host (Claude Desktop) and ResilientDB backend services:

```
┌─────────────┐         ┌──────────────┐         ┌─────────────┐
│ Claude      │────────▶│ MCP Server   │────────▶│ ResilientDB │
│ Desktop     │         │ (Python)     │         │ Backend     │
└─────────────┘         └──────────────┘         └─────────────┘
                              │
                              ├──▶ GraphQL Client (port 8000)
                              │    (Asset Transactions only)
                              │
                              └──▶ HTTP REST Client (port 18000)
                                   (Key-Value Operations)
```

### Routing Logic

The server automatically routes requests to the appropriate service:
- **Asset Transactions** → GraphQL API (port 8000)
  - `getTransaction`: Retrieve asset transactions
  - `postTransaction`: Post asset transactions (requires PrepareAsset)
  - `createAccount`: Create accounts (if supported)
  - `updateTransaction`: Update transactions (if supported)

- **Key-Value Operations** → HTTP REST API (port 18000 - Crow server)
  - `get`: Retrieve key-value pairs
  - `set`: Store key-value pairs

**Important:** KV operations use HTTP REST API (port 18000).

## Development

### Project Structure

```
ResilientDB-MCP/
├── server.py              # Main MCP server implementation
├── graphql_client.py      # GraphQL client for ResilientDB
├── rescontract_client.py  # ResContract CLI client
├── config.py              # Configuration management
├── requirements.txt       # Python dependencies
├── Dockerfile             # Docker configuration
└── README.md              # This file
```

### Running Tests

```bash
# Install test dependencies
pip install pytest pytest-asyncio

# Run tests
pytest
```

### Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Submit a pull request

## Troubleshooting

### ResContract CLI Not Found

If you get an error about ResContract CLI not being found:
1. Ensure ResContract CLI is installed
2. Add it to your PATH, or
3. Set `RESCONTRACT_CLI_PATH` environment variable to the full path

### GraphQL Connection Errors

If you encounter GraphQL connection errors:
1. Verify ResilientDB is running
2. Check the `RESILIENTDB_GRAPHQL_URL` is correct (should be port 8000, not 9000)
3. Ensure network connectivity to the GraphQL endpoint
4. Check firewall settings
5. Verify GraphQL server is accessible: `curl http://localhost:8000/graphql`

### HTTP Connection Errors

If you encounter HTTP connection errors for KV operations:
1. Verify Crow HTTP server is running on port 18000
2. Check the `RESILIENTDB_HTTP_URL` is correct
3. Test HTTP endpoint: `curl http://localhost:18000/v1/transactions/test`
4. Ensure the HTTP server is accessible

### Key-Value Operations Not Working

If KV operations (get/set) fail:
1. Verify you're using HTTP REST API (port 18000), not GraphQL
2. Check that Crow HTTP server is running
3. Test with curl:
   ```bash
   # Set a value
   curl -X POST -d '{"id":"test","value":"hello"}' http://localhost:18000/v1/transactions/commit
   
   # Get a value
   curl http://localhost:18000/v1/transactions/test
   ```
4. Verify HTTP REST API (port 18000) is accessible for KV operations

### Transaction Timeouts

If transactions timeout:
1. Increase `REQUEST_TIMEOUT` in `.env`
2. Check ResilientDB blockchain status
3. Verify network latency

## Key Architecture Insights

### Service Separation

ResilientDB uses different services for different operations:

1. **GraphQL Server (Port 8000)**
   - Purpose: Blockchain asset transactions
   - Operations: `getTransaction`, `postTransaction` (with PrepareAsset)

2. **HTTP/Crow Server (Port 18000)**
   - Purpose: Simple key-value storage
   - Operations: `get`, `set` (via REST API)
   - Endpoints: 
     - `POST /v1/transactions/commit` (for set)
     - `GET /v1/transactions/{key}` (for get)

### Why This Matters

- **KV operations use HTTP REST API** (port 18000) for `set`/`get` operations
- **Asset transactions use GraphQL** (port 8000) and require PrepareAsset format
- **Wrong port numbers** (e.g., 9000 instead of 8000) will cause connection errors

## References

- [ResilientDB GitHub](https://github.com/apache/incubator-resilientdb)
- [ResilientDB Documentation](https://resilientdb.incubator.apache.org/)
- [ResilientDB GraphQL API](https://beacon.resilientdb.com/docs/resilientdb_graphql)
- [ResilientDB Quick Start](https://quickstart.resilientdb.com/)
- [MCP Protocol Documentation](https://modelcontextprotocol.io/)

## License

Apache 2.0 License

## Authors

Team 10 - ECS 265 Project
