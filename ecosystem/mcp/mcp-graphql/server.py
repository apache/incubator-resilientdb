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
from rescontract_client import ResContractClient

# Initialize clients
graphql_client = GraphQLClient()
rescontract_client = ResContractClient()


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
        ),
        # Smart Contract Tools
        Tool(
            name="introspectGraphQL",
            description="Introspect the ResilientDB GraphQL schema to see available types and operations.",
            inputSchema={
                "type": "object",
                "properties": {},
                "required": []
            }
        ),
        Tool(
            name="compileContract",
            description="Compile a Solidity smart contract to JSON format.",
            inputSchema={
                "type": "object",
                "properties": {
                    "sol_path": {
                        "type": "string",
                        "description": "Path to the .sol file."
                    },
                    "output_name": {
                        "type": "string",
                        "description": "Name of the output .json file."
                    }
                },
                "required": ["sol_path", "output_name"]
            }
        ),
        Tool(
            name="deployContract",
            description="Deploy a smart contract to ResilientDB.",
            inputSchema={
                "type": "object",
                "properties": {
                    "config_path": {
                        "type": "string",
                        "description": "Path to the client configuration file."
                    },
                    "contract_path": {
                        "type": "string",
                        "description": "Path to the compiled contract JSON file."
                    },
                    "name": {
                        "type": "string",
                        "description": "Name of the contract."
                    },
                    "arguments": {
                        "type": "string",
                        "description": "Constructor parameters (comma-separated)."
                    },
                    "owner_address": {
                        "type": "string",
                        "description": "The address of the contract owner."
                    }
                },
                "required": ["config_path", "contract_path", "name", "arguments", "owner_address"]
            }
        ),
        Tool(
            name="executeContract",
            description="Execute a function on a deployed smart contract.",
            inputSchema={
                "type": "object",
                "properties": {
                    "config_path": {
                        "type": "string",
                        "description": "Path to the client configuration file."
                    },
                    "sender_address": {
                        "type": "string",
                        "description": "The address of the sender executing the function."
                    },
                    "contract_address": {
                        "type": "string",
                        "description": "The address of the deployed contract."
                    },
                    "function_name": {
                        "type": "string",
                        "description": "Name of the function to execute (including parameter types, e.g., 'transfer(address,uint256)')."
                    },
                    "arguments": {
                        "type": "string",
                        "description": "Arguments to pass to the function (comma-separated)."
                    }
                },
                "required": ["config_path", "sender_address", "contract_address", "function_name", "arguments"]
            }
        ),
        Tool(
            name="createAccount",
            description="Create a new ResilientDB account for smart contract operations.",
            inputSchema={
                "type": "object",
                "properties": {
                    "config_path": {
                        "type": "string",
                        "description": "Path to the client configuration file."
                    }
                },
                "required": ["config_path"]
            }
        ),
        Tool(
            name="checkReplicasStatus",
            description="Check the status of ResilientDB contract service replicas. Returns information about how many of the 5 required replicas are currently running.",
            inputSchema={
                "type": "object",
                "properties": {},
                "required": []
            }
        ),
        Tool(
            name="startReplicas",
            description="Start or restart the ResilientDB contract service replica cluster. WARNING: This will wipe the existing blockchain state.",
            inputSchema={
                "type": "object",
                "properties": {},
                "required": []
            }
        ),
        Tool(
            name="getServerLogs",
            description="Get recent log entries from a specific replica server.",
            inputSchema={
                "type": "object",
                "properties": {
                    "server_id": {
                        "type": "integer",
                        "description": "The server ID (0-3 for server0.log through server3.log). Default is 0."
                    },
                    "lines": {
                        "type": "integer",
                        "description": "Number of recent log lines to retrieve. Default is 50."
                    }
                },
                "required": []
            }
        ),
        Tool(
            name="getClientLogs",
            description="Get recent log entries from the client proxy.",
            inputSchema={
                "type": "object",
                "properties": {
                    "lines": {
                        "type": "integer",
                        "description": "Number of recent log lines to retrieve. Default is 50."
                    }
                },
                "required": []
            }
        ),
        Tool(
            name="validateConfig",
            description="Validate a ResilientDB configuration file. Checks for file existence, correct format, valid addresses, valid ports, and other configuration errors.",
            inputSchema={
                "type": "object",
                "properties": {
                    "config_path": {
                        "type": "string",
                        "description": "Absolute path to the configuration file to validate."
                    }
                },
                "required": ["config_path"]
            }
        ),
        Tool(
            name="healthCheck",
            description="Perform a comprehensive health check of all ResilientDB system components. Checks replicas, REST API, GraphQL API, and network latency.",
            inputSchema={
                "type": "object",
                "properties": {},
                "required": []
            }
        ),
        Tool(
            name="listAllAccounts",
            description="List all accounts found on the ResilientDB blockchain. Parses server logs to find all created accounts and their activity levels.",
            inputSchema={
                "type": "object",
                "properties": {},
                "required": []
            }
        ),
        Tool(
            name="getTransactionHistory",
            description="Query transaction history from the ResilientDB blockchain. Parses server logs to extract DEPLOY and EXECUTE transactions with filtering options.",
            inputSchema={
                "type": "object",
                "properties": {
                    "limit": {
                        "type": "integer",
                        "description": "Maximum number of transactions to return. Default is 50."
                    },
                    "tx_type": {
                        "type": "string",
                        "description": "Filter by transaction type: 'DEPLOY' or 'EXECUTE'. Optional."
                    },
                    "address": {
                        "type": "string",
                        "description": "Filter by account address (shows transactions involving this address). Optional."
                    }
                },
                "required": []
            }
        ),
        Tool(
            name="searchLogs",
            description="Search for a text pattern in the server logs.",
            inputSchema={
                "type": "object",
                "properties": {
                    "query": {
                        "type": "string",
                        "description": "The text string to search for (e.g., 'Error', 'TransactionID')."
                    },
                    "server_id": {
                        "type": "integer",
                        "description": "Optional server ID (0-3) to search only one log. If omitted, searches all logs."
                    },
                    "lines": {
                        "type": "integer",
                        "description": "Maximum number of matching lines to return. Default is 100."
                    }
                },
                "required": ["query"]
            }
        ),
        Tool(
            name="getConsensusMetrics",
            description="Get internal consensus metrics from the system logs. Extracts the current View Number, Sequence Number, and Primary Replica ID.",
            inputSchema={
                "type": "object",
                "properties": {},
                "required": []
            }
        ),
        Tool(
            name="archiveLogs",
            description="Archive all current log files to a ZIP file. Creates a timestamped ZIP file containing server0-3.log, client.log, and configuration files.",
            inputSchema={
                "type": "object",
                "properties": {},
                "required": []
            }
        ),
        Tool(
            name="benchmarkThroughput",
            description="Benchmark system throughput by sending a batch of transactions via HTTP REST API (key-value operations). Returns metrics including TPS (transactions per second), latency statistics (min/avg/max), success rate, and duration. Useful for performance testing and capacity planning.",
            inputSchema={
                "type": "object",
                "properties": {
                    "num_tx": {
                        "type": "integer",
                        "description": "Number of transactions to send for benchmarking. Default is 100.",
                        "default": 100
                    }
                },
                "required": []
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
            
            # Process asset - ensure it has 'data' field
            asset = arguments["asset"]
            if isinstance(asset, str):
                try:
                    asset = json.loads(asset)
                except json.JSONDecodeError:
                    pass  # Keep as string if not valid JSON
            
            # If asset is a dict but doesn't have 'data' field, wrap it
            if isinstance(asset, dict) and "data" not in asset:
                asset = {"data": asset}
            elif not isinstance(asset, dict):
                # If it's still a string or other type, wrap it in data
                asset = {"data": asset}
            
            # Build PrepareAsset from individual arguments
            data = {
                "operation": arguments["operation"],
                "amount": arguments["amount"],
                "signerPublicKey": arguments["signerPublicKey"],
                "signerPrivateKey": arguments["signerPrivateKey"],
                "recipientPublicKey": arguments["recipientPublicKey"],
                "asset": asset
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
        
        # Smart Contract Tools
        elif name == "introspectGraphQL":
            query = "{ __schema { types { name } } }"
            result = await graphql_client.execute_query(query)
            return [TextContent(
                type="text",
                text=json.dumps(result, indent=2)
            )]
        
        elif name == "compileContract":
            sol_path = arguments["sol_path"]
            output_name = arguments["output_name"]
            result = rescontract_client.compile_solidity(sol_path, output_name)
            return [TextContent(
                type="text",
                text=json.dumps({"status": "success", "output": result}, indent=2)
            )]
        
        elif name == "deployContract":
            config_path = arguments["config_path"]
            contract_path = arguments["contract_path"]
            name = arguments["name"]
            arguments_str = arguments.get("arguments", "")
            owner_address = arguments["owner_address"]
            result = rescontract_client.deploy_contract(config_path, contract_path, name, arguments_str, owner_address)
            return [TextContent(
                type="text",
                text=json.dumps({"status": "success", "output": result}, indent=2)
            )]
        
        elif name == "executeContract":
            config_path = arguments["config_path"]
            sender_address = arguments["sender_address"]
            contract_address = arguments["contract_address"]
            function_name = arguments["function_name"]
            arguments_str = arguments.get("arguments", "")
            result = rescontract_client.execute_contract(config_path, sender_address, contract_address, function_name, arguments_str)
            return [TextContent(
                type="text",
                text=json.dumps({"status": "success", "output": result}, indent=2)
            )]
        
        elif name == "createAccount":
            config_path = arguments["config_path"]
            result = rescontract_client.create_account(config_path)
            return [TextContent(
                type="text",
                text=json.dumps({"status": "success", "output": result}, indent=2)
            )]
        
        elif name == "checkReplicasStatus":
            status = rescontract_client.check_replica_status()
            response = f"{status['message']}\n\n"
            if status['count'] > 0:
                response += "Running processes:\n"
                for i, detail in enumerate(status['details'], 1):
                    detail_short = detail[:150] + "..." if len(detail) > 150 else detail
                    response += f"{i}. {detail_short}\n"
            if not status['running']:
                response += "\n⚠️ System is NOT ready for operations. Use startReplicas tool to start the cluster."
            else:
                response += "\n✅ System is ready for contract operations."
            return [TextContent(
                type="text",
                text=json.dumps({"status": status, "message": response}, indent=2)
            )]
        
        elif name == "startReplicas":
            result = rescontract_client.start_replica_cluster()
            return [TextContent(
                type="text",
                text=json.dumps({"status": "success", "output": result, "warning": "The blockchain state has been reset. You will need to create new accounts and redeploy contracts."}, indent=2)
            )]
        
        elif name == "getServerLogs":
            server_id = arguments.get("server_id", 0)
            lines = arguments.get("lines", 50)
            if server_id < 0 or server_id > 3:
                raise ValueError(f"server_id must be between 0 and 3. Got: {server_id}")
            log_file = f"server{server_id}.log"
            result = rescontract_client.get_logs(log_file, lines)
            return [TextContent(
                type="text",
                text=json.dumps({"log_file": log_file, "lines": lines, "content": result}, indent=2)
            )]
        
        elif name == "getClientLogs":
            lines = arguments.get("lines", 50)
            result = rescontract_client.get_logs("client.log", lines)
            return [TextContent(
                type="text",
                text=json.dumps({"log_file": "client.log", "lines": lines, "content": result}, indent=2)
            )]
        
        elif name == "validateConfig":
            config_path = arguments["config_path"]
            result = rescontract_client.validate_config(config_path)
            return [TextContent(
                type="text",
                text=json.dumps(result, indent=2)
            )]
        
        elif name == "healthCheck":
            health = rescontract_client.health_check()
            status_emoji = {
                "healthy": "✅",
                "degraded": "⚠️",
                "down": "❌"
            }
            overall_emoji = status_emoji.get(health["overall_status"], "❓")
            report = f"🏥 ResilientDB Health Check Report\n\n"
            report += f"Overall Status: {overall_emoji} {health['overall_status'].upper()}\n\n"
            report += "📊 Components:\n"
            rep = health["replicas"]
            rep_emoji = status_emoji.get(rep["status"], "❓")
            report += f"  {rep_emoji} Replicas: {rep['message']}\n"
            rest = health["rest_api"]
            rest_emoji = status_emoji.get(rest["status"], "❓")
            if rest["status"] == "healthy":
                report += f"  {rest_emoji} REST API: Responding ({rest['url']}) - {rest['latency_ms']}ms\n"
            else:
                report += f"  {rest_emoji} REST API: Down ({rest['url']}) - {rest.get('error', 'Unknown error')}\n"
            gql = health["graphql_api"]
            gql_emoji = status_emoji.get(gql["status"], "❓")
            if gql["status"] == "healthy":
                report += f"  {gql_emoji} GraphQL API: Responding ({gql['url']}) - {gql['latency_ms']}ms\n"
            else:
                report += f"  {gql_emoji} GraphQL API: Down ({gql['url']}) - {gql.get('error', 'Unknown error')}\n"
            if health["overall_status"] != "healthy":
                report += "\n💡 Recommendations:\n"
                if health["replicas"]["status"] != "healthy":
                    report += "  • Start replicas using the startReplicas tool\n"
                if health["rest_api"]["status"] != "healthy":
                    report += "  • Check if ResilientDB REST service is running on port 18000\n"
                if health["graphql_api"]["status"] != "healthy":
                    report += "  • Check if ResilientDB GraphQL service is running on port 8000\n"
            return [TextContent(
                type="text",
                text=json.dumps({"health": health, "report": report}, indent=2)
            )]
        
        elif name == "listAllAccounts":
            accounts = rescontract_client.list_all_accounts()
            if not accounts:
                response = "No accounts found in the system logs.\n\nCreate an account using the createAccount tool."
            else:
                response = f"👥 ResilientDB Accounts ({len(accounts)} total)\n\n"
                for i, acc in enumerate(accounts, 1):
                    response += f"{i}. {acc['address']}\n"
                    response += f"   Created: {acc['created']}\n"
                    response += f"   Activity: {acc['activity_count']} log entries\n\n"
            return [TextContent(
                type="text",
                text=json.dumps({"accounts": accounts, "message": response}, indent=2)
            )]
        
        elif name == "getTransactionHistory":
            limit = arguments.get("limit", 50)
            tx_type = arguments.get("tx_type")
            address = arguments.get("address")
            transactions = rescontract_client.get_transaction_history(limit, tx_type, address)
            if not transactions:
                response = "📜 No transactions found matching the criteria.\n\nTransactions will appear here after deploying contracts or executing functions."
            else:
                response = f"📜 Transaction History ({len(transactions)} transactions"
                if tx_type:
                    response += f", type={tx_type}"
                if address:
                    response += f", address={address[:10]}..."
                response += ")\n\n"
                for i, tx in enumerate(transactions, 1):
                    if tx["type"] == "DEPLOY":
                        response += f"{i}. [DEPLOY] {tx['timestamp']}\n"
                        response += f"   Caller: {tx['caller']}\n"
                        response += f"   Contract: {tx['contract_name']}\n\n"
                    elif tx["type"] == "EXECUTE":
                        response += f"{i}. [EXECUTE] {tx['timestamp']}\n"
                        response += f"   Caller: {tx['caller']}\n"
                        response += f"   Contract: {tx['contract_address']}\n"
                        response += f"   Function: {tx['function']}\n\n"
            return [TextContent(
                type="text",
                text=json.dumps({"transactions": transactions, "message": response}, indent=2)
            )]
        
        elif name == "searchLogs":
            query = arguments["query"]
            server_id = arguments.get("server_id")
            lines = arguments.get("lines", 100)
            result = rescontract_client.search_logs(query, server_id, lines)
            return [TextContent(
                type="text",
                text=json.dumps({"query": query, "results": result}, indent=2)
            )]
        
        elif name == "getConsensusMetrics":
            metrics = rescontract_client.get_consensus_metrics()
            report = f"📊 Consensus Metrics\n\n"
            report += f"👑 Primary Replica: {metrics['primary_id']}\n"
            report += f"👀 Current View: {metrics['view']}\n"
            report += f"🔢 Sequence Number: {metrics['sequence']}\n"
            report += f"🟢 Active Replicas: {metrics['active_replicas']}/5\n"
            return [TextContent(
                type="text",
                text=json.dumps({"metrics": metrics, "report": report}, indent=2)
            )]
        
        elif name == "archiveLogs":
            archive_path = rescontract_client.archive_logs()
            return [TextContent(
                type="text",
                text=json.dumps({"status": "success", "archive_path": archive_path, "message": f"📦 Logs archived successfully!\n\nLocation: {archive_path}"}, indent=2)
            )]
        
        elif name == "benchmarkThroughput":
            num_tx = arguments.get("num_tx", 100)
            if num_tx < 1 or num_tx > 10000:
                raise ValueError("num_tx must be between 1 and 10000")
            result = await rescontract_client.benchmark_throughput(num_tx)
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