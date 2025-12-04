"""MCP Server for ResilientDB - GraphQL integration."""
import asyncio
import json
import sys
import os
import time
from datetime import datetime
from typing import Any, Dict
import httpx

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


async def send_monitoring_data(tool_name: str, args: dict, result: Any, duration: float):
    """Send monitoring data to ResLens middleware."""
    try:
        async with httpx.AsyncClient() as client:
            await client.post(
                "http://localhost:3000/api/v1/mcp/prompts",
                json={
                    "tool": tool_name,
                    "args": args,
                    "result": str(result)[:1000] if result else "None",
                    "timestamp": datetime.now().isoformat(),
                    "duration": duration,
                    "resdb_metrics": {}
                },
                timeout=5.0
            )
    except Exception as e:
        print(f"Failed to send monitoring data to ResLens: {e}", file=sys.stderr)


async def analyze_transactions(transaction_ids: list[str]) -> Dict[str, Any]:
    """
    Analyze a set of transactions and compute summary statistics.
    
    Args:
        transaction_ids: List of transaction IDs to analyze
        
    Returns:
        Dictionary with summary statistics and raw transaction data
    """
    transactions = []
    errors = []
    
    # Fetch all transactions
    for tx_id in transaction_ids:
        try:
            result = await graphql_client.get_transaction(tx_id)
            # Extract the actual transaction data from GraphQL response
            tx_data = result.get("getTransaction", {})
            if tx_data:
                transactions.append(tx_data)
        except Exception as e:
            errors.append({
                "transactionId": tx_id,
                "error": str(e)
            })
    
    if not transactions:
        return {
            "summary": {
                "total": 0,
                "successful": 0,
                "failed": len(errors),
                "message": "No transactions could be retrieved"
            },
            "transactions": [],
            "errors": errors
        }
    
    # Compute statistics
    total = len(transactions)
    amounts = []
    operations = {}
    types = set()
    signers = set()
    public_keys = set()
    
    for tx in transactions:
        # Collect amounts
        if "amount" in tx and tx["amount"] is not None:
            try:
                amounts.append(int(tx["amount"]))
            except (ValueError, TypeError):
                pass
        
        # Count operations
        op = tx.get("operation", "UNKNOWN")
        operations[op] = operations.get(op, 0) + 1
        
        # Collect types
        if "type" in tx and tx["type"]:
            types.add(str(tx["type"]))
        
        # Collect signers
        if "signerPublicKey" in tx and tx["signerPublicKey"]:
            signers.add(str(tx["signerPublicKey"]))
        
        # Collect public keys
        if "publicKey" in tx and tx["publicKey"]:
            public_keys.add(str(tx["publicKey"]))
    
    # Build summary
    summary = {
        "total": total,
        "successful": len(transactions),
        "failed": len(errors),
        "byOperation": operations,
        "distinctTypes": list(types),
        "distinctSigners": len(signers),
        "distinctPublicKeys": len(public_keys)
    }
    
    # Add amount statistics if available
    if amounts:
        summary["amountStats"] = {
            "min": min(amounts),
            "max": max(amounts),
            "average": sum(amounts) / len(amounts),
            "total": sum(amounts),
            "count": len(amounts)
        }
    
    return {
        "summary": summary,
        "transactions": transactions,
        "errors": errors
    }


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


def _setup_sha3_shim():
    """Setup sha3 module shim using Python's built-in hashlib for Python 3.11+."""
    import hashlib
    import sys
    from types import ModuleType
    
    class SHA3_256:
        """SHA3-256 hash implementation using Python's built-in hashlib."""
        
        def __init__(self, data=None):
            """Initialize SHA3-256 hash object."""
            self._hash = hashlib.sha3_256()
            if data is not None:
                if isinstance(data, str):
                    data = data.encode('utf-8')
                self._hash.update(data)
        
        def update(self, data):
            """Update the hash with additional data."""
            if isinstance(data, str):
                data = data.encode('utf-8')
            self._hash.update(data)
        
        def hexdigest(self):
            """Return the hexadecimal digest of the hash."""
            return self._hash.hexdigest()
        
        def digest(self):
            """Return the binary digest of the hash."""
            return self._hash.digest()
    
    # Create a factory function that returns instances
    def sha3_256(data=None):
        """Factory function for SHA3-256 hash objects."""
        return SHA3_256(data)
    
    # Create a fake sha3 module and inject it into sys.modules
    sha3_module = ModuleType('sha3')
    sha3_module.sha3_256 = sha3_256
    
    # Only inject if sha3 is not already available
    if 'sha3' not in sys.modules:
        sys.modules['sha3'] = sha3_module


def generate_keypairs() -> Dict[str, str]:
    """
    Generate Ed25519 keypairs for ResilientDB transactions.
    
    Returns:
        Dictionary with signer and recipient public/private keys
    """
    try:
        _setup_resilientdb_path()
        # Setup sha3 shim before importing crypto (which imports sha3)
        _setup_sha3_shim()
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
            name="analyzeTransactions",
            description="Analyze a set of transactions by their IDs and compute summary statistics. Returns summary with counts by operation type, amount statistics (min/max/average), distinct types, signers, and public keys. Also returns raw transaction data and any errors encountered. Useful for understanding transaction patterns and identifying outliers.",
            inputSchema={
                "type": "object",
                "properties": {
                    "transactionIds": {
                        "type": "array",
                        "items": {
                            "type": "string"
                        },
                        "description": "List of transaction IDs to analyze (maximum 20 transactions recommended).",
                        "minItems": 1,
                        "maxItems": 20
                    }
                },
                "required": ["transactionIds"]
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
    
    start_time = time.time()
    result = None
    try:
        if name == "generateKeys":
            keys = generate_keypairs()
            result = {
                "signerPublicKey": keys["signerPublicKey"],
                "signerPrivateKey": keys["signerPrivateKey"],
                "recipientPublicKey": keys["recipientPublicKey"],
                "recipientPrivateKey": keys["recipientPrivateKey"],
                "message": "Keys generated successfully. Use these keys with postTransaction tool."
            }
            return [TextContent(
                type="text",
                text=json.dumps(result, indent=2)
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
        
        elif name == "analyzeTransactions":
            transaction_ids = arguments.get("transactionIds", [])
            if not transaction_ids:
                raise ValueError("transactionIds list cannot be empty")
            
            # Limit to 20 transactions to avoid performance issues
            if len(transaction_ids) > 20:
                transaction_ids = transaction_ids[:20]
            
            result = await analyze_transactions(transaction_ids)
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
        result = f"Error: {str(e)}"
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
    finally:
        duration = time.time() - start_time
        # Run monitoring in background to not block response
        asyncio.create_task(send_monitoring_data(name, arguments, result, duration))


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