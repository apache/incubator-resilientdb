<!--
Licensed to the Apache Software Foundation (ASF) under one
or more contributor license agreements.  See the NOTICE file
distributed with this work for additional information
regarding copyright ownership.  The ASF licenses this file
to you under the Apache License, Version 2.0 (the
"License"); you may not use this file except in compliance
with the License.  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing,
software distributed under the License is distributed on an
"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
KIND, either express or implied.  See the License for the
specific language governing permissions and limitations
under the License.
-->

# 🎓 ResInsight: AI-Driven Developer Ecosystem

An **interactive, conversational educational assistant** built with the Model Context Protocol (MCP) that helps students learn ResilientDB distributed database systems through natural language interactions.

---

## 📋 Table of Contents
- [What This Is](#what-this-is)
- [Architecture](#architecture)
- [Quick Start](#quick-start)
- [How to Run](#how-to-run)
- [Authentication & Security](#authentication--security)
- [How to Use](#how-to-use)
- [Example Scenarios](#example-scenarios)
- [MCP Client Setup](#mcp-client-setup)
- [Educational Features](#educational-features)
- [Troubleshooting](#troubleshooting)

---

## What This Is

This MCP server transforms learning ResilientDB from reading static documentation into having **natural conversations** with an AI assistant that:

- ✅ **Understands your questions** in plain English
- ✅ **Provides educational, context-aware responses** 
- ✅ **Helps troubleshoot problems** with specific solutions
- ✅ **Guides you through concepts** from beginner to advanced
- ✅ **Offers practical examples** and hands-on learning

---

## Architecture

### Technology Stack

| Component | Purpose |
|-----------|---------|
| **FastMCP** | MCP server framework for tool integration |
| **FAISS** | Vector similarity search for semantic code understanding |
| **NetworkX** | Graph-based dependency analysis |
| **Sentence Transformers** | Semantic embedding generation |

### Data Flow

```
GitHub Repo
    ↓
[Ingestion] → Code & metadata via GitHub API
    ↓
[Processing] → Code chunked & embedded
    ↓
[Indexing] → FAISS embeddings + NetworkX graph
    ↓
[Query] → User queries through MCP tools
    ↓
[Retrieval] → Hybrid semantic + structural search
    ↓
[Response] → Context-aware answers via MCP
```

---

## 🚀 Quick Start

### Prerequisites

- **Python 3.9+** (3.10+ recommended; the provided `Dockerfile` uses 3.11)
- **Docker** (optional; use it to [run ResInsight in a container](#docker-optional))
- **MCP-compatible client** (Claude Desktop, Continue, Cursor, or any MCP client)
- **Tokens** — a **GitHub token** you create yourself, and an **`MCP_TOKEN`** for Bearer auth. **Locally**, you choose any strong secret (e.g. a random string) and use the same value in the server `.env` and your client. **To connect to the team’s cloud instance**, you must use the **`MCP_TOKEN` issued by the ResilientDB team** (see [Authentication & Security](#authentication--security))

### Installation Steps

1. **Clone the repository** (from the Apache ResilientDB monorepo, ResInsight lives under `ecosystem/ai-tools/mcp/ResInsight`)
   ```bash
   git clone https://github.com/apache/incubator-resilientdb.git
   cd incubator-resilientdb/ecosystem/ai-tools/mcp/ResInsight
   ```

2. **Create virtual environment**
   ```bash
   python -m venv .venv
   ```

3. **Activate environment**
   ```bash
   # Windows
   .venv\Scripts\activate

   # Linux/Mac
   source .venv/bin/activate
   ```

4. **Install dependencies**
   ```bash
   pip install -r requirements.txt
   ```

5. **Configure environment variables**
   
   Create a `.env` file in the ResInsight directory (same names the server reads in `server.py`):

   - **`MCP_TOKEN`** — Any strong secret **you define** when running the server yourself (must match what clients send as `Authorization: Bearer …`). If you use **ResilientDB’s hosted (cloud) endpoint**, use the token **we give you**—do not invent one for that URL.
   - **`GITHUB_TOKEN`** — **Strongly recommended.** A [GitHub Personal Access Token](https://github.com/settings/tokens) so the server can call the GitHub API without harsh rate limits.

   ```env
   GITHUB_TOKEN=ghp_your_github_token_here
   MCP_TOKEN=your_chosen_secret_or_team_issued_token
   ```

   Optional: if you use **GitHub Enterprise**, you can set `GITHUB_ENTERPRISE_TOKEN` instead of `GITHUB_TOKEN` (the server prefers the enterprise token when set).

---

## How to Run

Configure **`.env`** before starting the server: set **`GITHUB_TOKEN`** for GitHub API access, and **`MCP_TOKEN`** to a secret you pick when self-hosting (or the value issued by the team if you only connect to cloud)—see [Authentication & Security](#authentication--security).

### Direct Python execution

```bash
# Ensure virtual environment is activated and you are in this directory
python server.py
```

The server listens on **port 8005** with MCP over **streamable HTTP** at path **`/mcp`** (e.g. `http://localhost:8005/mcp`).

### Docker (optional)

From the `ResInsight` directory (where the `Dockerfile` is):

```bash
docker build -t resinsight-mcp .
docker run -d --name resinsight --restart unless-stopped -p 8005:8005 --env-file .env resinsight-mcp
```

Ensure `.env` contains `GITHUB_TOKEN`, `MCP_TOKEN`, and any other variables you need. The image exposes port **8005**.

---

## ResInsight MCP Access

### Hosted deployment

If you only want to use ResInsight, no clone is required. Connect your MCP client to:

`http://52.45.172.212:8005/mcp`

Use the lab-issued Bearer token in your client configuration. The hosted server runs in Docker as `resinsight` with port mapping `8005:8005`.

### Local setup

To run ResInsight locally or modify the server:

1. Clone this fork or the upstream repository.
2. Create and activate a Python virtual environment.
3. Install dependencies from `requirements.txt`.
4. Create a `.env` file with `GITHUB_TOKEN` and `MCP_TOKEN`.
5. Start the server with `python server.py`, or build and run the Docker image.

### Claude Desktop configuration

For local, self-hosted Claude Desktop usage, use the direct Python MCP server entry:

```json
{
  "mcpServers": {
    "resilientdb-assistant": {
      "command": "python",
      "args": ["C:/path/to/incubator-resilientdb/ecosystem/ai-tools/mcp/ResInsight/server.py"],
      "env": {
        "PYTHONPATH": "C:/path/to/incubator-resilientdb/ecosystem/ai-tools/mcp/ResInsight",
        "GITHUB_TOKEN": "ghp_...",
        "MCP_TOKEN": "same_as_server_env_or_cloud_issued_token"
      }
    }
  }
}
```

For remote/deployed usage in Claude builds that require a bridge, use the hosted `/mcp` URL with `mcp-remote`:

```json
{
  "mcpServers": {
    "ResInsight: AI-driven developer onboarding ecosystem": {
      "command": "npx",
      "args": [
        "-y",
        "mcp-remote",
        "http://52.45.172.212:8005/mcp",
        "--header",
        "Authorization: Bearer MCP_TOKEN"
      ],
      "env": {
        "MCP_REMOTE_CONFIG_DIR": "C:/Users/your-user/.mcp-auth"
      }
    }
  }
}
```

`MCP_REMOTE_CONFIG_DIR` is optional and only controls where the bridge stores auth/session files. If your client supports native remote HTTP MCP fields, you can use `url` + `headers` instead of the bridge.

---

## Authentication & Security

ResInsight uses **two complementary credentials**:

### 1. MCP access token (`MCP_TOKEN`)

**Purpose:** Authenticates clients to the MCP HTTP server (`Authorization: Bearer …`).

**Two cases:**

| Scenario | Who sets `MCP_TOKEN` |
|----------|----------------------|
| **You run ResInsight locally** (your machine, Docker on your host, etc.) | **You.** Pick any strong random value, put it in the server’s `.env`, and configure your MCP client to send the **same** token. No need to contact the team. |
| **You connect to ResilientDB’s cloud / team-hosted instance** | **The ResilientDB team.** Use the token we provide for that endpoint; clients must not use a self-made token against our server unless we’ve told you to. |

Treat every token as a secret: do not commit `.env` or share tokens in chat.

If `MCP_TOKEN` is unset on **your own** server, it can start without Bearer enforcement (only for local, non-exposed use).

### 2. GitHub Personal Access Token (`GITHUB_TOKEN`)

**Purpose:** Lets the server call the GitHub API to analyze repositories.

**Typical scopes:**
- `public_repo` — public repositories
- `repo` — private repositories, if needed

**How to create:**

1. GitHub → **Settings** → **Developer settings** → **Personal access tokens** → **Tokens (classic)** → **Generate new token**
2. Choose the scopes above as needed.
3. Copy the token once and add to `.env`: `GITHUB_TOKEN=ghp_...`

---

## How to Use

### Method 1: MCP Client Integration (Recommended) : Scroll for MCP Client Setup

1. **Setup MCP Client** (e.g., Claude Desktop)
   - Install Claude Desktop or your preferred MCP client
   - Configure the MCP server in your client settings
   - Add server configuration pointing to `server.py`

2. **Start Conversing**
   ```
   You: "I'm new to ResilientDB, where should I start?"
   
   Assistant: Welcome to ResilientDB! Let me guide you through 
   the fundamentals and get you started...
   ```

---

## Example Scenarios

### 🎓 Complete Beginner Workflow

Start with the basics and progressively learn:

```bash
# 1. Introduction
"What is ResilientDB and why should I care?"

# 2. Setup guidance
"How do I install ResilientDB on Windows?"

# 3. Concept learning
"Explain Byzantine fault tolerance in simple terms"

# 4. Hands-on practice
"Show me how to create a simple transaction"
```

### 🔧 Troubleshooting Workflow

Get targeted help when you encounter issues:

```bash
# 1. Report the problem
"I'm getting cmake build errors when compiling ResilientDB"

# 2. Get specific guidance
"The error says 'grpc++/grpc++.h file not found'"

# 3. Verify the solution
"How do I check if gRPC is properly installed?"
```

### 🚀 Advanced Learning Workflow

Deep dive into system design and optimization:

```bash
# 1. Algorithm deep-dive
"Explain the PBFT consensus algorithm in detail"

# 2. Performance tuning
"How can I benchmark ResilientDB throughput?"

# 3. Code exploration
"Show me the transaction processing implementation"
```

---

## MCP Client Setup

### Claude Desktop Configuration

Add the following to your Claude Desktop MCP configuration file:

```json
{
  "mcpServers": {
    "resilientdb-assistant": {
      "command": "python",
      "args": ["C:/path/to/incubator-resilientdb/ecosystem/ai-tools/mcp/ResInsight/server.py"],
      "env": {
        "PYTHONPATH": "C:/path/to/incubator-resilientdb/ecosystem/ai-tools/mcp/ResInsight",
        "GITHUB_TOKEN": "ghp_...",
        "MCP_TOKEN": "same_as_server_env_or_cloud_issued_token"
      }
    }
  }
}
```

Set `GITHUB_TOKEN` and `MCP_TOKEN` to match your setup: **same self-chosen secret as the server** when local, or the **team-issued token** when pointing the client at the cloud instance (or rely on `.env` if the client loads it).

### VS Code with Continue Extension

Add the following to your Continue configuration:

```json
{
  "mcp": [
    {
      "serverName": "resilientdb-assistant",
      "command": "python",
      "args": ["server.py"],
      "cwd": "/path/to/incubator-resilientdb/ecosystem/ai-tools/mcp/ResInsight",
      "env": {
        "GITHUB_TOKEN": "ghp_...",
        "MCP_TOKEN": "same_as_server_env_or_cloud_issued_token"
      }
    }
  ]
}
```

---

## Educational Features

### Available Query Types

| Query Type | Example |
|-----------|---------|
| **Installation Help** | "How do I install ResilientDB?" |
| **Troubleshooting** | "I'm getting build errors" |
| **Concept Explanations** | "What is consensus?" |
| **Consensus Algorithms** | "Explain PBFT algorithm" |
| **Performance** | "How to optimize throughput?" |
| **Code Exploration** | "Show transaction code" |
| **Docker Help** | "Help with containers" |
| **General Questions** | "Tell me about ResilientDB" |

### Learning Progression

#### 🟢 Beginner Level
- Questions on fundamentals
  ```bash
  "What is ResilientDB?"
  "Why use blockchain databases?"  
  "How do I get started?"
  ```

#### 🟡 Intermediate Level
- Understanding core concepts
  ```bash
  "Explain Byzantine fault tolerance"
  "How does consensus work?"
  "Show me code examples"
  "How is inventory upload implemented in Arrayan?"
  ```

#### 🔴 Advanced Level
- Deep system design knowledge
  ```bash
  "Deep dive into PBFT algorithm"
  "Performance tuning parameters"
  "Network partition handling"
  "What is the relation between these files for Inventory Upload in Arrayan?"
  ```

### Verify Your Setup

```bash
# Check Python version
python --version

# Check installed packages
pip list | grep -E "(mcp|docker|fastmcp)"

# Test Docker (if using)
docker --version
```


---

## Troubleshooting

### Common Issues & Solutions

#### "Module not found" errors

```bash
# Ensure virtual environment is activated
.venv\Scripts\activate  # Windows
source .venv/bin/activate  # Linux/Mac

# Reinstall dependencies
pip install -r requirements.txt
```

#### MCP client connection issues

- Check file paths in MCP configuration (paths should point at `ecosystem/ai-tools/mcp/ResInsight` in the monorepo)
- Ensure Python path is correct
- Verify server starts without errors
- Ensure **`MCP_TOKEN`** matches the server: **your chosen secret** for local runs, or the **team-issued token** for the cloud instance; set **`GITHUB_TOKEN`** in `.env` or the client `env` block

---

## References

- 📚 [Model Context Protocol Documentation](https://modelcontextprotocol.io/)
- 🔗 [ResilientDB GitHub Project](https://github.com/apache/incubator-resilientdb)
- 🐳 [Docker Documentation](https://docs.docker.com/)

---

## Getting Started

### Quick Launch

1. **Create `.env`** with `GITHUB_TOKEN` and `MCP_TOKEN` (pick your own secret for local runs; use the **team-issued token** only when connecting to the hosted cloud instance).

2. **Start the server**
   ```bash
   python server.py
   ```

3. **Configure your MCP client** (Claude Desktop, VS Code, or Cursor)

4. **Ask your first question**
   ```
   "I'm new to ResilientDB, where should I start?"
   ```

**Transform your ResilientDB learning from documentation reading to interactive conversation!** 🚀

---

## License

Licensed under the **Apache License, Version 2.0**. 
See the Apache ResilientDB LICENSE file for details.
All source files include the required Apache License 2.0 header.

---

## Acknowledgements

| Role | Name |
|------|------|
| **Developer** | Kunjal Agrawal |
| **Advisor** | Dr. Mohammad Sadoghi |
| **Lab** | ExpoLab |

---
