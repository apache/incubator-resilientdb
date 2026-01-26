# ResilientDB MCP Server Architecture

## Overview

The ResilientDB MCP Server is a Model Context Protocol (MCP) implementation that provides a standardized interface for AI agents to interact with ResilientDB blockchain. The server integrates GraphQL for asset transactions and HTTP REST API for key-value operations.

## Architecture Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                        MCP Host (Claude Desktop)                 │
└────────────────────────────┬────────────────────────────────────┘
                             │
                             │ MCP Protocol (stdio)
                             │
┌────────────────────────────▼────────────────────────────────────┐
│                    ResilientDB MCP Server                        │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │                    MCP Server Core                        │  │
│  │  - Tool Registration                                     │  │
│  │  - Request Routing                                       │  │
│  │  - Error Handling                                        │  │
│  └──────────────────────────────────────────────────────────┘  │
│                             │                                    │
│        ┌────────────────────┼────────────────────┐              │
│        │                    │                    │              │
│  ┌─────▼──────┐    ┌────────▼────────┐  ┌───────▼───────┐     │
│  │ GraphQL    │    │ HTTP REST       │  │ Configuration │     │
│  │ Client     │    │ Client          │  │ Manager       │     │
│  │ (Port 8000)│    │ (Port 18000)    │  │               │     │
│  └─────┬──────┘    └────────┬────────┘  └───────────────┘     │
│        │                    │                                    │
└────────┼────────────────────┼────────────────────────────────────┘
         │                    │
         │ HTTP/GraphQL       │ HTTP REST
         │                    │
┌────────▼────────────────────▼────────────────────────────────────┐
│                    ResilientDB Backend                            │
│  ┌──────────────────┐              ┌──────────────────────┐     │
│  │ GraphQL Server   │              │ HTTP/Crow Server     │     │
│  │ (Port 8000)      │              │ (Port 18000)         │     │
│  │ - Asset Txns     │              │ - Key-Value Ops      │     │git 
│  └──────────────────┘              └──────────────────────┘     │
│                                                                   │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │              ResilientDB Blockchain                       │   │
│  └──────────────────────────────────────────────────────────┘   │
└───────────────────────────────────────────────────────────────────┘
```

## Components

### 1. MCP Server Core (`server.py`)

The main server component that:
- Registers all available tools with the MCP protocol
- Handles incoming tool calls from MCP hosts
- Routes requests to appropriate clients (GraphQL or HTTP REST)
- Manages error handling and response formatting

**Key Responsibilities:**
- Tool registration and discovery
- Request validation
- Response formatting
- Error handling and reporting

**Available Tools:**
- `getTransaction` - Get asset transaction by ID (GraphQL)
- `postTransaction` - Post asset transaction (GraphQL)
- `get` - Retrieve key-value pair (HTTP REST)
- `set` - Store key-value pair (HTTP REST)

### 2. GraphQL Client (`graphql_client.py`)

Handles GraphQL-based operations for asset transactions and HTTP REST operations for key-value storage.

**GraphQL Operations (Port 8000):**
- `get_transaction(transaction_id)` - Retrieve asset transaction by ID
- `post_transaction(data)` - Submit new asset transaction with PrepareAsset format

**HTTP REST Operations (Port 18000):**
- `get_key_value(key)` - Retrieve key-value pair via HTTP REST API
- `set_key_value(key, value)` - Store key-value pair via HTTP REST API

**Key Features:**
- Async HTTP client for both GraphQL and REST operations
- Error handling and validation
- Request timeout management
- Response parsing and formatting

### 3. Configuration Manager (`config.py`)

Manages server configuration through environment variables.

**Configuration Options:**
- `RESILIENTDB_GRAPHQL_URL` - GraphQL endpoint (default: `http://localhost:8000/graphql`)
- `RESILIENTDB_HTTP_URL` - HTTP/Crow server endpoint (default: `http://localhost:18000`)
- `RESILIENTDB_API_KEY` - Optional API key for authentication
- `RESILIENTDB_AUTH_TOKEN` - Optional auth token
- `REQUEST_TIMEOUT` - Request timeout in seconds (default: 30)

## Request Flow

### Asset Transaction Operations (GraphQL)

1. **MCP Host** sends tool call (e.g., `getTransaction`)
2. **MCP Server** receives and validates request
3. **GraphQL Client** constructs GraphQL query
4. **GraphQL Server** (port 8000) processes request
5. **Response** flows back through the chain

**Example Flow:**
```
Claude Desktop → MCP Server → GraphQL Client → GraphQL Server (8000) → ResilientDB
```

### Key-Value Operations (HTTP REST)

1. **MCP Host** sends tool call (e.g., `set`)
2. **MCP Server** receives and validates request
3. **GraphQL Client** (HTTP REST methods) constructs HTTP request
4. **HTTP/Crow Server** (port 18000) processes request
5. **Response** flows back through the chain

**Example Flow:**
```
Claude Desktop → MCP Server → HTTP REST Client → Crow Server (18000) → ResilientDB
```

## Routing Logic

The server uses operation-based routing:

| Operation | Service | Port | Purpose |
|-----------|---------|------|---------|
| `getTransaction` | GraphQL | 8000 | Get asset transaction by ID |
| `postTransaction` | GraphQL | 8000 | Post asset transaction |
| `get` | HTTP REST | 18000 | Retrieve key-value pair |
| `set` | HTTP REST | 18000 | Store key-value pair |

**Note:** All routing is direct with no fallback mechanisms.

## Data Flow

### Request Processing

```
MCP Request → Validation → Route Selection → Client Execution → Response Formatting → MCP Response
```

### GraphQL Request Flow

```
1. MCP Tool Call (getTransaction/postTransaction)
2. Server validates arguments
3. GraphQL Client constructs query/mutation
4. HTTP POST to GraphQL endpoint (port 8000)
5. Parse GraphQL response
6. Format and return to MCP host
```

### HTTP REST Request Flow

```
1. MCP Tool Call (get/set)
2. Server validates arguments
3. HTTP Client constructs REST request
4. HTTP GET/POST to Crow server (port 18000)
5. Parse HTTP response
6. Format and return to MCP host
```

### Response Format

All responses are formatted as JSON:
```json
{
  "data": {...},
  "error": null
}
```

Error responses:
```json
{
  "error": {
    "type": "ErrorType",
    "message": "Error message",
    "details": {...}
  }
}
```

## Service Separation

### GraphQL Server (Port 8000)

**Purpose:** Blockchain asset transactions

**Operations:**
- `getTransaction(id: ID!)` - Retrieve asset transaction
- `postTransaction(data: PrepareAsset!)` - Create asset transaction

**Schema:**
- Query: `getTransaction` returns `RetrieveTransaction`
- Mutation: `postTransaction` accepts `PrepareAsset` and returns `CommitTransaction`

**Required Fields for postTransaction:**
- `operation` (String) - Transaction operation type
- `amount` (Int) - Transaction amount
- `signerPublicKey` (String) - Signer's public key
- `signerPrivateKey` (String) - Signer's private key
- `recipientPublicKey` (String) - Recipient's public key
- `asset` (JSONScalar) - Asset data as JSON

### HTTP/Crow Server (Port 18000)

**Purpose:** Simple key-value storage

**Operations:**
- `POST /v1/transactions/commit` - Store key-value pair
- `GET /v1/transactions/{key}` - Retrieve key-value pair

**Request Format (set):**
```json
{
  "id": "key",
  "value": "value"
}
```

**Response Format (set):**
```
id: key
```

**Response Format (get):**
```json
{
  "id": "key",
  "value": "value"
}
```

## Error Handling

### Error Types

1. **Configuration Errors**: Missing or invalid configuration
2. **GraphQL Errors**: API request failures, query errors, missing fields
3. **HTTP Errors**: Connection failures, invalid responses
4. **Network Errors**: Connection timeouts, unreachable services
5. **Validation Errors**: Invalid parameters, missing required fields

### Error Flow

1. Error occurs in client layer
2. Exception is caught and formatted
3. Error details are included in response
4. MCP host receives structured error response

**Example Error Response:**
```json
{
  "error": "GraphQLError",
  "message": "Missing required fields in PrepareAsset: signerPublicKey, signerPrivateKey",
  "tool": "postTransaction",
  "arguments": {...}
}
```

## Security Considerations

1. **Authentication**: Optional API keys and tokens via environment variables
2. **Input Validation**: All inputs are validated before processing
3. **Error Messages**: Sensitive information is not exposed in error messages
4. **Network Security**: HTTPS should be used for production endpoints
5. **Private Keys**: Never expose private keys in logs or error messages

## Performance Considerations

1. **Async Operations**: All I/O operations are asynchronous
2. **Connection Pooling**: HTTP clients use connection pooling
3. **Timeout Management**: Configurable timeouts prevent hanging requests
4. **Resource Management**: Proper cleanup of resources

## Implementation Details

### GraphQL Integration

- Asset transaction queries (`getTransaction`)
- Asset transaction mutations (`postTransaction`)
- Full PrepareAsset support with validation

### HTTP REST Integration

- Key-value storage (`set`)
- Key-value retrieval (`get`)
- Direct HTTP API calls to Crow server

### MCP Protocol

- Full MCP server implementation
- Tool registration and discovery
- Error handling and response formatting

## Extension Points

The architecture supports extension through:

1. **New Tools**: Add new tools by registering them in `server.py`
2. **New Clients**: Add new client implementations for additional services
3. **Custom Routing**: Modify routing logic in `server.py`
4. **Middleware**: Add middleware for logging, metrics, etc.

## Testing Strategy

1. **Unit Tests**: Test individual components in isolation
2. **Integration Tests**: Test component interactions
3. **End-to-End Tests**: Test complete request flows
4. **Mock Services**: Use mocks for external dependencies
5. **Error Scenarios**: Test error handling and recovery

## Deployment Considerations

1. **Docker**: Containerized deployment for consistency
2. **Environment Variables**: Configuration via environment variables
3. **Health Checks**: Monitor server health and status
4. **Logging**: Comprehensive logging for debugging

## Future Enhancements

1. **Caching**: Add caching layer for frequently accessed data
2. **Rate Limiting**: Implement rate limiting for API calls
3. **Batch Operations**: Support for batch operations
4. **Metrics**: Detailed metrics and analytics
5. **Enhanced Authentication**: More robust authentication mechanisms

## Project Location

This project would be located in the ResilientDB ecosystem directory structure as:

```
ecosystem/
└── tools/
    └── resilientdb-mcp/          # This MCP server project
        ├── server.py
        ├── graphql_client.py
        ├── config.py
        ├── requirements.txt
        └── ...
```

**Rationale:**
- It's a development tool that enables AI agents to interact with ResilientDB
- It fits alongside other tools like `resvault` and `create-resilient-app`
- It's not a service (like GraphQL server) or SDK (like resdb-orm)
- It's a tool that bridges MCP protocol with ResilientDB services

## References

- [ResilientDB GraphQL Documentation](http://beacon.resilientdb.com/docs/resilientdb_graphql)
- [ResilientDB GraphQL GitHub](https://github.com/apache/incubator-resilientdb-graphql)
- [MCP Protocol Documentation](https://modelcontextprotocol.io/)
- [ResilientDB Main Repository](https://github.com/apache/incubator-resilientdb)
