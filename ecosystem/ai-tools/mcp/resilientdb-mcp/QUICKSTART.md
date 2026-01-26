# Quick Start Guide

## Prerequisites

1. **Python 3.11+** installed
2. **ResilientDB** instance running (see [ResilientDB Installation](https://github.com/apache/incubator-resilientdb))
3. **ResContract CLI** installed (for smart contract operations)
4. **Claude Desktop** (for testing with MCP)

## Installation Steps

### 1. Clone and Setup

```bash
git clone https://github.com/rahulkanagaraj786/ResilientDB-MCP.git
cd ResilientDB-MCP
```

### 2. Install Dependencies

```bash
pip install -r requirements.txt
```

### 3. Configure Environment

```bash
cp .env.example .env
# Edit .env with your ResilientDB settings
```

Update `.env`:
```env
RESILIENTDB_GRAPHQL_URL=http://localhost:9000/graphql
RESCONTRACT_CLI_PATH=rescontract
```

### 4. Test the Server

Run the server directly:
```bash
python server.py
```

The server should start and listen on stdio for MCP protocol messages.

## Docker Setup

### Build Docker Image

```bash
docker build -t mcp/resilientdb -f Dockerfile .
```

### Run Docker Container

```bash
docker run -i --rm mcp/resilientdb
```

## Claude Desktop Configuration

### 1. Locate Claude Desktop Config

- **macOS**: `~/Library/Application Support/Claude/claude_desktop_config.json`
- **Windows**: `%APPDATA%\Claude\claude_desktop_config.json`
- **Linux**: `~/.config/Claude/claude_desktop_config.json`

### 2. Add MCP Server Configuration

Edit the config file and add:

```json
{
  "mcpServers": {
    "resilientdb": {
      "command": "python",
      "args": ["/absolute/path/to/ResilientDB-MCP/server.py"],
      "env": {
        "RESILIENTDB_GRAPHQL_URL": "http://localhost:9000/graphql",
        "RESCONTRACT_CLI_PATH": "rescontract"
      }
    }
  }
}
```

Or with Docker:

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

### 3. Restart Claude Desktop

Restart Claude Desktop to load the new MCP server configuration.

## Testing

### Test Key-Value Operations

In Claude Desktop, you can now ask:

- "Store a key-value pair with key 'test' and value 'hello'"
- "Get the value for key 'test'"

### Test Smart Contract Operations

- "Compile a contract at /path/to/contract.sol"
- "Deploy the compiled contract"
- "Execute the getValue method on contract 0x123..."

### Test Account Operations

- "Create a new account in ResilientDB"
- "Get transaction details for tx-123456"

## Troubleshooting

### Server Not Starting

1. Check Python version: `python --version` (should be 3.11+)
2. Verify dependencies: `pip list | grep mcp`
3. Check environment variables in `.env`

### Connection Errors

1. Verify ResilientDB is running
2. Check GraphQL URL is correct
3. Test GraphQL endpoint: `curl http://localhost:9000/graphql`

### ResContract CLI Not Found

1. Verify ResContract is installed
2. Check PATH: `which rescontract`
3. Set `RESCONTRACT_CLI_PATH` in `.env`

### Claude Desktop Not Connecting

1. Check Claude Desktop logs
2. Verify config file syntax (valid JSON)
3. Ensure absolute path to server.py is correct
4. Check file permissions

## Next Steps

1. Read the [README.md](README.md) for detailed documentation
2. Review [ARCHITECTURE.md](ARCHITECTURE.md) for architecture details
3. Explore the available tools in Claude Desktop
4. Start building with ResilientDB!

## Support

For issues or questions:
- Check the [ResilientDB Documentation](https://resilientdb.incubator.apache.org/)
- Review [ResContract CLI Docs](https://beacon.resilientdb.com/docs/rescontract)
- Check [ResilientDB GraphQL API](https://beacon.resilientdb.com/docs/resilientdb_graphql)

