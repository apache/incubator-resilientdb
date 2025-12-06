# 🚀 Complete Team Setup Guide - GraphQ-LLM with Nexus Integration

**Purpose:** This guide will help you replicate the exact working setup from scratch. Follow each step in order to reach the current state of the project.

---

## 🔗 Quick Reference - Repository Links

### **Repositories to Clone:**
1. **GraphQ-LLM** (this repository)
   - Replace `<repository-url>` with your actual GraphQ-LLM repository URL
   - Contains: Main application code, Docker configs, documentation

2. **Nexus** (separate repository - must be cloned)
   - **URL:** https://github.com/ResilientApp/nexus.git
   - **Purpose:** Next.js frontend application
   - **Note:** Requires modifications (see Step 7.6)

3. **ResilientDB** (reference only - using Docker image)
   - **URL:** https://github.com/apache/incubator-resilientdb
   - **Note:** We use the Docker image `expolab/resdb`, not cloning the repo

### **Key Documentation Files (in GraphQ-LLM repository):**
- `NEXUS_UI_EXTENSION_GUIDE.md` - Complete code for Nexus integration (Step 7.6)
- `TEST_QUERIES.md` - Example queries for testing
- `FINAL_PHASES_AND_STEPS.md` - Project status and progress
- `WHY_INTEGRATE_RESLENS_NEXUS.md` - Integration rationale
- `TEST_DOCKER_SERVICES.md` - Comprehensive testing guide for all Docker services

---

## 📋 Prerequisites Checklist

Before starting, ensure you have:

- [ ] **Docker** and **Docker Compose** installed
- [ ] **Git** installed
- [ ] **Node.js 18+** installed (for local development)
- [ ] **PostgreSQL** with pgvector (we'll set this up with Docker)

---

## 🔑 Required API Keys

Before starting, gather these API keys (some are optional but recommended):

### **Required:**
- **Gemini API Key** (for LLM - currently using `gemini-2.5-flash-lite`)
  - Get it from: https://makersuite.google.com/app/apikey
  - Create a new API key and copy it

### **Optional but Recommended:**
- **Hugging Face API Key** (for faster model downloads)
  - Get it from: https://huggingface.co/settings/tokens
  - Create a new token with read access

### **Optional (for Nexus features):**
- **DeepSeek API Key** - https://www.deepseek.com/ → Sign up → Get API key
- **LlamaCloud API Key** - https://cloud.llamaindex.ai/ → Sign up → Get API key
- **Supabase** (if using cloud PostgreSQL) - https://supabase.com → Create project

---

## 🎯 Step-by-Step Setup (Follow in Order)

### **Step 1: Clone GraphQ-LLM Repository**

```bash
# Clone the GraphQ-LLM repository
# Replace <repository-url> with your actual repository URL
git clone <repository-url>
cd graphq-llm
```

**Repository:** Replace `<repository-url>` with your GraphQ-LLM repository URL.

**Verify:** You should see files like `package.json`, `src/`, `docs/`, `docker-compose.dev.yml`, etc.

---

### **Step 2: Install Dependencies**

```bash
# Install all Node.js dependencies
npm install
```

**Expected output:** All packages will be installed, including:
- `@xenova/transformers` (for local LLM and embeddings)
- `@huggingface/inference` (for Hugging Face API)
- `@modelcontextprotocol/sdk` (for MCP server)
- All other dependencies

**Time:** 2-5 minutes

---

### **Step 3: Create Environment File**

Create a `.env` file in the `graphq-llm` directory:

```bash
cd /path/to/graphq-llm
touch .env
```

Copy this exact configuration into `.env`:

```env
# GraphQ-LLM Environment Variables

# ResilientDB Configuration
# For local development (ResilientDB running on host):
RESILIENTDB_GRAPHQL_URL=http://localhost:5001/graphql
# For Docker (ResilientDB in container):
# RESILIENTDB_GRAPHQL_URL=http://resilientdb:5001/graphql
# Note: GraphQL server runs on port 5001 (KV service is on port 18000)

# Nexus Configuration
NEXUS_API_URL=http://localhost:3000
NEXUS_API_KEY=

# ResLens Configuration (Live Mode - optional)
RESLENS_LIVE_MODE=false
RESLENS_POLL_INTERVAL=5000

# HTTP API Server Configuration (for GraphQ-LLM Backend)
# This is the HTTP API server that Nexus connects to (not the MCP server)
MCP_SERVER_PORT=3001
MCP_SERVER_HOST=localhost
# Note: MCP_SERVER_PORT is used by the HTTP API backend service
# The MCP server itself uses stdio transport and runs as a separate service

# LLM Configuration
# Using Gemini (recommended) or local LLM
LLM_PROVIDER=gemini
LLM_API_KEY=your_gemini_api_key_here
LLM_MODEL=gemini-2.5-flash-lite
LLM_ENABLE_LIVE_STATS=false

# Alternative: Using local LLM (no API key needed, but lower quality)
# LLM_PROVIDER=local
# LLM_API_KEY=
# LLM_MODEL=Xenova/gpt2

# Embedding Configuration
# Using local embeddings (Xenova/all-MiniLM-L6-v2) - no API key needed
EMBEDDINGS_PROVIDER=local
# Optional: Hugging Face API key for faster model downloads
HUGGINGFACE_API_KEY=your_huggingface_key_here

# Logging
LOG_LEVEL=info
```

**Important:** 
- Replace `your_gemini_api_key_here` with your actual Gemini API key (required for LLM)
- Replace `your_huggingface_key_here` with your actual Hugging Face API key (optional but recommended for faster model downloads)

---

### **Step 4: Set Up ResilientDB and GraphQL Server**

**What is the GraphQL Server?**
- ResilientDB has two services:
  1. **KV Service** (port 18000) - Key-value storage
  2. **GraphQL Server** (port 5001) - GraphQL API for querying/storing data
- The GraphQL server is **required** for storing and retrieving document chunks
- It's a Python Flask application that provides GraphQL mutations and queries

**⚠️ Important:** You do NOT need to clone the separate GraphQL repository (`https://github.com/apache/incubator-resilientdb-graphql`). The GraphQL server code is already included in the ResilientDB Docker image (`expolab/resdb`), and the `setup-graphql.sh` script automatically sets it up. The separate repository is only for reference or manual setup outside Docker.

#### **Option A: Using Docker (Recommended)**

```bash
# Start ResilientDB using Docker Compose
docker-compose -f docker-compose.dev.yml up -d resilientdb

# Wait for ResilientDB to be healthy (60-120 seconds)
# The setup-graphql.sh script runs automatically in the container
# Check status
docker-compose -f docker-compose.dev.yml ps

# Verify ResilientDB KV service is running
curl http://localhost:18000/v1/transactions/test

# Verify GraphQL server is running (IMPORTANT - this must work!)
curl -X POST http://localhost:5001/graphql \
  -H "Content-Type: application/json" \
  -d '{"query": "{ __typename }"}'
```

**Expected:** 
- KV service should return a response (may be an error, but connection works)
- GraphQL server should return `{"data":{"__typename":"Query"}}` or similar

**If GraphQL server is not running:**
```bash
# Check container logs
docker-compose -f docker-compose.dev.yml logs resilientdb

# The setup-graphql.sh script should run automatically
# If it fails, you may need to set up ResilientDB manually (see Option B)
```

#### **Option B: Manual Setup (If Docker doesn't work)**

If Docker setup fails or you want to set up manually:

1. **Set up ResilientDB KV Service:**
   - Follow ResilientDB documentation: https://github.com/apache/incubator-resilientdb
   - Ensure KV service is running on port 18000

2. **Set up GraphQL Server:**

   **Option B.1: Using ResilientDB Main Repository (Recommended)**
   ```bash
   # Clone ResilientDB main repository
   git clone https://github.com/apache/incubator-resilientdb.git
   cd incubator-resilientdb/ecosystem/graphql
   
   # Copy setup-graphql.sh from graphq-llm repository
   # Or manually install dependencies:
   pip install flask flask-cors ariadne base58 cryptography pysha3 \
       cryptoconditions python-rapidjson strawberry-graphql
   
   # Fix the tx_Dict typo (if present)
   sed -i 's/tx_Dict/tx_dict/g' resdb_driver/transaction.py
   
   # Start the GraphQL server
   python3 app.py
   # Server should start on port 5001
   ```

   **Option B.2: Using Standalone GraphQL Repository (Alternative)**
   ```bash
   # Clone the standalone GraphQL repository
   git clone https://github.com/apache/incubator-resilientdb-graphql.git
   cd incubator-resilientdb-graphql
   
   # Follow the repository's README for setup
   # Install dependencies as per repository instructions
   # Start the server
   python3 app.py
   ```
   
   **Note:** The standalone repository (Option B.2) is a separate implementation. The main ResilientDB repository (Option B.1) is recommended as it matches what's in the Docker image.

3. **Verify GraphQL Server:**
   ```bash
   curl -X POST http://localhost:5001/graphql \
     -H "Content-Type: application/json" \
     -d '{"query": "{ __typename }"}'
   ```

4. **Update `.env`** to point to your ResilientDB:
   ```env
   RESILIENTDB_GRAPHQL_URL=http://localhost:5001/graphql
   ```

#### **Option C: Use Existing ResilientDB**

If you already have ResilientDB with GraphQL server running:

1. **Comment out the `resilientdb` service** in `docker-compose.dev.yml`
2. **Update `.env`** to point to your existing ResilientDB:
   ```env
   RESILIENTDB_GRAPHQL_URL=http://localhost:5001/graphql
   ```
3. **Verify GraphQL server is accessible:**
   ```bash
   curl -X POST http://localhost:5001/graphql \
     -H "Content-Type: application/json" \
     -d '{"query": "{ __typename }"}'
   ```

---

### **Step 5: Start GraphQ-LLM Backend (HTTP API Server)**

The GraphQ-LLM backend runs as an HTTP API server that provides REST endpoints for Nexus integration. It runs separately from the MCP server.

#### **Option A: Using Docker (Recommended)**

```bash
# Start the HTTP API backend service
docker-compose -f docker-compose.dev.yml up -d graphq-llm-backend

# Check logs (wait for "Server listening" message)
docker-compose -f docker-compose.dev.yml logs -f graphq-llm-backend

# In another terminal, verify backend is running
curl http://localhost:3001/health
```

**Expected response:**
```json
{"status":"ok","service":"graphq-llm-api"}
```

**Note:** 
- First startup takes 30-60 seconds as it downloads the local LLM model (`Xenova/gpt2`)
- This service runs the HTTP API on port 3001 for Nexus integration
- The MCP server is a separate service (see Step 5.1)

#### **Option B: Using Local Node.js**

```bash
# Set environment variables
export LLM_PROVIDER=local
export LLM_MODEL=Xenova/gpt2
export EMBEDDINGS_PROVIDER=local
export HUGGINGFACE_API_KEY=your_key_here  # Optional

# Start HTTP API server
npm run http-api
```

The server will start on port 3001. Check logs for "Server listening" message.

---

### **Step 5.1: Start MCP Server (Optional - for MCP Client Integration)**

The MCP (Model Context Protocol) Server provides secure two-way communication between ResilientDB and AI tools. It uses stdio transport for communication with MCP clients and runs as a **separate Docker service** from the HTTP API backend.

**Important:** The MCP server and HTTP API backend are now separate services:
- **HTTP API Backend** (Step 5): Runs on port 3001, provides REST endpoints for Nexus
- **MCP Server** (Step 5.1): Uses stdio transport, runs as a separate container for MCP client integration

#### **Option A: Using Docker (Recommended)**

```bash
# Start the MCP server service (separate from HTTP API backend)
docker-compose -f docker-compose.dev.yml up -d graphq-llm-mcp-server

# Check logs
docker-compose -f docker-compose.dev.yml logs -f graphq-llm-mcp-server

# Check status of all services
docker-compose -f docker-compose.dev.yml ps
```

**Expected output:**
```
MCP Server started and listening on stdio
Live Stats Service: Enabled/Disabled
```

**Note:** 
- The MCP server uses stdio transport (stdin/stdout), so it doesn't expose HTTP ports
- It's designed to be spawned as a subprocess by MCP clients
- The container keeps stdin/stdout open for communication
- It shares the same environment variables as the backend service
- Both services can run simultaneously (they're independent)

#### **Option B: Using Local Node.js**

```bash
# Set environment variables (same as backend)
export RESILIENTDB_GRAPHQL_URL=http://localhost:5001/graphql
export LLM_PROVIDER=deepseek
export LLM_API_KEY=your_key_here

# Start MCP server
npm run mcp-server
```

**Note:** MCP servers using stdio transport are typically invoked by MCP clients as child processes. Running it standalone allows you to test it directly. In Docker, it runs as a separate container that can be started/stopped independently.

---

### **Step 5.2: Start ResLens Services (Optional - for Performance Monitoring)**

ResLens provides real-time performance monitoring and profiling for ResilientDB, including CPU and memory metrics, flame graphs, and call stack analysis. Both ResLens Middleware and Frontend are now dockerized and can be started with Docker Compose.

**What is ResLens?**
- **ResLens Middleware**: Node.js Express API server that aggregates performance data from ResilientDB
- **ResLens Frontend**: React/Vite web application for visualizing performance metrics
- **Purpose**: Monitor and optimize ResilientDB performance with real-time metrics

**Architecture:**
- **ResLens Middleware** (`ResLens-Middleware/middleware/Dockerfile`):
  - Single-stage production build using Node.js 18 Alpine
  - Production dependencies only
  - Go installed for pprof tool (if needed)
  - Health check endpoint on port 3003
- **ResLens Frontend** (`ResLens/Dockerfile`):
  - Multi-stage build: Node.js 20 Alpine → Build Vite app → Nginx Alpine
  - Static files served via Nginx with SPA routing
  - Gzip compression and static asset caching enabled
  - Container port 80 mapped to host port 5173

#### **Option A: Using Docker (Recommended)**

```bash
# Start ResLens Middleware (required for Frontend)
docker-compose -f docker-compose.dev.yml up -d reslens-middleware

# Wait for middleware to be healthy (30-60 seconds)
sleep 30

# Start ResLens Frontend (depends on Middleware)
docker-compose -f docker-compose.dev.yml up -d reslens-frontend

# Check status of all services
docker-compose -f docker-compose.dev.yml ps

# Check logs
docker-compose -f docker-compose.dev.yml logs -f reslens-middleware reslens-frontend
```

**Expected output:**
- ResLens Middleware: Running on port 3003
- ResLens Frontend: Running on port 5173 (mapped from container port 80)

**Verify ResLens is running:**
```bash
# Test Middleware health endpoint
curl http://localhost:3003/api/v1/healthcheck

# Test Frontend (should return HTML)
curl http://localhost:5173

# Check service status
docker-compose -f docker-compose.dev.yml ps reslens-middleware reslens-frontend
```

**Expected responses:**
- Middleware: `{"status":"UP"}` or similar JSON response
- Frontend: HTML page content (should start with `<!DOCTYPE html>`)

**Access ResLens UI:**
- Open browser: `http://localhost:5173`
- You should see the ResLens dashboard for performance monitoring

#### **Option B: Build Images First (If Needed)**

If the images don't exist yet, build them:

```bash
# Build ResLens Middleware image
docker-compose -f docker-compose.dev.yml build reslens-middleware

# Build ResLens Frontend image
docker-compose -f docker-compose.dev.yml build reslens-frontend

# Then start services (Option A above)
docker-compose -f docker-compose.dev.yml up -d reslens-middleware reslens-frontend
```

**Note:** 
- First build may take 5-10 minutes as it installs dependencies
- ResLens Frontend depends on ResLens Middleware - start middleware first
- ResLens Middleware depends on ResilientDB - ensure ResilientDB is running (Step 4)
- ResLens services are optional but recommended for performance monitoring

#### **Configuration**

ResLens services use these default configurations in `docker-compose.dev.yml`:

- **ResLens Middleware:**
  - Port: `3003`
  - Connects to: `http://resilientdb:18000` (ResilientDB KV service)
  - Health check: `http://localhost:3003/api/v1/healthcheck`
  
  **Runtime Environment Variables** (can be set in docker-compose):
  - `PORT`: Server port (default: 3003)
  - `CPP_STATS_API_BASE_URL`: ResilientDB stats API (default: `http://resilientdb:18000`)
  - `CPP_TRANSACTIONS_API_BASE_URL`: ResilientDB transactions API (default: `http://resilientdb:18000`)
  - `PYROSCOPE_SERVER_URL`: Pyroscope server URL (optional)
  - `PROMETHEUS_URL`: Prometheus server URL (optional)
  - `NODE_EXPORTER_BASE_URL`: Node exporter URL (optional)
  - `PROCESS_EXPORTER_BASE_URL`: Process exporter URL (optional)
  - `EXPLORER_BASE_URL`: Explorer API URL (optional)

- **ResLens Frontend:**
  - Port: `5173` (host) → `80` (container)
  - Connects to: `http://reslens-middleware:3003/api/v1` (Middleware API)
  - Health check: `http://localhost:5173`
  
  **Build-time Environment Variables** (set during `docker build`):
  - `VITE_MIDDLEWARE_BASE_URL`: URL to middleware API (default: `http://reslens-middleware:3003/api/v1`)
  - `VITE_MIDDLEWARE_SECONDARY_BASE_URL`: Secondary middleware URL (optional)

**Networking:**
Both services are on the `graphq-llm-network` Docker network, allowing them to communicate with:
- ResilientDB (via service name `resilientdb`)
- GraphQ-LLM Backend (via service name `graphq-llm-backend`)
- Each other (via service names `reslens-middleware` and `reslens-frontend`)

**To customize:** Edit `docker-compose.dev.yml` environment variables for ResLens services.

#### **Troubleshooting ResLens**

**Issue: ResLens Middleware not starting**
```bash
# Check logs
docker-compose -f docker-compose.dev.yml logs reslens-middleware

# Verify ResilientDB is running
docker-compose -f docker-compose.dev.yml ps resilientdb

# Check if port 3003 is in use
lsof -i:3003
```

**Issue: ResLens Frontend can't connect to Middleware**
```bash
# Verify middleware is running
curl http://localhost:3003/api/v1/healthcheck

# Check network connectivity
docker-compose -f docker-compose.dev.yml exec reslens-frontend wget -O- http://reslens-middleware:3003/api/v1/healthcheck

# Check middleware logs
docker-compose -f docker-compose.dev.yml logs reslens-middleware
```

**Issue: Port conflicts (3003 or 5173 already in use)**
```bash
# Find process using port
lsof -i:3003
lsof -i:5173

# Option 1: Kill the process
kill -9 <PID>

# Option 2: Change ports in docker-compose.dev.yml
# Edit ports section:
#   ports:
#     - "3004:3003"  # Change host port
#     - "5174:80"    # Change host port
```

**Issue: Build failures**
```bash
# Clear build cache and rebuild
docker-compose -f docker-compose.dev.yml build --no-cache reslens-middleware reslens-frontend

# Check if package-lock.json exists (required for npm ci)
ls ResLens-Middleware/middleware/package-lock.json
ls ResLens/package-lock.json

# If missing, generate them (run from project root)
cd ResLens-Middleware/middleware && npm install && cd ../..
cd ResLens && npm install && cd ..
```

#### **Viewing Logs and Managing Services**

```bash
# View logs for both services
docker-compose -f docker-compose.dev.yml logs -f reslens-middleware reslens-frontend

# View logs for individual services
docker-compose -f docker-compose.dev.yml logs -f reslens-middleware
docker-compose -f docker-compose.dev.yml logs -f reslens-frontend

# Stop services
docker-compose -f docker-compose.dev.yml stop reslens-middleware reslens-frontend

# Remove services (keeps images)
docker-compose -f docker-compose.dev.yml rm reslens-middleware reslens-frontend

# Remove services and images
docker-compose -f docker-compose.dev.yml down reslens-middleware reslens-frontend
```

#### **Development vs Production**

**Development (Local):**
- Run services locally with `npm run dev` / `npm start` in the respective directories
- Hot reload enabled
- Easier debugging with direct file access

**Production (Docker):**
- Optimized builds
- Static file serving via Nginx for Frontend
- Production dependencies only
- Health checks enabled
- Better resource management

---

### **Step 6: Ingest Documentation (REQUIRED)**

**⚠️ IMPORTANT:** Each teammate must run this on their own setup! This stores docs in your local ResilientDB.

```bash
# If using Docker
docker-compose -f docker-compose.dev.yml exec graphq-llm-backend npm run ingest:graphql

# If using local Node.js
npm run ingest:graphql
```

**What this does:**
- Loads all GraphQL documentation from `docs/` directory
- Chunks documents into smaller pieces
- Generates embeddings (using local model or Hugging Face)
- Stores chunks in ResilientDB

**Expected output:**
```
📚 GraphQL-only Ingestion
✅ Loaded X docs from docs/graphql-official
✅ Loaded X docs from docs/graphql-spec
✅ Loaded X docs from docs/community-guides
🚀 Ingesting X GraphQL-specific documents...
📊 Progress: 100/100 chunks (100%)
✅ GraphQL ingestion complete!
Documents processed: X/X
Chunks stored: X
```

**Time:** 5-10 minutes (depending on system and embedding provider)

**Verify:** Check logs for "Chunks stored: X" (should be 100+ chunks).

---

### **Step 7: Set Up Nexus (Separate Repository)**

Nexus is a separate Next.js application that needs to be cloned and modified. Follow these steps:

**Repository:** [https://github.com/ResilientApp/nexus](https://github.com/ResilientApp/nexus.git)

#### **7.1: Clone Nexus Repository**

```bash
# Navigate to your workspace (outside graphq-llm)
# You can clone it in the same parent directory as graphq-llm
cd /path/to/workspace

# Clone Nexus repository
git clone https://github.com/ResilientApp/nexus.git
cd nexus
```

**Repository Link:** [https://github.com/ResilientApp/nexus](https://github.com/ResilientApp/nexus.git)

**Verify:** You should see a Next.js project structure with `package.json`, `src/`, `app/`, etc.

**Note:** You do NOT need to clone the ResilientDB GraphQL repository separately. The GraphQL server is included in the ResilientDB Docker image and is set up automatically via `setup-graphql.sh` script.

#### **7.2: Install Nexus Dependencies**

```bash
npm install
```

#### **7.3: Set Up PostgreSQL with pgvector**

**Using Docker (Recommended):**

```bash
# Start PostgreSQL with pgvector
docker run -d \
  --name nexus-postgres \
  -e POSTGRES_USER=nexus \
  -e POSTGRES_PASSWORD=nexus \
  -e POSTGRES_DB=nexus \
  -p 5432:5432 \
  pgvector/pgvector:pg14

# Wait for PostgreSQL to be ready (10-20 seconds)
sleep 15

# Verify PostgreSQL is running
docker ps | grep nexus-postgres
```

**Alternative: Using Supabase (Cloud)**

1. Go to https://supabase.com
2. Create a free account
3. Create a new project
4. Get your connection string from Project Settings → Database

#### **7.4: Configure Nexus Environment Variables**

Create a `.env` file in the `nexus` directory:

```bash
cd /path/to/nexus
touch .env
```

Add this configuration:

```env
# Nexus Environment Variables

# Database Configuration
# For Docker PostgreSQL:
DATABASE_URL=postgres://nexus:nexus@localhost:5432/nexus
# For Supabase:
# DATABASE_URL=postgres://postgres:[YOUR-PASSWORD]@[YOUR-PROJECT].supabase.co:5432/postgres

# Embedding Configuration
EMBEDDING_DIM=384  # For sentence-transformers/all-MiniLM-L6-v2

# API Keys (Optional but recommended for Nexus features)
# Get Gemini API key: https://makersuite.google.com/app/apikey
GEMINI_API_KEY=your_gemini_key_here
# Get DeepSeek API key: https://www.deepseek.com/
DEEPSEEK_API_KEY=your_deepseek_key_here
# Get LlamaCloud API key: https://cloud.llamaindex.ai/
LLAMA_CLOUD_API_KEY=your_llamacloud_key_here

# Supabase Configuration (if using Supabase for vector store)
SUPABASE_URL=https://your-project.supabase.co
SUPABASE_ANON_KEY=your_supabase_anon_key

# GraphQ-LLM Backend URL (REQUIRED for GraphQL Tutor)
GRAPHQ_LLM_API_URL=http://localhost:3001
# Or use NEXT_PUBLIC_ prefix for client-side access:
NEXT_PUBLIC_GRAPHQ_LLM_API_URL=http://localhost:3001
```

**How to get API keys:**
- **Gemini (Recommended):** https://makersuite.google.com/app/apikey → Create API key → Copy and paste
- **DeepSeek:** https://www.deepseek.com/ → Sign up → Get API key
- **LlamaCloud:** https://cloud.llamaindex.ai/ → Sign up → Get API key
- **Supabase:** From your Supabase project dashboard (if using cloud PostgreSQL)

**Note:** 
- Gemini API key is recommended for best LLM performance
- Other API keys are optional - Nexus will work without them, but some features may be limited

#### **7.5: Set Up Nexus Database Schema**

```bash
# If using Docker PostgreSQL, run migrations
# (Check Nexus repository for migration commands)
# Usually something like:
npm run db:migrate
# or
npx prisma migrate dev
```

#### **7.6: Add GraphQ-LLM Integration to Nexus (REQUIRED)**

**⚠️ IMPORTANT:** This step adds the GraphQL Tutor UI and API routes to Nexus. This is a critical integration step.

**📋 Reference Guide:** 
- **Location:** `NEXUS_UI_EXTENSION_GUIDE.md` in the GraphQ-LLM repository
- **Path:** `/path/to/graphq-llm/NEXUS_UI_EXTENSION_GUIDE.md`
- **Link:** See the file in your cloned GraphQ-LLM repository

**What you need to do:**

1. **Open the guide:**
   ```bash
   # From your workspace, open the guide
   cat /path/to/graphq-llm/NEXUS_UI_EXTENSION_GUIDE.md
   # Or open it in your editor
   ```

2. **Create the following files in Nexus:**

   **a. API Route** (create directory first):
   ```bash
   mkdir -p src/app/api/graphql-tutor/analyze
   ```
   - **File:** `src/app/api/graphql-tutor/analyze/route.ts`
   - **Code:** Copy from `NEXUS_UI_EXTENSION_GUIDE.md` Section 1

   **b. Main Page** (create directory first):
   ```bash
   mkdir -p src/app/graphql-tutor/components
   ```
   - **File:** `src/app/graphql-tutor/page.tsx`
   - **Code:** Copy from `NEXUS_UI_EXTENSION_GUIDE.md` Section 2

   **c. Components** (create in the components directory):
   - **File:** `src/app/graphql-tutor/components/tutor-panel.tsx`
     - **Code:** Copy from `NEXUS_UI_EXTENSION_GUIDE.md` Section 3
   - **File:** `src/app/graphql-tutor/components/explanation-panel.tsx`
     - **Code:** Copy from `NEXUS_UI_EXTENSION_GUIDE.md` Section 4
   - **File:** `src/app/graphql-tutor/components/optimization-panel.tsx`
     - **Code:** Copy from `NEXUS_UI_EXTENSION_GUIDE.md` Section 5
   - **File:** `src/app/graphql-tutor/components/efficiency-display.tsx`
     - **Code:** Copy from `NEXUS_UI_EXTENSION_GUIDE.md` Section 6

   **d. Update Home Page:**
   - **File:** `src/app/page.tsx` (modify existing file)
   - **Code:** Add navigation link from `NEXUS_UI_EXTENSION_GUIDE.md` Section 7

3. **Install Required Dependencies:**
   ```bash
   cd /path/to/nexus
   npm install lucide-react
   ```

**Step-by-Step Copy-Paste Method:**

1. Navigate to your GraphQ-LLM repository:
   ```bash
   cd /path/to/graphq-llm
   ```

2. Open `NEXUS_UI_EXTENSION_GUIDE.md` in your editor or view it:
   ```bash
   cat NEXUS_UI_EXTENSION_GUIDE.md
   # Or use: code NEXUS_UI_EXTENSION_GUIDE.md
   ```

3. For each section in the guide:
   - Copy the code block
   - Navigate to Nexus directory: `cd /path/to/nexus`
   - Create the file in the specified location
   - Paste the code
   - Save the file

4. Verify all files are created:
   ```bash
   cd /path/to/nexus
   ls -la src/app/api/graphql-tutor/analyze/route.ts
   ls -la src/app/graphql-tutor/page.tsx
   ls -la src/app/graphql-tutor/components/tutor-panel.tsx
   ls -la src/app/graphql-tutor/components/explanation-panel.tsx
   ls -la src/app/graphql-tutor/components/optimization-panel.tsx
   ls -la src/app/graphql-tutor/components/efficiency-display.tsx
   ```

**All files should exist after this step!**

#### **7.7: Start Nexus**

```bash
# Start Nexus development server
npm run dev
```

Nexus will start on `http://localhost:3000`.

**Verify Nexus is running:**
```bash
curl http://localhost:3000/api/research/documents
```

---

### **Step 8: Verify Everything Works**

#### **8.1: Test GraphQ-LLM Backend**

```bash
# Test health endpoint
curl http://localhost:3001/health

# Test explanation endpoint
curl -X POST http://localhost:3001/api/explanations/explain \
  -H "Content-Type: application/json" \
  -d '{"query":"{ getTransaction(id: \"123\") { asset } }"}'
```

**Expected:** Should return JSON with explanation including documentation context.

#### **8.2: Test Nexus Integration**

```bash
# Test Nexus API route that proxies to GraphQ-LLM
curl -X POST http://localhost:3000/api/graphql-tutor/analyze \
  -H "Content-Type: application/json" \
  -d '{"query":"{ getTransaction(id: \"123\") { asset } }"}'
```

**Expected:** Should return JSON with explanation, optimizations, and efficiency data.

#### **8.3: Test Nexus UI**

1. Open browser: `http://localhost:3000/graphql-tutor`
2. Enter a GraphQL query: `{ getTransaction(id: "123") { asset } }`
3. Click "Analyze Query"
4. You should see:
   - **Explanation tab:** Explanation with documentation context
   - **Optimization tab:** Optimization suggestions
   - **Efficiency tab:** Efficiency score and metrics

**Test with these queries:** See `TEST_QUERIES.md` for 10 example queries.

---

## ✅ Verification Checklist

After completing all steps, verify:

- [ ] GraphQ-LLM HTTP API responds: `curl http://localhost:3001/health`
- [ ] (Optional) GraphQ-LLM MCP Server is running: `docker-compose -f docker-compose.dev.yml ps graphq-llm-mcp-server`
- [ ] ResilientDB KV service is running: `curl http://localhost:18000/v1/transactions/test`
- [ ] **ResilientDB GraphQL server is running** (REQUIRED): `curl -X POST http://localhost:5001/graphql -H "Content-Type: application/json" -d '{"query":"{ __typename }"}'`
  - Should return: `{"data":{"__typename":"Query"}}` or similar
  - If this fails, document ingestion and RAG will not work!
- [ ] (Optional) ResLens Middleware is running: `curl http://localhost:3003/api/v1/healthcheck`
  - Should return: `{"status":"UP"}` or similar JSON response
- [ ] (Optional) ResLens Frontend is accessible: `curl http://localhost:5173` or open `http://localhost:5173` in browser
- [ ] Documents ingested: Check logs for "Chunks stored: X" (should be 100+)
- [ ] Nexus is running: `curl http://localhost:3000/api/research/documents`
- [ ] Nexus UI accessible: Open `http://localhost:3000/graphql-tutor`
- [ ] Explanation endpoint works: Test via Nexus UI or direct API call
- [ ] RAG retrieval works: Explanations include documentation context from ResilientDB

---

## 🔧 Current Configuration Summary

### **GraphQ-LLM Configuration:**
- **LLM Provider:** `gemini` (using `gemini-2.5-flash-lite`)
- **LLM API Key:** Required (get from https://makersuite.google.com/app/apikey)
- **Embeddings Provider:** `local` (using `Xenova/all-MiniLM-L6-v2`)
- **HTTP API Backend Port:** `3001` (for Nexus integration)
- **MCP Server:** Separate service using stdio transport (no HTTP port)
- **ResilientDB GraphQL URL:** `http://localhost:5001/graphql` (or `http://resilientdb:5001/graphql` in Docker)

### **Docker Services:**
- **ResilientDB:** Ports 18000 (KV service) and 5001 (GraphQL server)
- **GraphQ-LLM Backend:** Port 3001 (HTTP API for Nexus)
- **GraphQ-LLM MCP Server:** Separate service (stdio transport, no HTTP port)
- **ResLens Middleware:** Port 3003 (performance monitoring API) - Optional
- **ResLens Frontend:** Port 5173 (performance monitoring UI) - Optional

### **Nexus Configuration:**
- **Port:** `3000`
- **Database:** PostgreSQL with pgvector (Docker or Supabase)
- **GraphQ-LLM Backend:** `http://localhost:3001`

### **Integration:**
- GraphQ-LLM HTTP API: `http://localhost:3001` (for Nexus)
- GraphQ-LLM MCP Server: Separate service (for MCP client integration)
- Nexus API Route: `http://localhost:3000/api/graphql-tutor/analyze`
- Nexus proxies requests to GraphQ-LLM HTTP API backend

---

## 🐛 Troubleshooting

### **Issue: GraphQ-LLM HTTP API not starting**

**Check:**
```bash
# Check if port 3001 is in use
lsof -i :3001

# Check logs
docker-compose -f docker-compose.dev.yml logs graphq-llm-backend
# or if running locally
tail -f /tmp/graphq-llm-http.log
```

**Solution:**
- Kill process on port 3001: `pkill -f "npm run http-api"`
- Restart the service

### **Issue: Local LLM model not downloading**

**Check:**
```bash
# Check Hugging Face cache
ls -la ~/.cache/huggingface/

# Clear cache and retry
rm -rf ~/.cache/huggingface/
```

**Solution:**
- Ensure `HUGGINGFACE_API_KEY` is set in `.env` (optional but helps with downloads)
- Wait for model download (first time takes 5-10 minutes)
- Check internet connection

### **Issue: Nexus not connecting to GraphQ-LLM**

**Check:**
```bash
# Verify GraphQ-LLM is running
curl http://localhost:3001/health

# Check Nexus environment variables
cat /path/to/nexus/.env | grep GRAPHQ_LLM
```

**Solution:**
- Ensure `GRAPHQ_LLM_API_URL=http://localhost:3001` is set in Nexus `.env`
- Or set `NEXT_PUBLIC_GRAPHQ_LLM_API_URL=http://localhost:3001`
- Restart Nexus after changing `.env`

### **Issue: GraphQL Server not running**

**Check:**
```bash
# Verify GraphQL server is running
curl -X POST http://localhost:5001/graphql \
  -H "Content-Type: application/json" \
  -d '{"query": "{ __typename }"}'
```

**Solution:**
- If using Docker: Check container logs: `docker-compose -f docker-compose.dev.yml logs resilientdb`
- The `setup-graphql.sh` script should run automatically in Docker
- If it fails, check that Python dependencies are installed
- GraphQL server must be running on port 5001 for document ingestion to work

### **Issue: No documents retrieved in explanations**

**Check:**
```bash
# Verify GraphQL server is running first
curl -X POST http://localhost:5001/graphql \
  -H "Content-Type: application/json" \
  -d '{"query": "{ __typename }"}'

# Verify documents were ingested via GraphQL
curl -X POST http://localhost:5001/graphql \
  -H "Content-Type: application/json" \
  -d '{"query": "{ transactions(limit: 5) { id } }"}'
```

**Solution:**
- **First:** Ensure GraphQL server is running (see issue above)
- Re-run document ingestion: `npm run ingest:graphql`
- Check that `docs/` directory has GraphQL documentation files
- Verify ingestion completed successfully (check logs)
- If GraphQL server is not running, ingestion will fail

### **Issue: PostgreSQL connection failed (Nexus)**

**Check:**
```bash
# Verify PostgreSQL is running
docker ps | grep nexus-postgres

# Test connection
psql postgres://nexus:nexus@localhost:5432/nexus -c "SELECT 1"
```

**Solution:**
- Start PostgreSQL: `docker start nexus-postgres`
- Check `DATABASE_URL` in Nexus `.env` file
- Wait 15-20 seconds after starting PostgreSQL before starting Nexus

### **Issue: Nexus UI components not rendering**

**Check:**
- Verify all component files are created in correct locations
- Check browser console for errors
- Verify shadcn/ui components are installed

**Solution:**
- Ensure all files from `NEXUS_UI_EXTENSION_GUIDE.md` are created
- Install missing dependencies: `npm install lucide-react`
- Restart Nexus: `npm run dev`

---

## 🎯 Quick Start Commands (Copy-Paste)

For team members who want to get started quickly:

```bash
# 1. Clone and install GraphQ-LLM
# Replace <repo-url> with your actual GraphQ-LLM repository URL
git clone <repo-url> && cd graphq-llm && npm install

# 2. Create .env file (copy configuration from Step 3 above)
touch .env
# Edit .env with your configuration

# 3. Start ResilientDB
docker-compose -f docker-compose.dev.yml up -d resilientdb

# 4. Wait for ResilientDB (60-120 seconds)
sleep 60

# 5. Start GraphQ-LLM HTTP API backend
docker-compose -f docker-compose.dev.yml up -d graphq-llm-backend

# 5.1. (Optional) Start MCP Server for MCP client integration
docker-compose -f docker-compose.dev.yml up -d graphq-llm-mcp-server

# 5.2. (Optional) Start ResLens services for performance monitoring
docker-compose -f docker-compose.dev.yml up -d reslens-middleware
sleep 30  # Wait for middleware to be healthy
docker-compose -f docker-compose.dev.yml up -d reslens-frontend

# 6. Wait for backend (30-60 seconds)
sleep 30

# 7. Ingest documents (REQUIRED - each teammate must do this)
docker-compose -f docker-compose.dev.yml exec graphq-llm-backend npm run ingest:graphql

# 8. Set up Nexus (separate repository)
cd /path/to/workspace
git clone https://github.com/ResilientApp/nexus.git && cd nexus && npm install

# 9. Set up PostgreSQL for Nexus
docker run -d --name nexus-postgres \
  -e POSTGRES_USER=nexus -e POSTGRES_PASSWORD=nexus \
  -e POSTGRES_DB=nexus -p 5432:5432 pgvector/pgvector:pg14

# 10. Create Nexus .env file (copy configuration from Step 7.4 above)
touch .env
# Edit .env with your configuration

# 11. Add GraphQ-LLM integration to Nexus
# Follow Step 7.6 above - copy files from NEXUS_UI_EXTENSION_GUIDE.md
# Location: /path/to/graphq-llm/NEXUS_UI_EXTENSION_GUIDE.md

# 12. Start Nexus
npm run dev

# 13. Test integration
curl -X POST http://localhost:3000/api/graphql-tutor/analyze \
  -H "Content-Type: application/json" \
  -d '{"query":"{ getTransaction(id: \"123\") { asset } }"}'
```

---

## 📚 Additional Resources

### **Repositories:**
- **GraphQ-LLM Repository:** Replace `<repository-url>` with your actual repository URL
- **Nexus Repository:** [https://github.com/ResilientApp/nexus](https://github.com/ResilientApp/nexus.git)
- **ResilientDB Main Repository:** [https://github.com/apache/incubator-resilientdb](https://github.com/apache/incubator-resilientdb) (for reference - contains GraphQL server in `ecosystem/graphql/`)
- **ResilientDB GraphQL Repository (Standalone):** [https://github.com/apache/incubator-resilientdb-graphql](https://github.com/apache/incubator-resilientdb-graphql) (for reference only - not needed if using Docker)

### **Documentation Files (in GraphQ-LLM repository):**
- **Nexus UI Extension Guide:** `NEXUS_UI_EXTENSION_GUIDE.md` (complete code for all Nexus integration files)
- **Test Queries:** `TEST_QUERIES.md` (10 example queries to test the tutor)
- **Docker Services Testing:** `TEST_DOCKER_SERVICES.md` (comprehensive testing guide for all Docker services including ResLens)
- **Project Status:** `FINAL_PHASES_AND_STEPS.md` (overall project progress)
- **Integration Details:** `WHY_INTEGRATE_RESLENS_NEXUS.md` (why these integrations are needed)
- **ResilientDB Docs:** `docs/` directory (documentation ingested for RAG)

### **API Key Resources:**
- **Gemini API Key:** https://makersuite.google.com/app/apikey
- **Hugging Face Token:** https://huggingface.co/settings/tokens
- **DeepSeek API Key:** https://www.deepseek.com/
- **LlamaCloud API Key:** https://cloud.llamaindex.ai/
- **Supabase:** https://supabase.com

---

## 🆘 Need Help?

If you encounter issues:

1. **Check logs:**
   ```bash
   # GraphQ-LLM logs
   docker-compose -f docker-compose.dev.yml logs graphq-llm-backend
   
   # Nexus logs (in Nexus directory)
   npm run dev  # Check terminal output
   ```

2. **Verify environment variables:**
   - GraphQ-LLM `.env` file
   - Nexus `.env` file

3. **Check service health:**
   ```bash
   # GraphQ-LLM HTTP API Backend
   curl http://localhost:3001/health
   
   # GraphQ-LLM MCP Server (check container status)
   docker-compose -f docker-compose.dev.yml ps graphq-llm-mcp-server
   
   # Nexus
   curl http://localhost:3000/api/research/documents
   
   # ResilientDB KV Service
   curl http://localhost:18000/v1/transactions/test
   
   # ResilientDB GraphQL Server
   curl -X POST http://localhost:5001/graphql -H "Content-Type: application/json" -d '{"query":"{ __typename }"}'
   
   # ResLens Middleware (Optional)
   curl http://localhost:3003/api/v1/healthcheck
   
   # ResLens Frontend (Optional)
   curl http://localhost:5173
   ```

4. **Review troubleshooting section above**

---

## ✅ Success Criteria

You've successfully set up the project when:

1. ✅ GraphQ-LLM HTTP API is running on port 3001
2. ✅ (Optional) GraphQ-LLM MCP Server is running (for MCP client integration)
3. ✅ ResilientDB is running and accessible (KV service on 18000, GraphQL on 5001)
4. ✅ (Optional) ResLens services are running (Middleware on 3003, Frontend on 5173)
5. ✅ Documents are ingested (check logs for chunk count - should be 100+)
6. ✅ Nexus is running on port 3000
7. ✅ Nexus UI shows GraphQL Tutor page at `http://localhost:3000/graphql-tutor`
8. ✅ Query analysis works in Nexus UI
9. ✅ Explanations include documentation context from ResilientDB

**Congratulations! You're now at the same stage as the current setup! 🎉**

---

## 📝 Notes for Team Members

### **Critical Steps:**
- **Each teammate must run document ingestion separately** - docs are stored in your local ResilientDB
- **Nexus UI extensions must be added manually** - follow Step 7.6 and use `NEXUS_UI_EXTENSION_GUIDE.md`
- **Gemini API key is required** - get it from https://makersuite.google.com/app/apikey

### **Important Reminders:**
- **First startup takes longer** - models need to be downloaded (5-10 minutes)
- **Environment variables are critical** - double-check `.env` files in both GraphQ-LLM and Nexus
- **Test with provided queries** - see `TEST_QUERIES.md` for 10 example queries
- **Nexus must be cloned separately** - it's a different repository from GraphQ-LLM
- **All Nexus integration files must be created** - see Step 7.6 for complete list

### **What Gets Modified:**
- **GraphQ-LLM:** No modifications needed (just clone and configure)
- **Nexus:** Requires adding 6 new files (see Step 7.6):
  1. `src/app/api/graphql-tutor/analyze/route.ts`
  2. `src/app/graphql-tutor/page.tsx`
  3. `src/app/graphql-tutor/components/tutor-panel.tsx`
  4. `src/app/graphql-tutor/components/explanation-panel.tsx`
  5. `src/app/graphql-tutor/components/optimization-panel.tsx`
  6. `src/app/graphql-tutor/components/efficiency-display.tsx`
  7. Modify `src/app/page.tsx` (add navigation link)

---

**Ready to start? Begin with Step 1! 🚀**
