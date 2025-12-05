# Minimal MCP Server for ResilientDB Smart Contracts
# dependencies = ["mcp", "httpx", "gql[aiohttp]"]

import logging
import os
import subprocess
import json
import tempfile
from dataclasses import dataclass
from typing import Any

from mcp.server.fastmcp import FastMCP

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s - %(name)s - %(levelname)s - %(message)s"
)
logger = logging.getLogger("resdb-mcp-minimal")

@dataclass
class ResDBConfig:
    """Configuration for ResilientDB connection."""
    host: str
    rest_port: int = 18000
    graphql_port: int = 8000
    
    @classmethod
    def from_env(cls) -> "ResDBConfig":
        return cls(host="127.0.0.1") # Default to localhost for minimal version

class ResContractClient:
    """Client for interacting with ResContract CLI and contract_tools."""

    def __init__(self, config: ResDBConfig):
        self.config = config
        self.rescontract_cmd = "rescontract"
        
        # Determine repo root relative to this script file
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
        # Ensure contract_path ends with .json
        if not contract_path.endswith('.json'):
            contract_path += '.json'
            
        # Preprocess the contract JSON to match what contract_tools expects
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
            "contract_address": owner_address, 
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

# Initialize MCP Server
mcp = FastMCP("ResilientDB Smart Contracts")
config = ResDBConfig.from_env()
contract_client = ResContractClient(config)

@mcp.tool()
async def create_account(config_path: str = "service/tools/config/interface/service.config") -> str:
    """Create a new ResilientDB account."""
    return contract_client.create_account(config_path)

@mcp.tool()
async def compile_contract(sol_path: str, output_name: str) -> str:
    """Compile a Solidity contract."""
    return contract_client.compile_solidity(sol_path, output_name)

@mcp.tool()
async def deploy_contract(
    name: str, 
    arguments: str, 
    owner_address: str,
    config_path: str = "service/tools/config/interface/service.config",
    contract_path: str = None
) -> str:
    """Deploy a smart contract."""
    if contract_path is None:
        contract_path = name
    return contract_client.deploy_contract(config_path, contract_path, name, arguments, owner_address)

@mcp.tool()
async def execute_contract(
    contract_address: str,
    function_name: str,
    arguments: str,
    sender_address: str,
    config_path: str = "service/tools/config/interface/service.config"
) -> str:
    """Execute a function on a deployed smart contract."""
    return contract_client.execute_contract(config_path, sender_address, contract_address, function_name, arguments)

if __name__ == "__main__":
    mcp.run()
