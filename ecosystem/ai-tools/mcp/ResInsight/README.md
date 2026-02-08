# ResilientDB Educational Assistant MCP Server

An **interactive, conversational educational assistant** built with the Model Context Protocol (MCP) that helps students learn ResilientDB distributed database systems through natural language interactions.

## 🎯 **What This Is**

This MCP server transforms learning ResilientDB from reading static documentation into having **natural conversations** with an AI assistant that:

- **Understands your questions** in plain English
- **Provides educational, context-aware responses** 
- **Helps troubleshoot problems** with specific solutions
- **Guides you through concepts** from beginner to advanced
- **Offers practical examples** and hands-on learning

## 🚀 **Quick Start**

### **Prerequisites**
- **Python 3.8+** (3.9+ recommended)
- **Docker** (optional but recommended for ResilientDB examples)
- **MCP-compatible client** (Claude Desktop, Continue, or any MCP client)

### **Installation**

1. **Clone and setup:**
```bash
# Clone the repository
git clone <repository-url>
cd resilientdb-mcp-server

# Create virtual environment
python -m venv .venv
```

2. **Activate environment:**
```bash
# Windows
.venv\Scripts\activate

# Linux/Mac
source .venv/bin/activate
```

3. **Install dependencies:**
```bash
pip install -r requirements.txt
```

## 🏃‍♂️ **How to Run**

### **Option 1: Windows Startup Script (Easiest)**
```bash
start_server.bat
```
This script will:
- Check dependencies
- Activate virtual environment
- Start the MCP server
- Show helpful startup information

### **Option 2: Direct Python Execution**
```bash
# Make sure virtual environment is activated
python resilientdb_mcp_server.py
```

### **Option 3: Docker Container**
```bash
# Build the container
docker build -t resilientdb-mcp-server .

# Run the container
docker run -p 8000:8000 resilientdb-mcp-server
```

## 🎮 **How to Use the Project**

### **Method 1: MCP Client Integration (Recommended)**

1. **Setup MCP Client** (e.g., Claude Desktop):
   - Install Claude Desktop or your preferred MCP client
   - Configure the MCP server in your client settings
   - Add server configuration pointing to `resilientdb_mcp_server.py`

2. **Start Conversations**:
   ```
   🎓 You: "I'm new to ResilientDB, where should I start?"
   🤖 Assistant: Welcome to ResilientDB! Let me guide you through...
   ```

### **Method 2: Interactive Demo Mode**

Run the standalone demo to test without MCP client:
```bash
python demo_conversational_assistant.py
```

**Demo Options:**
1. **Watch Sample Conversations** - See example student interactions
2. **Interactive Mode** - Ask your own questions directly
3. **Both** - Sample conversations followed by interactive mode

### **Method 3: Testing and Development**

Run the comprehensive test suite:
```bash
python test_conversational_assistant.py
```

## 💬 **Example Usage Scenarios**

### **🎓 Complete Beginner Workflow**

```bash
# 1. Start with basics
"What is ResilientDB and why should I care?"

# 2. Get setup help  
"How do I install ResilientDB on Windows?"

# 3. Learn concepts
"Explain Byzantine fault tolerance in simple terms"

# 4. Practical examples
"Show me how to create a simple transaction"
```

### **🔧 Troubleshooting Workflow**

```bash
# 1. Report your error
"I'm getting cmake build errors when compiling ResilientDB"

# 2. Get specific help
"The error says 'grpc++/grpc++.h file not found'"

# 3. Verify solution
"How do I check if gRPC is properly installed?"
```

### **🚀 Advanced Learning Workflow**

```bash
# 1. Deep dive into algorithms
"Explain the PBFT consensus algorithm in detail"

# 2. Performance optimization
"How can I benchmark ResilientDB throughput?"

# 3. Code exploration
"Show me the transaction processing implementation"
```

## 🛠️ **MCP Client Configuration**

### **Claude Desktop Setup**

Add to your Claude Desktop MCP configuration:

```json
{
  "mcpServers": {
    "resilientdb-assistant": {
      "command": "python",
      "args": ["C:/path/to/your/project/resilientdb_mcp_server.py"],
      "env": {
        "PYTHONPATH": "C:/path/to/your/project"
      }
    }
  }
}
```

### **VS Code with Continue Extension**

Add to your Continue configuration:

```json
{
  "mcp": [
    {
      "serverName": "resilientdb-assistant",
      "command": "python",
      "args": ["resilientdb_mcp_server.py"],
      "cwd": "/path/to/your/project"
    }
  ]
}
```

## 🎓 **Educational Features You Can Use**

### **Available Query Types**
- **Installation Help**: "How do I install ResilientDB?"
- **Troubleshooting**: "I'm getting build errors"
- **Concept Explanations**: "What is consensus?"
- **Consensus Questions**: "Explain PBFT algorithm"
- **Performance Questions**: "How to optimize throughput?"
- **Code Exploration**: "Show transaction code"
- **Docker Help**: "Help with containers"
- **General Questions**: "Tell me about ResilientDB"

### **Learning Progression**

**Beginner Level:**
```bash
"What is ResilientDB?"
"Why use blockchain databases?"  
"How do I get started?"
```

**Intermediate Level:**
```bash
"Explain Byzantine fault tolerance"
"How does consensus work?"
"Show me code examples"
```

**Advanced Level:**
```bash
"Deep dive into PBFT algorithm"
"Performance tuning parameters"
"Network partition handling"
```

## 🔍 **Verification & Testing**

### **Test the Server**
```bash
# Run all tests
python test_conversational_assistant.py

# Test specific functionality
python -c "
import asyncio
from resilientdb_mcp_server import ask_resilientdb_assistant
print(asyncio.run(ask_resilientdb_assistant('Hello, test the assistant')))
"
```

### **Check Dependencies**
```bash
# Verify Python version
python --version

# Check installed packages
pip list | grep -E "(mcp|docker|fastmcp)"

# Test Docker (if using)
docker --version
```

## 📁 **Project Structure**

```
resilientdb-mcp-server/
├── 📄 resilientdb_mcp_server.py           # 🎯 Main MCP server
├── 🎮 demo_conversational_assistant.py    # 🎪 Interactive demo
├── 🧪 test_conversational_assistant.py    # 🔬 Test suite  
├── 🚀 start_server.bat                    # 🪟 Windows launcher
├── 📋 requirements.txt                    # 📦 Dependencies
├── 🐳 Dockerfile                          # 📦 Container setup
├── 📖 README.md                           # 📚 This guide
└── 📄 CONVERSATIONAL_ASSISTANT_SUMMARY.md # 🔧 Technical details
```

## 🚨 **Troubleshooting**

### **Common Issues**

**"Module not found" errors:**
```bash
# Make sure virtual environment is activated
.venv\Scripts\activate  # Windows
source .venv/bin/activate  # Linux/Mac

# Reinstall dependencies
pip install -r requirements.txt
```

**"Docker unavailable" warnings:**
```bash
# Install Docker Desktop, or ignore (Docker is optional)
# The assistant works without Docker for most features
```

**MCP client connection issues:**
```bash
# Check file paths in MCP configuration
# Ensure Python path is correct
# Verify server starts without errors
```

### **Getting Help**

1. **Run the demo**: `python demo_conversational_assistant.py`
2. **Check tests**: `python test_conversational_assistant.py`  
3. **Ask the assistant**: "I'm having trouble with [specific issue]"
4. **Check logs**: Look for error messages in terminal output

## 📚 **Learning Resources**

### **Start Here**
- Run: `python demo_conversational_assistant.py`
- Ask: "What is ResilientDB and why should I use it?"
- Follow the guided learning path in responses

### **References**
- [Model Context Protocol](https://modelcontextprotocol.io/)
- [ResilientDB Project](https://github.com/apache/incubator-resilientdb)
- [FastMCP Framework](https://github.com/jlowin/fastmcp)
- [Docker Documentation](https://docs.docker.com/)

---

## 🎉 **Ready to Start Learning?**

1. **Run the server**: `start_server.bat` (Windows) or `python resilientdb_mcp_server.py`
2. **Try the demo**: `python demo_conversational_assistant.py`
3. **Ask your first question**: "I'm new to ResilientDB, where should I start?"

**Transform your ResilientDB learning from documentation reading to interactive conversation!** 🚀
