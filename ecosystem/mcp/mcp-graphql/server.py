"""MCP Server for ResilientDB - GraphQL integration."""
import asyncio
import json
import sys
import os
from typing import Any, Dict

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


def _setup_resilientdb_path() -> str:
    """Setup path to ResilientDB resdb_driver for key generation."""
    script_dir = os.path.dirname(os.path.abspath(__file__))
    
    possible_paths = [
        os.path.join(script_dir, '../../graphql/resdb_driver'),
        '/Users/rahul/data/workspace/kanagrah/incubator-resilientdb/ecosystem/graphql/resdb_driver',
        os.path.join(script_dir, '../graphql/resdb_driver'),
        os.path.join(script_dir, '../../../ecosystem/graphql/resdb_driver'),
    ]
    
    for path in possible_paths:
        abs_path = os.path.abspath(path)
        if os.path.exists(abs_path):
            if abs_path not in sys.path:
                sys.path.insert(0, abs_path)
            return abs_path
    
    raise ImportError(
        f"Could not find ResilientDB resdb_driver directory. "
        f"Tried: {', '.join([os.path.abspath(p) for p in possible_paths])}"
    )


def generate_keypairs() -> Dict[str, str]:
    """
    Generate Ed25519 keypairs for ResilientDB transactions.
    
    Returns:
        Dictionary with signer and recipient public/private keys
    """
    try:
        _setup_resilientdb_path()
        from crypto import generate_keypair
    except ImportError as e:
        raise ImportError(
            f"Could not import generate_keypair from ResilientDB crypto module: {e}"
        )
    
    signer = generate_keypair()
    recipient = generate_keypair()
    
    return {
        "signerPublicKey": signer.public_key,
        "signerPrivateKey": signer.private_key,
        "recipientPublicKey": recipient.public_key,
        "recipientPrivateKey": recipient.private_key
    }

# Create MCP server
app = Server("resilientdb-mcp")


@app.list_tools()
async def handle_list_tools() -> list[Tool]:
    """List all available tools."""
    return [
        Tool(
            name="generateKeys",
            description="Generate Ed25519 cryptographic keypairs (signer and recipient) for ResilientDB transactions. Returns signerPublicKey, signerPrivateKey, recipientPublicKey, and recipientPrivateKey. Use this tool to generate keys before creating transactions, or it will be automatically called when needed for postTransaction.",
            inputSchema={
                "type": "object",
                "properties": {},
                "required": []
            }
        ),
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
            description="Post a new asset transaction to ResilientDB using GraphQL (port 8000). Requires PrepareAsset with: operation (String), amount (Int), signerPublicKey (String), signerPrivateKey (String), recipientPublicKey (String), and asset (JSON). Returns CommitTransaction with transaction ID. If keys are not provided, automatically generate them using generateKeys tool first.",
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
                        "description": "Public key of the signer. If not provided, keys will be auto-generated."
                    },
                    "signerPrivateKey": {
                        "type": "string",
                        "description": "Private key of the signer. If not provided, keys will be auto-generated."
                    },
                    "recipientPublicKey": {
                        "type": "string",
                        "description": "Public key of the recipient. If not provided, keys will be auto-generated."
                    },
                    "asset": {
                        "description": "Asset data as JSON object."
                    }
                },
                "required": ["operation", "amount", "asset"]
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
        if name == "generateKeys":
            keys = generate_keypairs()
            return [TextContent(
                type="text",
                text=json.dumps({
                    "signerPublicKey": keys["signerPublicKey"],
                    "signerPrivateKey": keys["signerPrivateKey"],
                    "recipientPublicKey": keys["recipientPublicKey"],
                    "recipientPrivateKey": keys["recipientPrivateKey"],
                    "message": "Keys generated successfully. Use these keys with postTransaction tool."
                }, indent=2)
            )]
        
        elif name == "getTransaction":
            transaction_id = arguments["transactionId"]
            result = await graphql_client.get_transaction(transaction_id)
            return [TextContent(
                type="text",
                text=json.dumps(result, indent=2)
            )]
        
        elif name == "postTransaction":
            # Auto-generate keys if not provided or if any key is missing/empty
            required_keys = ["signerPublicKey", "signerPrivateKey", "recipientPublicKey"]
            if not all(k in arguments and arguments.get(k) for k in required_keys):
                keys = generate_keypairs()
                arguments["signerPublicKey"] = keys["signerPublicKey"]
                arguments["signerPrivateKey"] = keys["signerPrivateKey"]
                arguments["recipientPublicKey"] = keys["recipientPublicKey"]
            
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

