# ResilientDB MCP Server

This directory contains the **Model Context Protocol (MCP)** server for ResilientDB, allowing you to interact with the blockchain directly from Claude Desktop.

## 🚀 Features

### Smart Contract Operations
*   **`create_account`**: Generate new ResilientDB accounts.
*   **`compile_contract`**: Compile Solidity (`.sol`) files to JSON artifacts.
*   **`deploy_contract`**: Deploy compiled contracts to the blockchain.
*   **`execute_contract`**: Call functions on deployed contracts.

## 📦 Server Variants

We provide two MCP server implementations to suit different needs:

### 1. Minimal Server (`mcp.py`)
**Best for:** Smart contract developers who want a lightweight, focused experience.
**Features:**
*   `create_account`
*   `compile_contract`
*   `deploy_contract`
*   `execute_contract`

### 2. Full Server (`res-mcp.py`)
**Best for:** System administrators and advanced users who need full control and observability.
**Features:**
*   **All Smart Contract Operations** (same as above)
*   **System Management**: `health_check`, `start_replicas`, `validate_config`, `archive_logs`
*   **Observability**: `get_transaction_history`, `list_all_accounts`, `search_logs`, `get_consensus_metrics`
*   **Database**: `commit_transaction`, `get_transaction`, `introspect_graphql`

---

## 🛠️ Installation & Setup

### 1. Install ResilientDB
Before using the MCP server, you must have ResilientDB built and running.

1.  **Clone the Repository**:
    ```bash
    git clone https://github.com/resilientdb/resilientdb.git
    cd resilientdb
    ```

2.  **Install Dependencies**:
    Follow the official guide: [ResilientDB Installation Docs](https://beacon.resilientdb.com/docs/installation)
    *   Ensure you have Bazel installed.

3.  **Build the System**:
    ```bash
    ./INSTALL.sh
    ```

4.  **Start the Replicas**:
    ```bash
    ./service/tools/contract/service_tools/start_contract_service.sh
    ```

### 2. Set Up the MCP Server

1.  **Install Python Dependencies**:
    ```bash
    pip install -r MCP_mod/requirements.txt
    ```
    *(Requires Python 3.10+)*

2.  **Configure Claude Desktop**:
    Edit `~/Library/Application Support/Claude/claude_desktop_config.json`:

    ```json
    {
      "mcpServers": {
        "resilientdb": {
          "command": "/usr/bin/python3",
          "args": [
            "/ABSOLUTE/PATH/TO/resilientdb/MCP_mod/mcp.py"
          ]
        }
      }
    }
    ```
    *   Replace `/ABSOLUTE/PATH/TO` with the full path to your `resilientdb` directory.
    *   **Note**: Use `mcp.py` for the lightweight smart contract server. Use `res-mcp.py` if you need advanced system management tools.

3.  **Restart Claude Desktop**.

---

## 🧪 Testing Guide

Follow these steps to verify the smart contract features.

### Step 1: Create an Account
Ask Claude:
> **"Create a new ResilientDB account"**

*   **Expected Output**: A new address (e.g., `0x123...`).
*   *Copy this address for the next steps.*

### Step 2: Compile a Contract
Ensure you have a Solidity file (e.g., `Counter.sol`).
Ask Claude:
> **"Compile the contract at /path/to/Counter.sol"**

*   **Expected Output**: "Compilation successful."

### Step 3: Deploy the Contract
Ask Claude:
> **"Deploy the Counter contract using account 0x123..."**

*   **Expected Output**: "Deployment successful" and the new **Contract Address**.

### Step 4: Execute a Function
Ask Claude:
> **"Execute the increment function on the Counter contract at address 0xContract..."**

*   **Expected Output**: "Execution successful."

---

## 📂 File Structure
*   **`mcp.py`**: Minimal MCP server for smart contract operations.
*   **`res-mcp.py`**: Full-featured MCP server (includes system management & debugging).
*   **`requirements.txt`**: Python dependencies.
*   **`README.md`**: This guide.
