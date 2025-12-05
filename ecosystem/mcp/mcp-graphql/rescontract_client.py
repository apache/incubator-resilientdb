"""ResContract client for smart contract operations."""
import os
import subprocess
import json
import tempfile
import logging
import asyncio
from typing import Dict, Any, Optional

logger = logging.getLogger(__name__)


class ResContractClient:
    """Client for interacting with ResContract CLI and contract_tools."""

    def __init__(self, repo_root: Optional[str] = None):
        """
        Initialize ResContract client.
        
        Args:
            repo_root: Root directory of ResilientDB repository. If None, auto-detects.
        """
        if repo_root is None:
            # Auto-detect: go up from mcp-graphql to incubator-resilientdb root
            script_dir = os.path.dirname(os.path.abspath(__file__))
            # From ecosystem/mcp/mcp-graphql/ to root
            repo_root = os.path.abspath(os.path.join(script_dir, '../../..'))
        
        self.repo_root = repo_root
        self.rescontract_cmd = "rescontract"
        
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
            return f"{result.stdout}\n{result.stderr}"
        except subprocess.CalledProcessError as e:
            logger.error(f"Command failed: {e.stderr}\nOutput: {e.stdout}")
            return f"Command failed: {e.stderr}\nOutput: {e.stdout}"

    def compile_solidity(self, sol_path: str, output_name: str) -> str:
        """Compile a solidity file."""
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
        if not contract_path.endswith('.json'):
            contract_path += '.json'
            
        real_contract_path = contract_path
        full_contract_path = contract_path
        if not os.path.isabs(contract_path):
            full_contract_path = os.path.join(self.repo_root, contract_path)
            
        try:
            with open(full_contract_path, 'r') as f:
                contract_data = json.load(f)
                
            if "contracts" in contract_data:
                contracts = contract_data["contracts"]
                if name not in contracts:
                    found_key = None
                    for key in contracts:
                        if key.endswith(f":{name}"):
                            found_key = key
                            break
                    
                    if found_key:
                        logger.info(f"Mapping contract key '{found_key}' to '{name}' for deployment")
                        contract_data["contracts"][name] = contracts[found_key]
                        
                        with tempfile.NamedTemporaryFile(mode='w', suffix='.json', delete=False) as tmp_contract:
                            json.dump(contract_data, tmp_contract)
                            real_contract_path = contract_path
                            contract_path = tmp_contract.name
                            
        except Exception as e:
            logger.warning(f"Failed to preprocess contract JSON: {e}. Proceeding with original file.")

        config_data = {
            "command": "deploy",
            "contract_path": contract_path,
            "contract_name": name,
            "contract_address": owner_address,
            "init_params": arguments
        }
        
        try:
            return self._run_contract_tools(config_path, config_data)
        finally:
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

    def check_replica_status(self) -> Dict[str, Any]:
        """Check the status of contract_service replicas."""
        try:
            result = subprocess.run(
                ["ps", "aux"],
                capture_output=True,
                text=True,
                check=True
            )
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
                ["/bin/bash", script_path],
                capture_output=True,
                text=True,
                cwd=self.repo_root,
                timeout=30
            )
            
            import time
            time.sleep(2)
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
        """Validate Ethereum-style address format."""
        if not isinstance(address, str):
            return False, "Address must be a string"
        
        if not address.startswith("0x"):
            return False, "Address must start with '0x'"
        
        if len(address) != 42:
            return False, f"Address must be 42 characters (got {len(address)})"
        
        try:
            int(address[2:], 16)
            return True, None
        except ValueError:
            return False, "Address contains invalid hexadecimal characters"

    def validate_config(self, config_path: str) -> Dict[str, Any]:
        """Validate a ResilientDB configuration file."""
        errors = []
        warnings = []
        
        if not os.path.exists(config_path):
            return {
                "valid": False,
                "errors": [f"Config file not found: {config_path}"],
                "warnings": []
            }
        
        try:
            with open(config_path, 'r') as f:
                content = f.read().strip()
        except Exception as e:
            return {
                "valid": False,
                "errors": [f"Cannot read config file: {e}"],
                "warnings": []
            }
        
        if config_path.endswith('.json'):
            try:
                config_data = json.loads(content)
                
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

    def health_check(self) -> Dict[str, Any]:
        """Comprehensive system health check."""
        import socket
        health = {
            "replicas": self._health_check_replicas(),
            "rest_api": self._health_check_rest(),
            "graphql_api": self._health_check_graphql(),
            "overall_status": "healthy"
        }
        
        if health["replicas"]["status"] != "healthy" or health["rest_api"]["status"] != "healthy":
            health["overall_status"] = "down"
        elif health["graphql_api"]["status"] != "healthy":
            health["overall_status"] = "degraded"
            
        return health
    
    def _health_check_replicas(self) -> Dict[str, Any]:
        """Check replica health."""
        status = self.check_replica_status()
        return {
            "status": "healthy" if status["running"] else "down",
            "count": status["count"],
            "message": status["message"]
        }
    
    def _health_check_rest(self) -> Dict[str, Any]:
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
    
    def _health_check_graphql(self) -> Dict[str, Any]:
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

    def list_all_accounts(self) -> list[Dict[str, Any]]:
        """List all accounts found in the system logs."""
        import re
        
        accounts = {}
        log_file = os.path.join(self.repo_root, "server0.log")
        
        if not os.path.exists(log_file):
            return []
        
        try:
            with open(log_file, 'r') as f:
                for line in f:
                    if "create count:address" in line or 'create account:' in line:
                        match = re.search(r'address: "([^"]+)"', line)
                        if match:
                            addr = match.group(1)
                            if addr not in accounts:
                                timestamp_match = re.match(r'E(\d{8} \d{2}:\d{2}:\d{2})', line)
                                timestamp = timestamp_match.group(1) if timestamp_match else "Unknown"
                                
                                accounts[addr] = {
                                    "address": addr,
                                    "created": timestamp,
                                    "activity_count": 0
                                }
            
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
    ) -> list[Dict[str, Any]]:
        """Get transaction history from server logs."""
        import re
        
        transactions = []
        log_file = os.path.join(self.repo_root, "server0.log")
        
        if not os.path.exists(log_file):
            return []
        
        try:
            with open(log_file, 'r') as f:
                for line in f:
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
        
        return transactions[-limit:] if transactions else []
    
    def _matches_filter(
        self, 
        tx: Dict[str, Any], 
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
                cmd = ["grep", "-n", query, log_path]
                result = subprocess.run(
                    cmd,
                    capture_output=True,
                    text=True
                )
                
                if result.stdout:
                    matches = result.stdout.strip().split('\n')
                    if len(matches) > lines:
                        matches = matches[-lines:]
                        
                    results.append(f"--- {filename} ---")
                    results.extend(matches)
                    results.append("")
                    
            except Exception as e:
                results.append(f"Error searching {filename}: {e}")
                
        return "\n".join(results) if results else "No matches found."

    def get_consensus_metrics(self) -> Dict[str, Any]:
        """Extract consensus metrics from server logs."""
        import re
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
            result = subprocess.run(
                ["tail", "-n", "200", log_path],
                capture_output=True,
                text=True
            )
            
            lines = result.stdout.split('\n')
            for line in reversed(lines):
                if "primary:" in line and "version:" in line:
                    p_match = re.search(r'primary:(\d+)', line)
                    if p_match and metrics["primary_id"] == "Unknown":
                        metrics["primary_id"] = int(p_match.group(1))
                        
                    v_match = re.search(r'version:(\d+)', line)
                    if v_match and metrics["view"] == "Unknown":
                        metrics["view"] = int(v_match.group(1))
                
                if "execute done:" in line:
                    s_match = re.search(r'execute done:(\d+)', line)
                    if s_match and metrics["sequence"] == "Unknown":
                        metrics["sequence"] = int(s_match.group(1))
                        
                if metrics["view"] != "Unknown" and metrics["sequence"] != "Unknown":
                    break
                    
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

    async def benchmark_throughput(self, num_tx: int = 100) -> Dict[str, Any]:
        """
        Benchmark system throughput by sending a batch of transactions via HTTP REST API.
        
        Returns metrics including TPS and latency.
        
        Args:
            num_tx: Number of transactions to send (default: 100)
            
        Returns:
            Dictionary with benchmark metrics
        """
        import httpx
        import uuid
        import time
        from config import Config
        
        http_url = Config.HTTP_URL
        start_time = time.time()
        successful = 0
        failed = 0
        latencies = []
        
        # Create tasks for concurrent execution
        async def send_transaction(tx_id: str, value: str):
            """Send a single transaction and return (success, latency)."""
            tx_start = time.time()
            try:
                async with httpx.AsyncClient(timeout=30.0) as client:
                    response = await client.post(
                        f"{http_url}/v1/transactions/commit",
                        json={"id": tx_id, "value": value},
                        headers={"Content-Type": "application/json"}
                    )
                    response.raise_for_status()
                    latency = (time.time() - tx_start) * 1000  # Convert to ms
                    return True, latency
            except Exception as e:
                logger.error(f"Transaction {tx_id} failed: {e}")
                latency = (time.time() - tx_start) * 1000
                return False, latency
        
        # Execute in batches to avoid overwhelming the client/network
        batch_size = 50
        tasks = []
        
        for i in range(num_tx):
            tx_id = f"bench-{uuid.uuid4()}"
            value = f"bench-val-{i}"
            tasks.append(send_transaction(tx_id, value))
        
        # Process in batches
        for i in range(0, len(tasks), batch_size):
            batch = tasks[i:i+batch_size]
            results = await asyncio.gather(*batch, return_exceptions=True)
            
            for res in results:
                if isinstance(res, Exception):
                    failed += 1
                else:
                    success, latency = res
                    if success:
                        successful += 1
                        latencies.append(latency)
                    else:
                        failed += 1
                    
        end_time = time.time()
        duration = end_time - start_time
        tps = successful / duration if duration > 0 else 0
        
        avg_latency = sum(latencies) / len(latencies) if latencies else 0
        min_latency = min(latencies) if latencies else 0
        max_latency = max(latencies) if latencies else 0
        
        return {
            "total_transactions": num_tx,
            "successful": successful,
            "failed": failed,
            "duration_seconds": round(duration, 2),
            "tps": round(tps, 2),
            "latency_avg_ms": round(avg_latency, 2),
            "latency_min_ms": round(min_latency, 2),
            "latency_max_ms": round(max_latency, 2),
            "success_rate": round((successful / num_tx) * 100, 2) if num_tx > 0 else 0
        }

