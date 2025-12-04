"""MCP Server for ResilientDB - GraphQL integration."""
import asyncio
import json
import sys
from typing import Any

try:
    from mcp.server import Server
    from mcp.server.stdio import stdio_server
    from mcp.types import Tool, TextContent
except ImportError:
    print("Error: MCP SDK not found. Please install it with: pip install mcp", file=sys.stderr)
    sys.exit(1)

from config import Config
from graphql_client import GraphQLClient

# Initialize clients
graphql_client = GraphQLClient()

# Create MCP server
app = Server("resilientdb-mcp")


@app.list_tools()
async def handle_list_tools() -> list[Tool]:
    """List all available tools."""
    return [
        Tool(
            name="getTransaction",
            description="Get asset transaction details by transaction ID using GraphQL (port 8000). Returns RetrieveTransaction with id, version, amount, uri, type, publicKey, operation, metadata, asset, and signerPublicKey.",
            inputSchema={
                "type": "object",
                "properties": {
                    "transactionId": {
                        "type": "string",
                        "description": "Transaction ID to retrieve."
                    }
                },
                "required": ["transactionId"]
            }
        ),
        Tool(
            name="postTransaction",
            description="Post a new asset transaction to ResilientDB using GraphQL (port 8000). Requires PrepareAsset with: operation (String), amount (Int), signerPublicKey (String), signerPrivateKey (String), recipientPublicKey (String), and asset (JSON). Returns CommitTransaction with transaction ID.",
            inputSchema={
                "type": "object",
                "properties": {
                    "operation": {
                        "type": "string",
                        "description": "Transaction operation type (e.g., 'CREATE', 'TRANSFER')."
                    },
                    "amount": {
                        "type": "integer",
                        "description": "Transaction amount (integer)."
                    },
                    "signerPublicKey": {
                        "type": "string",
                        "description": "Public key of the signer."
                    },
                    "signerPrivateKey": {
                        "type": "string",
                        "description": "Private key of the signer."
                    },
                    "recipientPublicKey": {
                        "type": "string",
                        "description": "Public key of the recipient."
                    },
                    "asset": {
                        "description": "Asset data as JSON object."
                    }
                },
                "required": ["operation", "amount", "signerPublicKey", "signerPrivateKey", "recipientPublicKey", "asset"]
            }
        ),
        Tool(
            name="get",
            description="Retrieves a value from ResilientDB by key using HTTP REST API (Crow server on port 18000).",
            inputSchema={
                "type": "object",
                "properties": {
                    "key": {
                        "type": "string",
                        "description": "Key to retrieve."
                    }
                },
                "required": ["key"]
            }
        ),
        Tool(
            name="set",
            description="Stores a key-value pair in ResilientDB using HTTP REST API (Crow server on port 18000).",
            inputSchema={
                "type": "object",
                "properties": {
                    "key": {
                        "type": "string",
                        "description": "Key to store the value under."
                    },
                    "value": {
                        "description": "Value to store (can be any JSON-serializable value)."
                    }
                },
                "required": ["key", "value"]
            }
        )
    ]


@app.call_tool()
async def handle_call_tool(name: str, arguments: dict[str, Any] | None) -> list[TextContent]:
    """Handle tool calls and route to appropriate services."""
    if arguments is None:
        arguments = {}
    
    try:
        if name == "getTransaction":
            transaction_id = arguments["transactionId"]
            result = await graphql_client.get_transaction(transaction_id)
            return [TextContent(
                type="text",
                text=json.dumps(result, indent=2)
            )]
        
        elif name == "postTransaction":
            # Build PrepareAsset from individual arguments
            data = {
                "operation": arguments["operation"],
                "amount": arguments["amount"],
                "signerPublicKey": arguments["signerPublicKey"],
                "signerPrivateKey": arguments["signerPrivateKey"],
                "recipientPublicKey": arguments["recipientPublicKey"],
                "asset": arguments["asset"]
            }
            result = await graphql_client.post_transaction(data)
            return [TextContent(
                type="text",
                text=json.dumps(result, indent=2)
            )]
        
        elif name == "get":
            key = arguments["key"]
            result = await graphql_client.get_key_value(key)
            return [TextContent(
                type="text",
                text=json.dumps(result, indent=2)
            )]
        
        elif name == "set":
            key = arguments["key"]
            value = arguments["value"]
            result = await graphql_client.set_key_value(key, value)
            return [TextContent(
                type="text",
                text=json.dumps(result, indent=2)
            )]
        
        else:
            raise ValueError(f"Unknown tool: {name}")
    
    except Exception as e:
        error_message = f"Error executing tool '{name}': {str(e)}"
        error_details = {
            "error": type(e).__name__,
            "message": error_message,
            "tool": name,
            "arguments": arguments
        }
        return [TextContent(
            type="text",
            text=json.dumps(error_details, indent=2)
        )]


async def main():
    """Main entry point for the MCP server."""
    # Run the server using stdio transport
    # The stdio_server context manager returns (read_stream, write_stream)
    async with stdio_server() as (read_stream, write_stream):
        await app.run(
            read_stream,
            write_stream,
            app.create_initialization_options()
        )


if __name__ == "__main__":
    asyncio.run(main())

