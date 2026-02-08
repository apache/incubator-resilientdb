# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.

import ast
from fastmcp import FastMCP
from pydantic import BaseModel, Field
from typing import List, Optional, Dict, Any
import httpx
import faiss
import numpy as np
import networkx as nx
import matplotlib.pyplot as plt
import io
import base64
import ast
import numpy as np
import os
import json
from dotenv import load_dotenv

load_dotenv() # Load environment variables from .env file

# Optional: Import SentenceTransformer (only needed for semantic search)
try:
    from sentence_transformers import SentenceTransformer
    SENTENCE_TRANSFORMER_AVAILABLE = True
    print("[OK] SentenceTransformer available")
except ImportError:
    SENTENCE_TRANSFORMER_AVAILABLE = False
    SentenceTransformer = None
    print("[WARNING] SentenceTransformer not available - semantic search disabled")

# Import ResilientDB Knowledge Base
try:
    from ResilientDBKnowledgeBase import ResilientDBKnowledgeBase
    KNOWLEDGE_BASE_AVAILABLE = True
    print("[OK] ResilientDB Knowledge Base loaded")
except ImportError as e:
    KNOWLEDGE_BASE_AVAILABLE = False
    print(f"[WARNING] ResilientDB Knowledge Base not available: {e}")

# -------------------------
# Authentication and Token Setup
# -------------------------
MCP_TOKEN = os.getenv("MCP_TOKEN")  # Lab-provided MCP access token
GITHUB_ENTERPRISE_TOKEN = os.getenv("GITHUB_ENTERPRISE_TOKEN")  # GitHub Enterprise token

# Use enterprise token, fallback to environment variable
GITHUB_TOKEN = GITHUB_ENTERPRISE_TOKEN or os.getenv("GITHUB_TOKEN")

def get_auth_headers():
    """Get GitHub API authentication headers"""
    if GITHUB_TOKEN:
        return {"Authorization": f"token {GITHUB_TOKEN}"}
    return {}

# -------------------------
# FAISS vector search setup
# -------------------------
# Global variables to track index state
index = None
index_dimension = None  # Track the dimension used for the index
metadata = []
model = None

if SENTENCE_TRANSFORMER_AVAILABLE:
    try:
        model = SentenceTransformer('all-MiniLM-L6-v2')
        print("[OK] Sentence transformer model loaded")
    except Exception as e:
        print(f"[WARNING] Failed to load model: {e}")
        model = None

def embed_text(text: str) -> np.ndarray:
    global index_dimension
    if model is None:
        raise RuntimeError("SentenceTransformer model not available - semantic search disabled")
    embedding = model.encode(text)

    # Track dimension on first use
    if index_dimension is None:
        index_dimension = len(embedding)

    return embedding

# -------------------------
# Pydantic Models
# -------------------------
class FileSummary(BaseModel):
    filename: str
    code_summary: str
    insights: List[str] = Field(default_factory=list)

class RepoSummary(BaseModel):
    repo_name: str
    total_files: int
    files: List[str]

class SearchResult(BaseModel):
    filepath: str
    code_snippet: str
    score: float

class RepoInsights(BaseModel):
    repo_name: str
    insights: List[str] = Field(default_factory=list)

# -------------------------
# FastMCP Server Setup
# -------------------------
mcp = FastMCP(name="ResInsight: AI-driven developer onboarding ecosystem")

# -------------------------
# Authentication Setup Function
# -------------------------
def setup_authentication():
    """Setup authentication middleware after FastMCP initialization"""
    from starlette.middleware.base import BaseHTTPMiddleware
    from starlette.requests import Request
    from starlette.responses import JSONResponse
    
    class AuthMiddleware(BaseHTTPMiddleware):
        async def dispatch(self, request: Request, call_next):
            # Skip authentication for docs/health endpoints
            if request.url.path in ["/health", "/", "/docs", "/openapi.json", "/redoc"]:
                return await call_next(request)
            
            # Check Authorization header
            auth_header = request.headers.get("Authorization")
            
            if not auth_header:
                return JSONResponse(
                    status_code=401,
                    content={
                        "error": "unauthorized",
                        "message": "Missing Authorization header",
                        "hint": "Include header: Authorization: Bearer YOUR_LAB_TOKEN"
                    }
                )
            
            if not auth_header.startswith("Bearer "):
                return JSONResponse(
                    status_code=401,
                    content={
                        "error": "unauthorized", 
                        "message": "Invalid Authorization format",
                        "hint": "Use: Authorization: Bearer YOUR_LAB_TOKEN"
                    }
                )
            
            token = auth_header.replace("Bearer ", "")
            
            if not MCP_TOKEN:
                return JSONResponse(
                    status_code=500,
                    content={
                        "error": "server_error",
                        "message": "Server not configured with MCP_TOKEN"
                    }
                )
            
            if token != MCP_TOKEN:
                return JSONResponse(
                    status_code=403,
                    content={
                        "error": "forbidden",
                        "message": "Invalid authentication token"
                    }
                )
            
            response = await call_next(request)
            return response
    
    try:
        if hasattr(mcp, '_app'):
            mcp._app.add_middleware(AuthMiddleware)
            print("[✓] Authentication middleware enabled")
            return True
        else:
            print("[✗] Warning: Could not add auth middleware - FastMCP API changed")
            return False
    except Exception as e:
        print(f"[✗] Error adding auth middleware: {e}")
        return False

# -------------------------
# ResilientDB Knowledge Base Initialization
# -------------------------
resilientdb_knowledge = None
if KNOWLEDGE_BASE_AVAILABLE:
    try:
        resilientdb_knowledge = ResilientDBKnowledgeBase()
        print("[OK] ResilientDB Knowledge Base initialized and ready")
    except Exception as e:
        print(f"[ERROR] Failed to initialize knowledge base: {e}")
        resilientdb_knowledge = None

# -------------------------
# Helper functions
# -------------------------
def split_code_into_chunks(code: str, max_lines: int = 200) -> List[str]:
    lines = code.splitlines()
    return ["\n".join(lines[i:i+max_lines]) for i in range(0, len(lines), max_lines)]

async def fetch_repo_tree(owner: str, repo: str, branch: str = "main") -> List[dict]:
    """Fetch repository tree with better branch handling"""
    headers = get_auth_headers()
    
    # Try both main and master branches
    branches_to_try = [branch]
    if branch == "main":
        branches_to_try.append("master")
    elif branch == "master":
        branches_to_try.append("main")
    
    async with httpx.AsyncClient(follow_redirects=True, timeout=30.0) as client:
        last_error = None
        for try_branch in branches_to_try:
            try:
                url = f"https://api.github.com/repos/{owner}/{repo}/git/trees/{try_branch}?recursive=1"
                response = await client.get(url, headers=headers)
                response.raise_for_status()
                data = response.json()
                return [item for item in data.get("tree", []) if item['type'] == 'blob']
            except Exception as e:
                last_error = e
                continue
        
        # If all branches failed, raise the last error
        if last_error:
            raise last_error
        return []

async def fetch_raw_file(owner: str, repo: str, filepath: str, branch: str = "main") -> Optional[str]:
    """Fetch raw file with better branch handling"""
    branches_to_try = [branch]
    if branch == "main":
        branches_to_try.append("master")
    elif branch == "master":
        branches_to_try.append("main")
    
    async with httpx.AsyncClient(follow_redirects=True, timeout=30.0) as client:
        for try_branch in branches_to_try:
            try:
                url = f"https://raw.githubusercontent.com/{owner}/{repo}/{try_branch}/{filepath}"
                r = await client.get(url)
                if r.status_code == 200:
                    return r.text
            except:
                continue
    return None

# Alternate parser using Python's built-in ast module
def parse_python_functions_ast(code: str) -> List[str]:
    functions = []
    try:
        tree = ast.parse(code)
        for node in ast.walk(tree):
            if isinstance(node, ast.FunctionDef):
                # Generate a simple string summary (function signature)
                args = [arg.arg for arg in node.args.args]
                arglist = ", ".join(args)
                functions.append(f"def {node.name}({arglist}):")
    except Exception:
        # If parsing fails, return empty list to avoid crashing MCP server
        return []
    return functions

# -------------------------
# MCP Tools
# -------------------------
# Helper function for Contents API recursion (NOT an MCP tool)
async def _fetch_files_recursive(
    owner: str, 
    repo: str, 
    branch: str, 
    path: str = ""
) -> List[str]:
    """
    Internal helper to recursively fetch files using Contents API.
    """
    headers = get_auth_headers()
    files = []
    url = f"https://api.github.com/repos/{owner}/{repo}/contents/{path}?per_page=100"
    
    async with httpx.AsyncClient(follow_redirects=True, timeout=30.0) as client:
        while url:
            try:
                response = await client.get(url, headers=headers)
                response.raise_for_status()
            except Exception as e:
                print(f"Error fetching {url}: {e}")
                break
            
            # Handle Link header pagination
            link_header = response.headers.get('Link', '')
            next_url = None
            if 'rel="next"' in link_header:
                next_url = [link.split(';')[0].strip('<> \t') for link in link_header.split(',') 
                           if 'rel="next"' in link][0]
            
            data = response.json()
            
            # Single file response (base64 content)
            if isinstance(data, dict) and 'content' in data:
                files.append(data['path'])
                break
            
            # Directory listing (list of items)
            for item in data:
                if item['type'] == 'file':
                    files.append(item['path'])
                elif item['type'] == 'dir':
                    # Call the HELPER function recursively
                    dir_files = await _fetch_files_recursive(
                        owner, repo, branch, item['path']
                    )
                    files.extend(dir_files)
            
            url = next_url  # Continue pagination
    
    return files


# MCP tool with hybrid approach
@mcp.tool(name="list_github_repo_files")
async def list_github_repo_files(
    owner: str, 
    repo: str, 
    branch: str = "main", 
    path: str = ""
) -> List[str]:
    """
    This tool fetches the list of files in a particular repository using the github api mentioned in the MCP tool and not from any online source. 
    """
    headers = get_auth_headers()
    
    # FAST PATH: Try Tree API first (single request for entire repo)
    try:
        async with httpx.AsyncClient(follow_redirects=True, timeout=30.0) as client:
            # Try to get the branch SHA first for more reliable results
            try:
                ref_url = f"https://api.github.com/repos/{owner}/{repo}/git/ref/heads/{branch}"
                ref_response = await client.get(ref_url, headers=headers)
                ref_response.raise_for_status()
                sha = ref_response.json()['object']['sha']
            except:
                # Fallback: use branch name directly
                sha = branch
            
            # Get the full tree recursively in one call
            tree_url = f"https://api.github.com/repos/{owner}/{repo}/git/trees/{sha}?recursive=1"
            tree_response = await client.get(tree_url, headers=headers)
            tree_response.raise_for_status()
            data = tree_response.json()
            
            # Check if the response was truncated (repo too large)
            if not data.get('truncated', False):
                # Success! Return all files from tree
                files = [item['path'] for item in data.get('tree', []) 
                        if item['type'] == 'blob']
                print(f"✓ Tree API: Found {len(files)} files in one request")
                return files
            else:
                # Repo is too large, tree was truncated
                print(f"⚠ Tree API truncated, falling back to Contents API...")
                
    except Exception as e:
        # Tree API failed for some reason
        print(f"⚠ Tree API failed ({str(e)}), using Contents API...")
    
    # SLOW PATH: Fall back to Contents API with recursive traversal
    print(f"→ Using Contents API (may take longer for large repos)...")
    files = await _fetch_files_recursive(owner, repo, branch, path)
    print(f"✓ Contents API: Found {len(files)} files")
    return files

@mcp.tool(name="Get_Repo_Summary")
async def getRepoSummary(owner: str, repo: str, branch: str = "main") -> str:
    """
    Get the summary of a particular repository. Ask the user about the branch they want the information about.This MCP tool will give the summary of the repo as to what type of files, how many files, what it does, what is it about, what is it implementing..
    """
    try:
        tree = await fetch_repo_tree(owner, repo, branch)
        files = [item['path'] for item in tree]
        
        # Analyze file types
        file_types = {}
        for f in files:
            ext = f.split('.')[-1] if '.' in f else 'no_extension'
            file_types[ext] = file_types.get(ext, 0) + 1
        
        result = f"Repository: {owner}/{repo}\n"
        result += f"Branch: {branch}\n"
        result += f"Total files: {len(files)}\n\n"
        result += "File types:\n"
        for ext, count in sorted(file_types.items(), key=lambda x: x[1], reverse=True)[:10]:
            result += f"  .{ext}: {count} files\n"
        
        return result
    except Exception as e:
        return f"Error fetching repository summary: {str(e)}\nTry using branch='master' if the default 'main' doesn't work."

@mcp.tool(name="getFileSummary")
async def getFileSummary(owner: str, repo: str, filenames: List[str], branch: str = "main") -> List[FileSummary]:
    """
    Generate the summary of a particular file in a branch. Do not provide extra unnecessary information. To the point and specific information and what the file does..
    """
    file_summaries = []
    for filename in filenames:
        code = await fetch_raw_file(owner, repo, filename, branch)
        if code:
            # Simple summary: line count, presence of TODOs or defs
            summary_text = f"{filename} has {len(code.splitlines())} lines of code."
            insights = []
            if "TODO" in code:
                insights.append("Contains TODO comments.")
            if "def " in code:
                insights.append("Contains function definitions.")
            file_summaries.append(FileSummary(filename=filename, code_summary=summary_text, insights=insights))
        else:
            file_summaries.append(FileSummary(filename=filename, code_summary="File not found or inaccessible", insights=[]))
    return file_summaries

@mcp.tool(name="Ingest_Repo_Code")
async def ingest_repo_code(owner: str, repo: str, branch: str = "main") -> str:
    """
    Fetch repo files, chunk code, generate embeddings and index them
    """
    global index, metadata, index_dimension
    
    if model is None:
        return "⚠️ Semantic search not available - SentenceTransformer not installed"
    
    try:
        # Fetch all files using the helper function directly
        files = await _fetch_files_recursive(owner, repo, branch, "")
        
        if not files:
            # Try alternative method
            try:
                tree = await fetch_repo_tree(owner, repo, branch)
                files = [item['path'] for item in tree]
            except:
                return f"Could not fetch files from {owner}/{repo}. Try checking the branch name."
        
        # Filter for code files only
        code_extensions = ['.py', '.js', '.jsx', '.ts', '.tsx', '.java', '.cpp', '.c', '.go', '.rs']
        code_files = [f for f in files if any(f.endswith(ext) for ext in code_extensions)]
        
        if not code_files:
            return f"No code files found in {owner}/{repo}"
        
        # Collect all chunks and their metadata
        all_chunks = []
        all_metadata = []
        
        for filepath in code_files[:50]:  # Limit to first 50 files to avoid timeout
            try:
                content = await fetch_raw_file(owner, repo, filepath, branch)
                if content:
                    # Simple chunking by lines
                    lines = content.split('\n')
                    chunk_size = 50
                    
                    for i in range(0, len(lines), chunk_size):
                        chunk = '\n'.join(lines[i:i+chunk_size])
                        if chunk.strip():
                            all_chunks.append(chunk)
                            all_metadata.append({
                                'file': filepath,
                                'chunk_start': i,
                                'chunk_end': min(i+chunk_size, len(lines))
                            })
            except Exception as e:
                print(f"Skipping {filepath}: {e}")
                continue
        
        if not all_chunks:
            return f"No content could be extracted from {owner}/{repo}"
        
        # Generate embeddings
        print(f"Generating embeddings for {len(all_chunks)} chunks...")
        embeddings = np.array([embed_text(chunk) for chunk in all_chunks])
        
        # Store dimension
        index_dimension = embeddings.shape[1]
        
        # Create FAISS index
        index = faiss.IndexFlatL2(index_dimension)
        index.add(embeddings.astype('float32'))
        
        # Store metadata
        metadata = all_metadata
        
        return f"✓ Successfully indexed {len(all_chunks)} code chunks from {len(code_files)} files. Index dimension: {index_dimension}"
        
    except Exception as e:
        return f"Error ingesting repo: {str(e)}"


@mcp.tool(name="Semantic_Search")
async def semanticSearch(query: str, top_k: int = 5) -> str:
    """
    Perform semantic search on the data after vectorization and indexing.
    """
    global index, metadata, index_dimension
    
    # Check if index exists
    if index is None or len(metadata) == 0:
        return "⚠️ No index found. Please run 'Ingest_Repo_Code' first to build the index."
    
    try:
        # Generate query embedding
        qv = embed_text(query)
        
        # Verify dimension match
        if len(qv) != index_dimension:
            return f"⚠️ Dimension mismatch: Query has {len(qv)} dimensions but index expects {index_dimension}. Please rebuild the index."
        
        # Reshape for FAISS
        qv = qv.reshape(1, -1).astype('float32')
        
        # Search
        D, I = index.search(qv, min(top_k, len(metadata)))
        
        # Format results
        results = []
        for rank, (dist, idx) in enumerate(zip(D[0], I[0]), 1):
            if idx < len(metadata):
                meta = metadata[idx]
                results.append(
                    f"\n{rank}. File: {meta['file']}\n"
                    f"   Lines: {meta['chunk_start']}-{meta['chunk_end']}\n"
                    f"   Distance: {dist:.4f}"
                )
        
        if not results:
            return "No results found."
        
        return f"Search results for: '{query}'\n" + "\n".join(results)
        
    except AssertionError as e:
        return f"⚠️ Dimension mismatch error. Index dimension: {index_dimension}, Query dimension: {len(embed_text(query))}. Please rebuild the index."
    except Exception as e:
        return f"⚠️ Search error: {str(e)}"

@mcp.tool(name="Get_File_Functions")
async def getFileFunctions(owner: str, repo: str, filepath: str, branch: str = "main") -> List[str]:
    """
    The MCP tool getFileFunctions extracts Python function definitions from a given file in a GitHub repository. Here's what it does in detail:

    It fetches the raw source code of the specified file from the GitHub repo using fetch_raw_file.

    If the file content is empty or unavailable, it returns an empty list.

    Otherwise, it calls parse_python_functions_ast, which uses Python's built-in ast module to:

    Parse the source code into an abstract syntax tree (AST).

    Traverse the AST to find all function definitions (ast.FunctionDef nodes).

    For each function found, it creates a simple signature string like def function_name(arg1, arg2):.

    The tool returns a list of these function signature strings
    Purpose:
    This tool helps junior developers (or any users) quickly understand the structure of Python files by listing the function definitions and their arguments without needing to manually inspect the file line-by-line. It supports the larger goal of the MCP-powered Repo Analyzer to provide guided navigation and understanding of a codebase
    """
    code = await fetch_raw_file(owner, repo, filepath, branch)
    if not code:
        return []
    funcs = parse_python_functions_ast(code)
    return funcs

# =========================================================================
# RESILIENTDB KNOWLEDGE BASE QUERY TOOL - USE THIS FIRST FOR RESILIENTDB!
# =========================================================================

@mcp.tool(name="SearchResilientDBKnowledge")
async def search_resilientdb_knowledge(query: str, category: Optional[str] = None) -> str:
    """
    🎓 CRITICAL: USE THIS TOOL FIRST for ANY question about ResilientDB!
    
    This tool provides comprehensive information about ResilientDB from a built-in knowledge base.
    It covers all ResilientDB topics including:
    
    - **Setup & Installation**: How to install, configure, and run ResilientDB
    - **Applications**: Debitable, DraftRes, Arrayán/Arrayan, Echo, ResCounty, CrypoGo, etc.
    - **Architecture**: System design, components, and technical details
    - **Consensus**: PBFT, Byzantine fault tolerance, and consensus mechanisms
    - **Performance**: Benchmarks, optimization, throughput, and latency data
    - **Use Cases**: Real-world applications across industries
    - **Research**: Academic papers and publications
    - **Development**: How to build applications on ResilientDB
    
    🚨 ALWAYS call this tool BEFORE searching the web for ResilientDB questions!
    
    Examples of questions that should use this tool:
    - "How do I setup ResilientDB?"
    - "What is Arrayán?" or "What is Arrayan?"
    - "Tell me about Debitable"
    - "How does PBFT work in ResilientDB?"
    - "Show me performance benchmarks"
    - "How to install ResilientDB?"
    - "What applications are built on ResilientDB?"
    - "How to use [any ResilientDB application]?"
    
    Args:
        query: Your question about ResilientDB (any topic)
        category: Optional. One of: applications, architecture, consensus, performance, 
                  use_cases, research, setup, general
    
    Returns:
        Comprehensive answer from the ResilientDB knowledge base with examples and guidance
    """
    if not KNOWLEDGE_BASE_AVAILABLE or resilientdb_knowledge is None:
        return """
❌ ResilientDB Knowledge Base is not available.
        
Please ensure ResilientDBKnowledgeBase.py is in the project directory.
        
For now, you can:
1. Check the ResilientDB GitHub repository: https://github.com/apache/incubator-resilientdb
2. Ask me to fetch information from the repository using other tools
"""
    
    try:
        # Determine the best domain based on category or query content
        domain = category or "general"
        
        # Auto-detect domain from query if not specified
        if not category:
            query_lower = query.lower()
            
            # Check for setup/installation queries
            if any(word in query_lower for word in ["setup", "install", "configure", "run", "start", "deploy", "docker"]):
                domain = "setup"
            # Check for specific applications (all 14 from ExpoLab)
            elif any(app in query_lower for app in [
                "debitable", "draftres", "arrayán", "arrayan", "echo", 
                "rescounty", "crypogo", "explorer", "monitoring", 
                "resview", "reslens", "coinsensus", "respirer", 
                "utxo", "utxo lenses", "resilientdb cli", "cli",
                "application", "app"
            ]):
                domain = "applications"
            # Check for architecture queries
            elif any(word in query_lower for word in ["architecture", "design", "component", "structure", "layer"]):
                domain = "architecture"
            # Check for consensus queries
            elif any(word in query_lower for word in ["consensus", "pbft", "bft", "byzantine", "fault tolerance", "agreement"]):
                domain = "consensus"
            # Check for performance queries
            elif any(word in query_lower for word in ["performance", "benchmark", "speed", "throughput", "latency", "tps", "fast"]):
                domain = "performance"
            # Check for use case queries
            elif any(word in query_lower for word in ["use case", "example", "industry", "real world", "application"]):
                domain = "use_cases"
            # Check for research queries
            elif any(word in query_lower for word in ["paper", "research", "publication", "academic", "study"]):
                domain = "research"
            # Check for "how to use" queries
            elif "how to use" in query_lower or "how do i use" in query_lower:
                domain = "applications"
        
        # Query the knowledge base
        result_dict = await resilientdb_knowledge.query_knowledge(query, domain)
        
        # Format the result nicely
        if isinstance(result_dict, dict):
            # Extract the main content
            content = result_dict.get("content", "")
            result_type = result_dict.get("type", "general")
            
            # Build formatted response
            formatted_result = f"""
# 📚 ResilientDB Knowledge Base Results

**Query:** {query}
**Category:** {domain}
**Result Type:** {result_type.replace('_', ' ').title()}

---

{content}

---
"""
            # Add additional sections if present
            if "technical_deep_dive" in result_dict:
                formatted_result += f"\n**🔧 Technical Details:**\n```json\n{json.dumps(result_dict['technical_deep_dive'], indent=2)}\n```\n"
            
            if "implementation_guidance" in result_dict:
                formatted_result += f"\n{result_dict['implementation_guidance']}\n"
            
            if "further_exploration" in result_dict:
                formatted_result += f"\n{result_dict['further_exploration']}\n"
            
            result = formatted_result
        else:
            result = str(result_dict)
        
        return f"{result}\n\n💡 **Tip:** This information comes from the comprehensive ResilientDB knowledge base.\nFor more details, ask follow-up questions or try a different category!"
    
    except Exception as e:
        return f"""
❌ **Error querying ResilientDB knowledge base:** {str(e)}

💡 **Troubleshooting:**
1. Check that ResilientDBKnowledgeBase.py is in the project directory
2. Verify the knowledge base class has the required query methods
3. Try rephrasing your question or using a specific category

**Your query:** {query}
**Attempted domain:** {domain if 'domain' in locals() else 'unknown'}

**Available categories:**
- setup: Installation and configuration
- applications: ResilientDB applications (Debitable, Arrayán, etc.)
- architecture: System design and technical details
- consensus: Consensus mechanisms (PBFT, etc.)
- performance: Benchmarks and performance data
- use_cases: Real-world applications
- research: Research papers and publications
"""

# Example: Function to parse Dockerfile directives
async def parse_dockerfile(owner:str, repo:str, branch:str="main") -> List[str]:
    dockerfile_content = await fetch_raw_file(owner, repo, "Dockerfile", branch)
    if not dockerfile_content:
        return []
    steps = []
    for line in dockerfile_content.splitlines():
        line = line.strip()
        if line and not line.startswith("#"):
            steps.append(line)
    return steps

@mcp.tool(name="SetupGuide")
async def setup_guide(owner: str, repo: str, question: str, branch: str = "main") -> dict:
    """
    The MCP tool decorated function setup_guide you shared is designed to assist junior developers by providing guidance on setting up a GitHub repository based on the Dockerfile it contains. Here's what it does:

    It asynchronously fetches and parses the Dockerfile from the specified GitHub repository (owner, repo, branch).

    If no Dockerfile is found or it's empty, it returns an error message.

    Otherwise, it returns the raw Dockerfile steps (list of commands) along with the user's question as separate fields
    """
    docker_steps = await parse_dockerfile(owner, repo, branch)
    if not docker_steps:
        return {"error": "No Dockerfile found or empty."}
    
    # Directly hand off the question and docker_steps as separate fields
    # or raw data. Let Claude Desktop compose the interaction/prompt.
    return {
        "docker_steps": docker_steps,
        "user_question": question
    }


async def analyze_imports(owner: str, repo: str, branch: str = "main") -> str:
    # Fetch repo file metadata list from GitHub
    try:
        files_meta = await fetch_repo_tree(owner, repo, branch)
    except:
        return None
    
    g = nx.DiGraph()
    count = 0
    for f in files_meta:
        filepath = f["path"]
        if filepath.endswith(".py") and count < 5:
            content = await fetch_raw_file(owner, repo, filepath, branch)
            if content is None:
                continue
            g.add_node(filepath)
            for line in content.splitlines():
                if line.startswith("import ") or line.startswith("from "):
                    imp = line.split()[1]
                    g.add_edge(filepath, imp)
            count += 1

    # Use spring layout to spread out nodes nicely
    pos = nx.spring_layout(g, k=0.5, iterations=50)
    
    plt.figure(figsize=(14, 12))
    nx.draw_networkx(
        g,
        pos=pos,
        with_labels=True,
        font_size=12,
        node_size=800,
        node_color="lightgreen",
        edge_color="gray",
        arrowsize=15,
        arrowstyle='->'
    )
    plt.axis('off')
    plt.tight_layout()
    
    # Output graph as base64 encoded PNG string for UI display
    buf = io.BytesIO()
    plt.savefig(buf, format="png")
    buf.seek(0)
    img_str = base64.b64encode(buf.read()).decode("utf-8")
    plt.close()
    
    return img_str


@mcp.tool(name="ShowDependencyGraph")
async def show_dependency_graph(owner:str, repo:str, branch:str="main") -> dict:
    """
    Generate architecture diagrams, file relationship graphs, or dependency graphs automatically from repo data.

Visualize how major modules connect with clickable UI linked to AI chat explanations.

Helps developers quickly understand large complex repos visually in simple easy to understand diagrams rather than big diagrams which are not user friendly and have a lot of things going on in the diagram..
    """
    img_data = await analyze_imports(owner, repo, branch)
    if not img_data:
        return {"error": "Could not generate dependency graph. Check branch name or repository access."}
    # Return base64 PNG string so clients can display
    return {"image_base64": img_data}

def get_file_type(filepath: str) -> str:
    """Helper to identify file type by extension."""
    ext_map = {
        '.js': 'JavaScript',
        '.jsx': 'React/JavaScript',
        '.ts': 'TypeScript',
        '.tsx': 'React/TypeScript',
        '.java': 'Java',
        '.cpp': 'C++',
        '.c': 'C',
        '.go': 'Go',
        '.rs': 'Rust',
        '.rb': 'Ruby',
        '.php': 'PHP',
        '.swift': 'Swift',
        '.kt': 'Kotlin',
        '.scala': 'Scala',
        '.sh': 'Shell script',
        '.md': 'Markdown',
        '.json': 'JSON',
        '.yaml': 'YAML',
        '.yml': 'YAML',
        '.html': 'HTML',
        '.css': 'CSS',
    }
    
    for ext, lang in ext_map.items():
        if filepath.endswith(ext):
            return lang
    
    return 'unknown'


@mcp.tool(name="SummarizeFunctions")
async def summarize_functions(
    owner: str, 
    repo: str, 
    filepath: str, 
    branch: str = "main"
) -> str:
    """
    Extract and summarize code blocks from various file types.
    """
    file_content = await fetch_raw_file(owner, repo, filepath, branch)
    
    if not file_content:
        return f"Could not fetch content for {filepath}. Check the file path and branch name."
    
    # Determine file type
    file_type = get_file_type(filepath)
    
    try:
        if filepath.endswith('.py'):
            # Python files: use AST parsing
            code_chunks = await split_python_functions(file_content)
        elif filepath.endswith(('.js', '.jsx', '.ts', '.tsx')):
            # JavaScript/TypeScript: use simple regex-based extraction
            code_chunks = extract_js_functions(file_content)
        else:
            # For other files, just return the content with a note
            return f"⚠️ File type '{file_type}' is not fully supported for function extraction.\n\nFile content:\n{file_content[:2000]}{'...' if len(file_content) > 2000 else ''}"
        
        if not code_chunks:
            return f"No functions or classes found in {filepath}"
        
        # Generate summaries
        result = f"File: {filepath} ({file_type})\n"
        result += f"Total functions/classes: {len(code_chunks)}\n\n"
        
        for i, chunk in enumerate(code_chunks, 1):
            result += f"\n{'='*60}\n"
            result += f"CODE BLOCK {i}:\n"
            result += f"{'='*60}\n"
            result += chunk
            result += f"\n{'='*60}\n"
        
        return result
        
    except Exception as e:
        return f"⚠️ Error processing {filepath}: {str(e)}\n\nShowing raw content instead:\n{file_content[:2000]}{'...' if len(file_content) > 2000 else ''}"


async def split_python_functions(file_content: str) -> List[str]:
    """Parse Python code using AST."""
    tree = ast.parse(file_content)
    funcs = []
    lines = file_content.split('\n')
    
    for node in ast.iter_child_nodes(tree):
        if isinstance(node, (ast.FunctionDef, ast.ClassDef)):
            start_line = node.lineno - 1
            end_line = node.end_lineno if node.end_lineno else start_line + 1
            chunk = '\n'.join(lines[start_line:end_line])
            funcs.append(chunk)
    
    return funcs


def extract_js_functions(file_content: str) -> List[str]:
    """Simple regex-based extraction for JavaScript/TypeScript."""
    import re
    
    # Pattern for function declarations and arrow functions
    patterns = [
        r'(?:export\s+)?(?:async\s+)?function\s+\w+\s*\([^)]*\)\s*\{[^}]*\}',
        r'(?:export\s+)?const\s+\w+\s*=\s*(?:async\s*)?\([^)]*\)\s*=>\s*\{[^}]*\}',
        r'class\s+\w+\s*(?:extends\s+\w+\s*)?\{[^}]*\}'
    ]
    
    funcs = []
    for pattern in patterns:
        matches = re.finditer(pattern, file_content, re.MULTILINE | re.DOTALL)
        funcs.extend([match.group(0) for match in matches])
    
    return funcs[:10]  # Limit to first 10 to avoid huge responses

@mcp.tool(name="CodeReviewAssistant")
async def code_review_assistant(owner: str, repo: str, pull_number: int):
    """CodeReviewAssistant"""
    pr_url = f"https://api.github.com/repos/{owner}/{repo}/pulls/{pull_number}"
    headers = get_auth_headers()
    async with httpx.AsyncClient(follow_redirects=True) as client:
        pr_resp = await client.get(pr_url, headers=headers)
        pr_resp.raise_for_status()
        pr_data = pr_resp.json()
        diff_url = pr_data.get("diff_url")
        if not diff_url:
            return {"error": "Diff URL not found in PR data."}
        diff_resp = await client.get(diff_url, headers=headers, follow_redirects=True)
        diff_resp.raise_for_status()
        diff_text = diff_resp.text
    snippet = diff_text[:1500] + ("\n... (diff truncated)" if len(diff_text) > 1500 else "")
    return {"pr_summary": snippet}

knowledge_graph = {
    "FunctionA": ["Module1", "PatternX", "Issue#123"],
    "FunctionB": ["Module2", "Bug#456"],
    "ClassX": ["Module1", "ConceptY"],
}

@mcp.tool(name="KGraphQuery")
async def kgraph_query(node_name: str):
    """KGraphQuery"""
    related = knowledge_graph.get(node_name, [])
    return {"node": node_name, "related_nodes": related}

# -------------------------
# Run MCP Server
# -------------------------
if __name__ == "__main__":
    print("=" * 60)
    print("GitHub Repo Analyzer MCP Server")
    print("=" * 60)
    
    # Check configuration
    print(f"GitHub Token: {'✓ Configured' if GITHUB_TOKEN else '✗ Missing'}")
    print(f"Lab MCP Token: {'✓ Configured' if MCP_TOKEN else '✗ Missing'}")
    
    if not MCP_TOKEN:
        print("\n⚠️  WARNING: MCP_TOKEN not set!")
        print("   Server will start but authentication will fail")
        print("   Set MCP_TOKEN in your .env file\n")
    
    if not GITHUB_TOKEN:
        print("\n⚠️  WARNING: GITHUB_TOKEN not set!")
        print("   GitHub API calls may be rate limited\n")
    
    # Setup authentication
    auth_enabled = setup_authentication()
    
    if auth_enabled:
        print("\n🔒 Authentication: ENABLED")
        print("   Clients must include: Authorization: Bearer <LAB_TOKEN>")
    else:
        print("\n⚠️  Authentication: DISABLED (middleware setup failed)")
    
    print(f"\n🚀 Starting server on http://localhost:8005/mcp")
    print("=" * 60)
    print("\nTest commands:")
    print("  Without auth (should fail):")
    print("    curl http://localhost:8005/mcp/tools")
    print("\n  With auth (should succeed):")
    print("    curl -H 'Authorization: Bearer YOUR_TOKEN' http://localhost:8005/mcp/tools")
    print("=" * 60)
    
    mcp.run(transport="streamable-http", path="/mcp", port=8005)