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

- **Python 3.8+** (3.9+ recommended)
- **Docker** (optional but recommended for ResilientDB examples)
- **MCP-compatible client** (Claude Desktop, Continue, or any MCP client)

### Installation Steps

1. **Clone the repository**
   ```bash
   git clone <repository-url>
   cd ResInsight
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
   
   Create a `.env` file in the ResInsight directory:
   ```env
   GITHUB_TOKEN=ghp_your_token_here
   MCP_TOKEN=your_mcp_token_here
   ```

---

## How to Run

### Direct Python Execution

```bash
# Ensure virtual environment is activated
python server.py
```

---

## Authentication & Security

ResInsight implements a **two-layer authentication system** for secure operation:

### 1. MCP Access Token (Client Authentication)

**Purpose:** Authenticates clients connecting to the MCP server

**Setup:**
- Contact ExpoLab administrator (Harish or Bisman) to receive an MCP access token
- Add to your `.env` file: `MCP_TOKEN=your_token_here`
- Keep your token confidential and do not share it

### 2. GitHub Personal Access Token (Server-Side)

**Purpose:** Enables the server to access GitHub repositories via API

**Required Scopes:**
- `public_repo` - For accessing public repositories
- `repo` - For accessing private repositories (if needed)

**How to Generate:**

1. Go to GitHub Settings → Developer Settings → Personal Access Tokens → Tokens (classic)
2. Click "Generate new token"
3. Select required scopes: `repo` or `public_repo`
4. Copy the token immediately (it won't be shown again)
5. Add to your `.env` file: `GITHUB_PAT=ghp_your_token_here`

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
      "args": ["C:/path/to/your/project/server.py"],
      "env": {
        "PYTHONPATH": "C:/path/to/your/project"
      }
    }
  }
}
```

### VS Code with Continue Extension

Add the following to your Continue configuration:

```json
{
  "mcp": [
    {
      "serverName": "resilientdb-assistant",
      "command": "python",
      "args": ["server.py"],
      "cwd": "/path/to/your/project"
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
  "How is inventory uplaod implemented in Arrayan?"
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

- Check file paths in MCP configuration
- Ensure Python path is correct
- Verify server starts without errors
- Review `.env` file for required tokens

---

## References

- 📚 [Model Context Protocol Documentation](https://modelcontextprotocol.io/)
- 🔗 [ResilientDB GitHub Project](https://github.com/apache/incubator-resilientdb)
- 🐳 [Docker Documentation](https://docs.docker.com/)

---

## Getting Started

### Quick Launch

1. **Start the server**
   ```bash
   python server.py
   ```

2. **Configure your MCP client** (Claude Desktop or VS Code)

3. **Ask your first question**
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
