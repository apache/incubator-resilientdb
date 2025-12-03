# /// script
# requires-python = ">=3.10"
# dependencies = [
#     "mcp",
#     "httpx",
#     "gql[aiohttp]",
# ]
# ///

import asyncio
import logging
import os
import socket
import subprocess
import json
import tempfile
from dataclasses import dataclass
from typing import Any, Optional

import httpx
from gql import Client, gql
from gql.transport.aiohttp import AIOHTTPTransport
from mcp.server.fastmcp import FastMCP
import time
import functools


# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s - %(name)s - %(levelname)s - %(message)s"
)
logger = logging.getLogger("resdb-mcp")

@dataclass
class ResDBConfig:
    """Configuration for ResilientDB connection."""
    host: str
    rest_port: int = 18000
    graphql_port: int = 8000
    
    @property
    def rest_url(self) -> str:
        """Base URL for REST API."""
        return f"http://{self.host}:{self.rest_port}/v1"
    
    @property
    def graphql_url(self) -> str:
        """URL for GraphQL API."""
        return f"http://{self.host}:{self.graphql_port}/graphql"

    @classmethod
    def from_env(cls) -> "ResDBConfig":
        """Create configuration from environment variables or auto-detection."""
        host = cls._detect_host()
        return cls(host=host)

    @staticmethod
    def _detect_host() -> str:
        """Detect the ResDB host from environment or network availability."""
        # 1. Explicit override
        env_host = os.getenv("RESDB_HOST")
        if env_host:
            logger.info(f"Using RESDB_HOST from environment: {env_host}")
            return env_host

        # 2. Localhost check
        if ResDBConfig._can_connect("127.0.0.1", 18000):
            logger.info("Detected ResDB on 127.0.0.1")
            return "127.0.0.1"

        # 3. Docker host check
        if ResDBConfig._resolves("host.docker.internal") and \
           ResDBConfig._can_connect("host.docker.internal", 18000):
            logger.info("Detected ResDB on host.docker.internal")
            return "host.docker.internal"

        # 4. Default fallback
        logger.warning("Could not detect ResDB, defaulting to 127.0.0.1")
        return "127.0.0.1"

    @staticmethod
    def _can_connect(host: str, port: int) -> bool:
        try:
            with socket.create_connection((host, port), timeout=1):
                return True
        except OSError:
            return False

    @staticmethod
    def _resolves(host: str) -> bool:
        try:
            socket.gethostbyname(host)
            return True
        except OSError:
            return False

class ResDBClient:
    """Client for interacting with ResilientDB via REST and GraphQL."""
    
    def __init__(self, config: ResDBConfig, retries: int = 3):
        self.config = config
        self.retries = retries

    async def post_transaction(self, transaction_id: str, value: str) -> dict[str, Any]:
        """
        Post a transaction to the REST API.
        
        Args:
            transaction_id: Unique ID for the transaction
            value: Value to store
            
        Returns:
            The JSON response from the server
            
        Raises:
            httpx.HTTPError: If the request fails after retries
        """
        url = f"{self.config.rest_url}/transactions/commit"
        payload = {"id": transaction_id, "value": value}
        
        return await self._make_rest_request("POST", url, json=payload)

    async def get_transaction(self, transaction_id: str) -> Optional[dict[str, Any]]:
        """
        Fetch a transaction by ID.
        
        Args:
            transaction_id: The ID of the transaction to fetch
            
        Returns:
            The transaction data or None if not found/error
        """
        # Note: Adjust endpoint based on actual ResDB API if different
        url = f"{self.config.rest_url}/transactions/{transaction_id}"
        try:
            return await self._make_rest_request("GET", url)
        except httpx.HTTPError:
            return None

    async def _make_rest_request(self, method: str, url: str, **kwargs) -> dict[str, Any]:
        """Helper to make REST requests with retries."""
        async with httpx.AsyncClient() as client:
            for attempt in range(1, self.retries + 1):
                try:
                    logger.debug(f"REST Attempt {attempt}: {method} {url}")
                    response = await client.request(method, url, timeout=10.0, **kwargs)
                    response.raise_for_status()
                    try:
                        return response.json()
                    except ValueError:
                        # Fallback for non-JSON responses (e.g. plain text confirmation)
                        return {"text": response.text}
                except httpx.HTTPError as e:
                    logger.warning(f"REST Attempt {attempt} failed: {e}")
                    if attempt == self.retries:
                        raise
                    await asyncio.sleep(1)
                except Exception as e:
                    logger.exception(f"Unexpected error during REST request")
                    if attempt == self.retries:
                        raise
                    await asyncio.sleep(1)
        return {}

    async def execute_graphql(self, query: str) -> dict[str, Any]:
        """
        Execute a GraphQL query.
        
        Args:
            query: The GraphQL query string
            
        Returns:
            The query result
        """
        transport = AIOHTTPTransport(url=self.config.graphql_url)
        
        for attempt in range(1, self.retries + 1):
            try:
                logger.debug(f"GraphQL Attempt {attempt}: executing query")
                async with Client(transport=transport, fetch_schema_from_transport=True) as session:
                    return await session.execute(gql(query))
            except Exception as e:
                logger.warning(f"GraphQL Attempt {attempt} failed: {e}")
                if attempt == self.retries:
                    raise
                await asyncio.sleep(1)
        return {}

    async def benchmark_throughput(self, num_tx: int = 100) -> dict[str, Any]:
        """
        Benchmark system throughput by sending a batch of transactions.
        Returns metrics including TPS and latency.
        """
        import time
        import uuid
        
        start_time = time.time()
        successful = 0
        failed = 0
        
        # Create tasks for concurrent execution
        tasks = []
        for i in range(num_tx):
            tx_id = f"bench-{uuid.uuid4()}"
            value = f"bench-val-{i}"
            tasks.append(self.post_transaction(tx_id, value))
            
        # Execute in batches to avoid overwhelming the client/network
        batch_size = 50
        for i in range(0, len(tasks), batch_size):
            batch = tasks[i:i+batch_size]
            results = await asyncio.gather(*batch, return_exceptions=True)
            
            for res in results:
                if isinstance(res, Exception):
                    failed += 1
                else:
                    successful += 1
                    
        end_time = time.time()
        duration = end_time - start_time
        tps = successful / duration if duration > 0 else 0
        
        return {
            "total_transactions": num_tx,
            "successful": successful,
            "failed": failed,
            "duration_seconds": round(duration, 2),
            "tps": round(tps, 2),
            "latency_avg_ms": round((duration / num_tx) * 1000, 2) if num_tx > 0 else 0
        }

class ResContractClient:
    """Client for interacting with ResContract CLI and contract_tools."""

    def __init__(self, config: ResDBConfig):
        self.config = config
        # Assuming rescontract is in the path, or we can specify full path if needed
        self.rescontract_cmd = "rescontract"
        
        # Determine repo root relative to this script file
        # Script is in <repo>/MCP_mod/res-mcp2.py, so repo root is parent dir
        self.repo_root = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
        
        self.contract_tools_path = os.path.join(
            self.repo_root, "bazel-bin/service/tools/contract/api_tools/contract_tools"
        )

    def _run_command(self, cmd: list[str]) -> str:
        """Run a command and return its output (stdout + stderr)."""
        logger.info(f"Running command: {' '.join(cmd)}")
        env = os.environ.copy()
        env["ResDB_Home"] = self.repo_root
        
        try:
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                check=True,
                env=env,
                cwd=self.repo_root
            )
            # Return both stdout and stderr as contract_tools logs to stderr
            return f"{result.stdout}\n{result.stderr}"
        except subprocess.CalledProcessError as e:
            logger.error(f"Command failed: {e.stderr}\nOutput: {e.stdout}")
            return f"Command failed: {e.stderr}\nOutput: {e.stdout}"

    def compile_solidity(self, sol_path: str, output_name: str) -> str:
        """Compile a solidity file."""
        # We can still use rescontract for compile as it just wraps solc, 
        # or we could call solc directly if needed. 
        # For now, assuming rescontract compile works as it doesn't involve contract_tools.
        cmd = [self.rescontract_cmd, "compile", "--sol", sol_path, "--output", output_name]
        return self._run_command(cmd)

    def _run_contract_tools(self, config_path: str, config_data: dict[str, Any]) -> str:
        """Helper to run contract_tools with a temp config file."""
        with tempfile.NamedTemporaryFile(mode='w', suffix='.json', delete=False) as tmp_config:
            json.dump(config_data, tmp_config)
            tmp_config_path = tmp_config.name
            
        try:
            cmd = [
                self.contract_tools_path,
                "-c", config_path,
                "--config_file", tmp_config_path
            ]
            return self._run_command(cmd)
        finally:
            if os.path.exists(tmp_config_path):
                os.remove(tmp_config_path)

    def deploy_contract(self, config_path: str, contract_path: str, name: str, 
                       arguments: str, owner_address: str) -> str:
        """Deploy a smart contract using contract_tools directly."""
        # Ensure contract_path ends with .json
        if not contract_path.endswith('.json'):
            contract_path += '.json'
            
        # Preprocess the contract JSON to match what contract_tools expects
        # contract_tools expects contracts[name] but solc outputs contracts[path:name]
        real_contract_path = contract_path
        
        # Resolve full path to read the file
        full_contract_path = contract_path
        if not os.path.isabs(contract_path):
            full_contract_path = os.path.join(self.repo_root, contract_path)
            
        try:
            with open(full_contract_path, 'r') as f:
                contract_data = json.load(f)
                
            if "contracts" in contract_data:
                contracts = contract_data["contracts"]
                if name not in contracts:
                    # Look for key ending with :name
                    found_key = None
                    for key in contracts:
                        if key.endswith(f":{name}"):
                            found_key = key
                            break
                    
                    if found_key:
                        # Create a temp file with corrected structure
                        logger.info(f"Mapping contract key '{found_key}' to '{name}' for deployment")
                        contract_data["contracts"][name] = contracts[found_key]
                        
                        # Write to temp file
                        with tempfile.NamedTemporaryFile(mode='w', suffix='.json', delete=False) as tmp_contract:
                            json.dump(contract_data, tmp_contract)
                            real_contract_path = contract_path # Keep original for reference
                            contract_path = tmp_contract.name # Use temp file
                            
        except Exception as e:
            logger.warning(f"Failed to preprocess contract JSON: {e}. Proceeding with original file.")

        config_data = {
            "command": "deploy",
            "contract_path": contract_path,
            "contract_name": name,
            "contract_address": owner_address, # In contract_tools, this maps to caller_address for deploy
            "init_params": arguments
        }
        
        try:
            return self._run_contract_tools(config_path, config_data)
        finally:
            # Clean up temp contract file if we created one
            if contract_path != real_contract_path and os.path.exists(contract_path):
                os.remove(contract_path)

    def execute_contract(self, config_path: str, sender_address: str, 
                        contract_address: str, function_name: str, 
                        arguments: str) -> str:
        """Execute a contract function using contract_tools directly."""
        config_data = {
            "command": "execute",
            "caller_address": sender_address,
            "contract_address": contract_address,
            "func_name": function_name,
            "params": arguments
        }
        return self._run_contract_tools(config_path, config_data)

    def create_account(self, config_path: str) -> str:
        """Create a new account using contract_tools directly."""
        config_data = {
            "command": "create_account"
        }
        return self._run_contract_tools(config_path, config_data)

    def check_replica_status(self) -> dict[str, Any]:
        """Check the status of contract_service replicas."""
        try:
            result = subprocess.run(
                ["ps", "aux"],
                capture_output=True,
                text=True,
                check=True
            )
            # Filter for contract_service processes (excluding grep itself)
            lines = [line for line in result.stdout.split('\n') 
                    if 'contract_service' in line and 'grep' not in line]
            
            count = len(lines)
            return {
                "count": count,
                "running": count == 5,
                "details": lines,
                "message": f"{count}/5 replicas running. System is {'ready' if count == 5 else 'NOT ready'}."
            }
        except subprocess.CalledProcessError as e:
            logger.error(f"Failed to check replica status: {e}")
            return {
                "count": 0,
                "running": False,
                "details": [],
                "message": f"Error checking replicas: {e}"
            }

    def start_replica_cluster(self) -> str:
        """Start/restart the ResilientDB replica cluster."""
        script_path = os.path.join(
            self.repo_root, 
            "service/tools/contract/service_tools/start_contract_service.sh"
        )
        
        if not os.path.exists(script_path):
            return f"Error: Start script not found at {script_path}"
        
        try:
            logger.info(f"Starting replica cluster using {script_path}")
            result = subprocess.run(
                ["/bin/bash", script_path],  # Execute through bash
                capture_output=True,
                text=True,
                cwd=self.repo_root,
                timeout=30
            )
            
            # Check if replicas started successfully
            import time
            time.sleep(2)  # Give replicas time to start
            status = self.check_replica_status()
            
            return f"Replica cluster started.\n{status['message']}\n\nOutput:\n{result.stdout}\n{result.stderr}"
        except subprocess.TimeoutExpired:
            return "Replica start script timed out after 30 seconds"
        except Exception as e:
            logger.error(f"Failed to start replicas: {e}")
            return f"Error starting replicas: {str(e)}"

    def get_logs(self, log_file: str, lines: int = 50) -> str:
        """Get recent lines from a log file."""
        log_path = os.path.join(self.repo_root, log_file)
        
        if not os.path.exists(log_path):
            return f"Error: Log file not found at {log_path}"
        
        try:
            result = subprocess.run(
                ["tail", "-n", str(lines), log_path],
                capture_output=True,
                text=True,
                check=True
            )
            return result.stdout
        except subprocess.CalledProcessError as e:
            logger.error(f"Failed to read log file {log_file}: {e}")
            return f"Error reading log file: {e.stderr}"

    @staticmethod
    def validate_address(address: str) -> tuple[bool, Optional[str]]:
        """
        Validate Ethereum-style address format.
        Returns (is_valid, error_message)
        """
        if not isinstance(address, str):
            return False, "Address must be a string"
        
        if not address.startswith("0x"):
            return False, "Address must start with '0x'"
        
        if len(address) != 42:  # 0x + 40 hex chars
            return False, f"Address must be 42 characters (got {len(address)})"
        
        try:
            int(address[2:], 16)  # Check if valid hex
            return True, None
        except ValueError:
            return False, "Address contains invalid hexadecimal characters"

    def validate_config(self, config_path: str) -> dict[str, Any]:
        """
        Validate a ResilientDB configuration file.
        Returns dict with 'valid', 'errors', and 'warnings' keys.
        """
        errors = []
        warnings = []
        
        # Check file exists
        if not os.path.exists(config_path):
            return {
                "valid": False,
                "errors": [f"Config file not found: {config_path}"],
                "warnings": []
            }
        
        # Check file is readable
        try:
            with open(config_path, 'r') as f:
                content = f.read().strip()
        except Exception as e:
            return {
                "valid": False,
                "errors": [f"Cannot read config file: {e}"],
                "warnings": []
            }
        
        # Determine config file type and validate accordingly
        if config_path.endswith('.json'):
            # JSON config file (e.g., contract deployment config)
            try:
                config_data = json.loads(content)
                
                # Validate common fields if present
                if "contract_address" in config_data:
                    is_valid, error = self.validate_address(config_data["contract_address"])
                    if not is_valid:
                        errors.append(f"Invalid contract_address: {error}")
                
                if "caller_address" in config_data:
                    is_valid, error = self.validate_address(config_data["caller_address"])
                    if not is_valid:
                        errors.append(f"Invalid caller_address: {error}")
                
                if "contract_path" in config_data:
                    if not os.path.exists(config_data["contract_path"]):
                        warnings.append(f"Contract file not found: {config_data['contract_path']}")
                        
            except json.JSONDecodeError as e:
                errors.append(f"Invalid JSON format: {e}")
        else:
            # Text-based config file (e.g., service.config)
            # Format: "num_replicas host port"
            lines = content.split('\n')
            for i, line in enumerate(lines, 1):
                if not line or line.startswith('#'):
                    continue
                    
                parts = line.split()
                if len(parts) < 3:
                    errors.append(f"Line {i}: Expected format 'num_replicas host port', got: {line}")
                    continue
                
                try:
                    num_replicas = int(parts[0])
                    if num_replicas < 1 or num_replicas > 100:
                        warnings.append(f"Line {i}: Unusual number of replicas: {num_replicas}")
                except ValueError:
                    errors.append(f"Line {i}: Invalid replica count '{parts[0]}' (must be integer)")
                
                host = parts[1]
                if not (host == "localhost" or host == "127.0.0.1" or "." in host):
                    warnings.append(f"Line {i}: Unusual host format: {host}")
                
                try:
                    port = int(parts[2])
                    if port < 1 or port > 65535:
                        errors.append(f"Line {i}: Invalid port {port} (must be 1-65535)")
                except ValueError:
                    errors.append(f"Line {i}: Invalid port '{parts[2]}' (must be integer)")
        
        return {
            "valid": len(errors) == 0,
            "errors": errors,
            "warnings": warnings
        }

    def health_check(self) -> dict[str, Any]:
        """Comprehensive system health check."""
        import socket
        health = {
            "replicas": self._health_check_replicas(),
            "rest_api": self._health_check_rest(),
            "graphql_api": self._health_check_graphql(),
            "overall_status": "healthy"
        }
        
        # Determine overall status
        if health["replicas"]["status"] != "healthy" or health["rest_api"]["status"] != "healthy":
            health["overall_status"] = "down"
        elif health["graphql_api"]["status"] != "healthy":
            health["overall_status"] = "degraded"
            
        return health
    
    def _health_check_replicas(self) -> dict[str, Any]:
        """Check replica health."""
        status = self.check_replica_status()
        return {
            "status": "healthy" if status["running"] else "down",
            "count": status["count"],
            "message": status["message"]
        }
    
    def _health_check_rest(self) -> dict[str, Any]:
        """Check REST API health."""
        import time
        import socket
        try:
            start = time.time()
            sock = socket.create_connection(("127.0.0.1", 18000), timeout=2)
            latency_ms = int((time.time() - start) * 1000)
            sock.close()
            return {
                "status": "healthy",
                "latency_ms": latency_ms,
                "url": "http://127.0.0.1:18000"
            }
        except (socket.timeout, ConnectionRefusedError, OSError) as e:
            return {
                "status": "down",
                "error": str(e),
                "url": "http://127.0.0.1:18000"
            }
    
    def _health_check_graphql(self) -> dict[str, Any]:
        """Check GraphQL API health."""
        import time
        import socket
        try:
            start = time.time()
            sock = socket.create_connection(("127.0.0.1", 8000), timeout=2)
            latency_ms = int((time.time() - start) * 1000)
            sock.close()
            return {
                "status": "healthy",
                "latency_ms": latency_ms,
                "url": "http://127.0.0.1:8000"
            }
        except (socket.timeout, ConnectionRefusedError, OSError) as e:
            return {
                "status": "down",
                "error": str(e),
                "url": "http://127.0.0.1:8000"
            }

    def list_all_accounts(self) -> list[dict[str, Any]]:
        """List all accounts found in the system logs."""
        import re
        from datetime import datetime
        
        accounts = {}
        log_file = os.path.join(self.repo_root, "server0.log")
        
        if not os.path.exists(log_file):
            return []
        
        # Parse server logs for account creation
        try:
            with open(log_file, 'r') as f:
                for line in f:
                    if "create count:address" in line or 'create account:' in line:
                        # Extract address using regex
                        match = re.search(r'address: "([^"]+)"', line)
                        if match:
                            addr = match.group(1)
                            if addr not in accounts:
                                # Extract timestamp
                                timestamp_match = re.match(r'E(\d{8} \d{2}:\d{2}:\d{2})', line)
                                timestamp = timestamp_match.group(1) if timestamp_match else "Unknown"
                                
                                accounts[addr] = {
                                    "address": addr,
                                    "created": timestamp,
                                    "activity_count": 0
                                }
            
            # Count activity (mentions in logs) for each account
            with open(log_file, 'r') as f:
                for line in f:
                    for addr in accounts:
                        if addr in line:
                            accounts[addr]["activity_count"] += 1
                            
        except Exception as e:
            logger.error(f"Error parsing accounts from logs: {e}")
            return []
        
        return sorted(accounts.values(), key=lambda x: x["created"], reverse=True)

    def get_transaction_history(
        self, 
        limit: int = 50,
        tx_type: Optional[str] = None,
        address: Optional[str] = None
    ) -> list[dict[str, Any]]:
        """Get transaction history from server logs."""
        import re
        
        transactions = []
        log_file = os.path.join(self.repo_root, "server0.log")
        
        if not os.path.exists(log_file):
            return []
        
        try:
            with open(log_file, 'r') as f:
                for line in f:
                   # Parse DEPLOY transactions
                    if "cmd: DEPLOY" in line:
                        timestamp_match = re.match(r'E(\d{8} \d{2}:\d{2}:\d{2})', line)
                        timestamp = timestamp_match.group(1) if timestamp_match else "Unknown"
                        
                        caller_match = re.search(r'caller_address: "([^"]+)"', line)
                        caller = caller_match.group(1) if caller_match else "Unknown"
                        
                        contract_match = re.search(r'contract_name: "([^"]+)"', line)
                        contract_name = contract_match.group(1) if contract_match else "Unknown"
                        
                        tx = {
                            "type": "DEPLOY",
                            "timestamp": timestamp,
                            "caller": caller,
                            "contract_name": contract_name,
                            "details": ""
                        }
                        
                        if self._matches_filter(tx, tx_type, address):
                            transactions.append(tx)
                    
                    # Parse EXECUTE transactions
                    elif "cmd: EXECUTE" in line:
                        timestamp_match = re.match(r'E(\d{8} \d{2}:\d{2}:\d{2})', line)
                        timestamp = timestamp_match.group(1) if timestamp_match else "Unknown"
                        
                        caller_match = re.search(r'caller_address: "([^"]+)"', line)
                        caller = caller_match.group(1) if caller_match else "Unknown"
                        
                        contract_match = re.search(r'contract_address: "([^"]+)"', line)
                        contract_addr = contract_match.group(1) if contract_match else "Unknown"
                        
                        func_match = re.search(r'func_name: "([^"]+)"', line)
                        func_name = func_match.group(1) if func_match else "Unknown"
                        
                        tx = {
                            "type": "EXECUTE",
                            "timestamp": timestamp,
                            "caller": caller,
                            "contract_address": contract_addr,
                            "function": func_name
                        }
                        
                        if self._matches_filter(tx, tx_type, address):
                            transactions.append(tx)
                            
        except Exception as e:
            logger.error(f"Error parsing transaction history: {e}")
            return []
        
        # Return most recent transactions
        return transactions[-limit:] if transactions else []
    
    def _matches_filter(
        self, 
        tx: dict[str, Any], 
        tx_type: Optional[str], 
        address: Optional[str]
    ) -> bool:
        """Check if transaction matches filter criteria."""
        if tx_type and tx["type"] != tx_type.upper():
            return False
        if address and address not in str(tx):
            return False
        return True

    def search_logs(self, query: str, server_id: Optional[int] = None, lines: int = 100) -> str:
        """Search for a pattern in log files."""
        if server_id is not None:
            files = [f"server{server_id}.log"]
        else:
            files = ["server0.log", "server1.log", "server2.log", "server3.log", "client.log"]
            
        results = []
        for filename in files:
            log_path = os.path.join(self.repo_root, filename)
            if not os.path.exists(log_path):
                continue
                
            try:
                # Use grep for efficient searching
                cmd = ["grep", "-n", query, log_path]
                # If we want to limit output, we can pipe to tail, but subprocess makes piping hard.
                # Instead, we'll just run grep and take the last N lines in python if needed, 
                # or use tail first? Grep is better.
                
                result = subprocess.run(
                    cmd,
                    capture_output=True,
                    text=True
                )
                
                if result.stdout:
                    matches = result.stdout.strip().split('\n')
                    # Limit to last N matches if too many
                    if len(matches) > lines:
                        matches = matches[-lines:]
                        
                    results.append(f"--- {filename} ---")
                    results.extend(matches)
                    results.append("")
                    
            except Exception as e:
                results.append(f"Error searching {filename}: {e}")
                
        return "\n".join(results) if results else "No matches found."

    def get_consensus_metrics(self) -> dict[str, Any]:
        """Extract consensus metrics from server logs."""
        import re
        # Parse server0.log (primary candidate) for view change and sequence info
        log_path = os.path.join(self.repo_root, "server0.log")
        metrics = {
            "view": "Unknown",
            "sequence": "Unknown",
            "primary_id": "Unknown",
            "active_replicas": 0
        }
        
        if not os.path.exists(log_path):
            return metrics
            
        try:
            # Get last few lines to find current state
            result = subprocess.run(
                ["tail", "-n", "200", log_path],
                capture_output=True,
                text=True
            )
            
            lines = result.stdout.split('\n')
            for line in reversed(lines):
                # Example log: "receive public size:5 primary:1 version:1 from region:1 sender:3"
                if "primary:" in line and "version:" in line:
                    import re
                    # Extract primary
                    p_match = re.search(r'primary:(\d+)', line)
                    if p_match and metrics["primary_id"] == "Unknown":
                        metrics["primary_id"] = int(p_match.group(1))
                        
                    # Extract view/version
                    v_match = re.search(r'version:(\d+)', line)
                    if v_match and metrics["view"] == "Unknown":
                        metrics["view"] = int(v_match.group(1))
                
                # Example log: "seq gap:1" or "execute done:105"
                if "execute done:" in line:
                    s_match = re.search(r'execute done:(\d+)', line)
                    if s_match and metrics["sequence"] == "Unknown":
                        metrics["sequence"] = int(s_match.group(1))
                        
                if metrics["view"] != "Unknown" and metrics["sequence"] != "Unknown":
                    break
                    
            # Check active replicas count from recent "receive public size" logs
            metrics["active_replicas"] = self.check_replica_status()["count"]
            
        except Exception as e:
            logger.error(f"Error parsing consensus metrics: {e}")
            
        return metrics

    def archive_logs(self) -> str:
        """Archive all log files to a zip file."""
        import zipfile
        from datetime import datetime
        
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        archive_name = f"resdb_logs_{timestamp}.zip"
        archive_path = os.path.join(self.repo_root, archive_name)
        
        files_to_archive = [
            "server0.log", "server1.log", "server2.log", "server3.log", 
            "client.log", "service/tools/config/interface/service.config"
        ]
        
        try:
            with zipfile.ZipFile(archive_path, 'w', zipfile.ZIP_DEFLATED) as zipf:
                for file in files_to_archive:
                    file_path = os.path.join(self.repo_root, file)
                    if os.path.exists(file_path):
                        zipf.write(file_path, arcname=file)
                    else:
                        logger.warning(f"File not found for archive: {file}")
                        
            return archive_path
        except Exception as e:
            raise Exception(f"Failed to create archive: {e}")


 
# Initialize
config = ResDBConfig.from_env()
client = ResDBClient(config)
contract_client = ResContractClient(config)
mcp = FastMCP("resdb")

def monitor_tool(func):
    """
    Decorator to monitor MCP tool execution and send data to ResLens middleware.
    """
    @functools.wraps(func)
    async def wrapper(*args, **kwargs):
        start_time = time.time()
        tool_name = func.__name__
        result = None
        error = None
        
        try:
            result = await func(*args, **kwargs)
            return result
        except Exception as e:
            error = str(e)
            raise e
        finally:
            duration = time.time() - start_time
            
            # Prepare log data
            log_data = {
                "tool": tool_name,
                "args": kwargs, # FastMCP passes args as kwargs usually
                "result": str(result) if result else str(error),
                "timestamp": datetime.now().isoformat(),
                "duration": duration
            }
            
            # Send to middleware asynchronously (fire and forget)
            try:
                # We use httpx to send the data. 
                # Since we are in an async function, we can await it, 
                # but to avoid slowing down the tool, we might want to just log errors if it fails.
                async with httpx.AsyncClient() as client:
                    await client.post(
                        "http://localhost:3000/api/v1/mcp/prompts", 
                        json=log_data,
                        timeout=0.5 # Short timeout to not block
                    )
            except Exception as e:
                # Just log locally if middleware is down, don't break the tool
                logger.warning(f"Failed to send monitoring data to ResLens: {e}")

    return wrapper

@mcp.tool()
@monitor_tool
async def commit_transaction(id: str, value: str) -> str:
    """
    Commit a transaction to ResilientDB.
    
    Args:
        id: The unique identifier for the transaction.
        value: The value to store.
    """
    try:
        result = await client.post_transaction(id, value)
        return f"Transaction committed successfully. Response: {result}"
    except Exception as e:
        return f"Failed to commit transaction: {str(e)}"

@mcp.tool()
@monitor_tool
async def get_transaction(id: str) -> str:
    """
    Retrieve a transaction from ResilientDB by its ID.
    
    Args:
        id: The unique identifier of the transaction to retrieve.
    """
    try:
        result = await client.get_transaction(id)
        if result:
            return f"Transaction found: {result}"
        return f"Transaction {id} not found."
    except Exception as e:
        return f"Error fetching transaction: {str(e)}"

@mcp.tool()
@monitor_tool
async def introspect_graphql() -> str:
    """
    Introspect the ResilientDB GraphQL schema to see available types.
    """
    query = "{ __schema { types { name } } }"
    try:
        result = await client.execute_graphql(query)
        return str(result)
    except Exception as e:
        return f"Failed to fetch schema: {str(e)}"

@mcp.tool()
@monitor_tool
async def compile_contract(sol_path: str, output_name: str) -> str:
    """
    Compile a Solidity smart contract.
    
    Args:
        sol_path: Path to the .sol file.
        output_name: Name of the output .json file.
    """
    try:
        result = contract_client.compile_solidity(sol_path, output_name)
        return f"Compilation successful.\n{result}"
    except Exception as e:
        return f"Compilation failed: {str(e)}"

@mcp.tool()
@monitor_tool
async def deploy_contract(config_path: str, contract_path: str, name: str, 
                         arguments: str, owner_address: str) -> str:
    """
    Deploy a smart contract to ResilientDB.
    
    Args:
        config_path: Path to the client configuration file.
        contract_path: Path to the compiled contract JSON file.
        name: Name of the contract.
        arguments: Constructor parameters (comma-separated).
        owner_address: The address of the contract owner.
    """
    try:
        result = contract_client.deploy_contract(config_path, contract_path, name, arguments, owner_address)
        return f"Deployment successful.\n{result}"
    except Exception as e:
        return f"Deployment failed: {str(e)}"

@mcp.tool()
@monitor_tool
async def execute_contract(config_path: str, sender_address: str, 
                          contract_address: str, function_name: str, 
                          arguments: str) -> str:
    """
    Execute a function on a deployed smart contract.
    
    Args:
        config_path: Path to the client configuration file.
        sender_address: The address of the sender executing the function.
        contract_address: The address of the deployed contract.
        function_name: Name of the function to execute (including parameter types, e.g., 'transfer(address,uint256)').
        arguments: Arguments to pass to the function (comma-separated).
    """
    try:
        result = contract_client.execute_contract(config_path, sender_address, contract_address, function_name, arguments)
        return f"Execution successful.\n{result}"
    except Exception as e:
        return f"Execution failed: {str(e)}"

@mcp.tool()
@monitor_tool
async def create_account(config_path: str) -> str:
    """
    Create a new ResilientDB account.
    
    Args:
        config_path: Path to the client configuration file.
    """
    try:
        result = contract_client.create_account(config_path)
        return f"Account creation successful.\n{result}"
    except Exception as e:
        return f"Account creation failed: {str(e)}"

@mcp.tool()
@monitor_tool
async def check_replicas_status() -> str:
    """
    Check the status of ResilientDB contract service replicas.
    
    Returns information about how many of the 5 required replicas are currently running.
    The system needs all 5 replicas (4 consensus nodes + 1 client proxy) to operate correctly.
    """
    try:
        status = contract_client.check_replica_status()
        response = f"{status['message']}\n\n"
        
        if status['count'] > 0:
            response += f"Running processes:\n"
            for i, detail in enumerate(status['details'], 1):
                # Truncate long process lines for readability
                detail_short = detail[:150] + "..." if len(detail) > 150 else detail
                response += f"{i}. {detail_short}\n"
        
        if not status['running']:
            response += "\n⚠️ System is NOT ready for operations. Use start_replicas tool to start the cluster."
        else:
            response += "\n✅ System is ready for contract operations."
        
        return response
    except Exception as e:
        return f"Error checking replica status: {str(e)}"

@mcp.tool()
@monitor_tool
async def start_replicas() -> str:
    """
    Start or restart the ResilientDB contract service replica cluster.
    
    This will start 5 processes: 4 consensus replicas and 1 client proxy.
    
    WARNING: Starting the replicas will wipe the existing blockchain state.
    All previously created accounts and deployed contracts will be lost and need to be recreated.
    """
    try:
        result = contract_client.start_replica_cluster()
        return f"Replica cluster start initiated.\n\n{result}\n\n⚠️ Note: The blockchain state has been reset. You will need to create new accounts and redeploy contracts."
    except Exception as e:
        return f"Error starting replicas: {str(e)}"

@mcp.tool()
@monitor_tool
async def get_server_logs(server_id: int = 0, lines: int = 50) -> str:
    """
    Get recent log entries from a specific replica server.
    
    Args:
        server_id: The server ID (0-3 for server0.log through server3.log). Default is 0.
        lines: Number of recent log lines to retrieve. Default is 50.
    
    Returns the most recent log entries from the specified server log file.
    """
    if server_id < 0 or server_id > 3:
        return f"Error: server_id must be between 0 and 3. Got: {server_id}"
    
    try:
        log_file = f"server{server_id}.log"
        result = contract_client.get_logs(log_file, lines)
        return f"Last {lines} lines from {log_file}:\n\n{result}"
    except Exception as e:
        return f"Error reading server logs: {str(e)}"

@mcp.tool()
@monitor_tool
async def get_client_logs(lines: int = 50) -> str:
    """
    Get recent log entries from the client proxy.
    
    Args:
        lines: Number of recent log lines to retrieve. Default is 50.
    
    Returns the most recent log entries from client.log.
    """
    try:
        result = contract_client.get_logs("client.log", lines)
        return f"Last {lines} lines from client.log:\n\n{result}"
    except Exception as e:
        return f"Error reading client logs: {str(e)}"

@mcp.tool()
@monitor_tool
async def validate_config(config_path: str) -> str:
    """
    Validate a ResilientDB configuration file.
    
    Checks for file existence, correct format, valid addresses, valid ports,
    and other configuration errors. Supports both JSON configs and service.config format.
    
    Args:
        config_path: Absolute path to the configuration file to validate.
    """
    try:
        result = contract_client.validate_config(config_path)
        
        if result["valid"]:
            response = f"✅ Configuration is valid: {config_path}"
            if result["warnings"]:
                response += "\n\n⚠️ Warnings:\n" + "\n".join([f"  • {w}" for w in result["warnings"]])
            return response
        else:
            response = f"❌ Configuration has errors: {config_path}\n\n"
            response += "Errors:\n" + "\n".join([f"  ❌ {e}" for e in result["errors"]])
            if result["warnings"]:
                response += "\n\nWarnings:\n" + "\n".join([f"  ⚠️ {w}" for w in result["warnings"]])
            return response
    except Exception as e:
        return f"Error validating config: {str(e)}"

@mcp.tool()
@monitor_tool
async def health_check() -> str:
    """
    Perform a comprehensive health check of all ResilientDB system components.
    
    Checks:
    - Replica processes (5 required for operation)
    - REST API (port 18000)
    - GraphQL API (port 8000)
    - Network latency
    
    Returns a detailed health report with component status and recommendations.
    """
    try:
        health = contract_client.health_check()
        
        # Build formatted report
        status_emoji = {
            "healthy": "✅",
            "degraded": "⚠️",
            "down": "❌"
        }
        
        overall_emoji = status_emoji.get(health["overall_status"], "❓")
        
        report = f"🏥 ResilientDB Health Check Report\n\n"
        report += f"Overall Status: {overall_emoji} {health['overall_status'].upper()}\n\n"
        report += "📊 Components:\n"
        
        # Replicas
        rep = health["replicas"]
        rep_emoji = status_emoji.get(rep["status"], "❓")
        report += f"  {rep_emoji} Replicas: {rep['message']}\n"
        
        # REST API
        rest = health["rest_api"]
        rest_emoji = status_emoji.get(rest["status"], "❓")
        if rest["status"] == "healthy":
            report += f"  {rest_emoji} REST API: Responding ({rest['url']}) - {rest['latency_ms']}ms\n"
        else:
            report += f"  {rest_emoji} REST API: Down ({rest['url']}) - {rest.get('error', 'Unknown error')}\n"
        
        # GraphQL API
        gql = health["graphql_api"]
        gql_emoji = status_emoji.get(gql["status"], "❓")
        if gql["status"] == "healthy":
            report += f"  {gql_emoji} GraphQL API: Responding ({gql['url']}) - {gql['latency_ms']}ms\n"
        else:
            report += f"  {gql_emoji} GraphQL API: Down ({gql['url']}) - {gql.get('error', 'Unknown error')}\n"
        
        # Recommendations
        if health["overall_status"] != "healthy":
            report += "\n💡 Recommendations:\n"
            if health["replicas"]["status"] != "healthy":
                report += "  • Start replicas using the start_replicas tool\n"
            if health["rest_api"]["status"] != "healthy":
                report += "  • Check if ResilientDB REST service is running on port 18000\n"
            if health["graphql_api"]["status"] != "healthy":
                report += "  • Check if ResilientDB GraphQL service is running on port 8000\n"
        
        return report
    except Exception as e:
        return f"Error performing health check: {str(e)}"

@mcp.tool()
@monitor_tool
async def list_all_accounts() -> str:
    """
    List all accounts found on the ResilientDB blockchain.
    
    Parses server logs to find all created accounts and their activity levels.
    Shows account addresses, creation timestamps, and transaction counts.
    """
    try:
        accounts = contract_client.list_all_accounts()
        
        if not accounts:
            return "No accounts found in the system logs.\n\nCreate an account using the create_account tool."
        
        response = f"👥 ResilientDB Accounts ({len(accounts)} total)\n\n"
        
        for i, acc in enumerate(accounts, 1):
            response += f"{i}. {acc['address']}\n"
            response += f"   Created: {acc['created']}\n"
            response += f"   Activity: {acc['activity_count']} log entries\n\n"
        
        return response
    except Exception as e:
        return f"Error listing accounts: {str(e)}"

@mcp.tool()
@monitor_tool
async def get_transaction_history(
    limit: int = 50,
    tx_type: str = None,
    address: str = None
) -> str:
    """
    Query transaction history from the ResilientDB blockchain.
    
    Parses server logs to extract DEPLOY and EXECUTE transactions with filtering options.
    
    Args:
        limit: Maximum number of transactions to return (default: 50)
        tx_type: Filter by transaction type: "DEPLOY" or "EXECUTE" (optional)
        address: Filter by account address (shows transactions involving this address) (optional)
    """
    try:
        transactions = contract_client.get_transaction_history(limit, tx_type, address)
        
        if not transactions:
            return "📜 No transactions found matching the criteria.\n\nTransactions will appear here after deploying contracts or executing functions."
        
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
        
        return response
    except Exception as e:
        return f"Error getting transaction history: {str(e)}"


@mcp.tool()
@monitor_tool
async def search_logs(query: str, server_id: int = None, lines: int = 100) -> str:
    """
    Search for a text pattern in the server logs.
    
    Args:
        query: The text string to search for (e.g., "Error", "TransactionID").
        server_id: Optional server ID (0-3) to search only one log. If omitted, searches all logs.
        lines: Maximum number of matching lines to return. Default is 100.
    """
    try:
        result = contract_client.search_logs(query, server_id, lines)
        return f"🔍 Search Results for '{query}':\n\n{result}"
    except Exception as e:
        return f"Error searching logs: {str(e)}"

@mcp.tool()
@monitor_tool
async def benchmark_throughput(num_tx: int = 100) -> str:
    """
    Benchmark system throughput by sending a batch of transactions.
    
    Sends a specified number of transactions to the system and calculates
    Transactions Per Second (TPS) and average latency.
    
    Args:
        num_tx: Number of transactions to send. Default is 100.
    """
    try:
        metrics = await client.benchmark_throughput(num_tx)
        
        report = f"🚀 Benchmark Results ({num_tx} transactions)\n\n"
        report += f"⏱️  Duration: {metrics['duration_seconds']}s\n"
        report += f"⚡ TPS: {metrics['tps']} transactions/sec\n"
        report += f"📶 Avg Latency: {metrics['latency_avg_ms']}ms\n"
        report += f"✅ Successful: {metrics['successful']}\n"
        report += f"❌ Failed: {metrics['failed']}\n"
        
        return report
    except Exception as e:
        return f"Error running benchmark: {str(e)}"

@mcp.tool()
@monitor_tool
async def get_consensus_metrics() -> str:
    """
    Get internal consensus metrics from the system logs.
    
    Extracts the current View Number, Sequence Number, and Primary Replica ID
    to give insights into the PBFT consensus state.
    """
    try:
        metrics = contract_client.get_consensus_metrics()
        
        report = f"📊 Consensus Metrics\n\n"
        report += f"👑 Primary Replica: {metrics['primary_id']}\n"
        report += f"👀 Current View: {metrics['view']}\n"
        report += f"🔢 Sequence Number: {metrics['sequence']}\n"
        report += f"🟢 Active Replicas: {metrics['active_replicas']}/5\n"
        
        return report
    except Exception as e:
        return f"Error getting consensus metrics: {str(e)}"

@mcp.tool()
@monitor_tool
async def archive_logs() -> str:
    """
    Archive all current log files to a ZIP file.
    
    Creates a timestamped ZIP file containing server0-3.log, client.log,
    and configuration files. Useful for saving state before a restart.
    """
    try:
        archive_path = contract_client.archive_logs()
        return f"📦 Logs archived successfully!\n\nLocation: {archive_path}"
    except Exception as e:
        return f"Error archiving logs: {str(e)}"


if __name__ == "__main__":
    mcp.run(transport="stdio")

