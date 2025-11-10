# 🚀 Team Setup Guide - GraphQ-LLM

## ⚡ Quick Verification (After Setup)

**Run this to check if everything works:**
```bash
# Quick check - all in one command
./verify-setup.sh

# Or manually check:
docker-compose -f docker-compose.dev.yml ps
curl http://localhost:3001/health
curl http://localhost:5001/graphql -X POST -H "Content-Type: application/json" -d '{"query":"{ __typename }"}'
```

**See full verification steps below** → [Verification & Testing](#-verification--testing)

---

## Quick Start (5 Minutes)

### Prerequisites
- **ResilientDB set up and running** (see "ResilientDB Setup" section below)
- Docker and Docker Compose installed
- Git (to clone the repository)
- Node.js 18+ (optional, for local development)

### Step 0: Set Up ResilientDB (Required First)
**⚠️ IMPORTANT:** You need ResilientDB running before starting this project.

**Option A: Set up ResilientDB separately (Recommended)**
1. Follow ResilientDB setup instructions from official docs
2. Start ResilientDB KV service on port 18000
3. Verify it's running: `curl http://localhost:18000/v1/transactions/test`
4. Then proceed to Step 1 below

**Option B: Try Docker setup (May not work)**
- The `expolab/resdb` image should start KV service, but we're overriding the command
- Try Step 3 below, then verify KV service is running
- If port 18000 doesn't respond, you'll need to set up ResilientDB separately (Option A)

### Step 1: Clone the Repository
```bash
git clone <repository-url>
cd graphq-llm
```

### Step 2: Set Up Environment Variables
Create a `.env` file in the project root:
```bash
# Copy example 
cp .env.example .env  
```

Required environment variables:
```env
# ResilientDB Configuration
# Use 'resilientdb' hostname when running in Docker, 'localhost' for local development
RESILIENTDB_GRAPHQL_URL=http://resilientdb:5001/graphql

# LLM Configuration (choose one provider)
LLM_PROVIDER=openai  # or anthropic, deepseek, huggingface
LLM_API_KEY=your-api-key-here
LLM_MODEL=gpt-3.5-turbo  # or model of your choice

# Hugging Face (for embeddings - optional but recommended)
HUGGINGFACE_API_KEY=your-huggingface-key-here  # Optional for higher rate limits

# ResLens (optional - for Live Mode)
RESLENS_API_URL=http://localhost:8080/api  # Optional
RESLENS_API_KEY=your-reslens-key  # Optional
RESLENS_LIVE_MODE=false  # Set to true when ResLens middleware is set up
```

### Step 3: Start Docker Services
```bash
docker-compose -f docker-compose.dev.yml up -d
```

That's it! 🎉

**What happens automatically:**
- ✅ ResilientDB container starts **first**
- ✅ **ResilientDB KV service starts automatically** (from `expolab/resdb` image)
  - KV service runs on port 18000
  - This is the core database service
- ✅ GraphQL server setup runs (`setup-graphql.sh`)
  - Installs Python dependencies
  - Fixes Python 3.8 compatibility issues
  - Starts GraphQL server on port 5001
- ✅ Health check confirms ResilientDB KV service is ready (port 18000)
- ✅ **Then** backend service starts (waits for ResilientDB to be healthy)
- ✅ Backend connects to ResilientDB GraphQL using service name `resilientdb:5001`
- ✅ All services are networked together via Docker's internal network

**Important:** The `expolab/resdb` Docker image already includes:
- ResilientDB KV service (the database itself)
- ResilientDB GraphQL code (we just set it up and start it)
- No separate ResilientDB installation needed!

**Wait for services to be healthy** (about 60-120 seconds for first run):
```bash
# Check service status
docker-compose -f docker-compose.dev.yml ps

# Watch logs in real-time
docker-compose -f docker-compose.dev.yml logs -f resilientdb
```

**⚠️ Important:** First startup takes longer (60-120 seconds) because:
- Docker image needs to be pulled (if not cached)
- Python dependencies need to be installed
- GraphQL server needs to start
- Health checks need to pass

**If ResilientDB doesn't start:**
1. Check logs: `docker-compose -f docker-compose.dev.yml logs resilientdb`
2. Verify ports aren't in use: `lsof -i :18000 -i :5001`
3. Try restarting: `docker-compose -f docker-compose.dev.yml restart resilientdb`

---

## 🔍 What's Automated vs Manual

### ✅ **Fully Automated (No Action Needed):**

1. **ResilientDB GraphQL Setup**
   - ✅ Python dependencies installation
   - ✅ Python 3.8 compatibility fixes
   - ✅ GraphQL server startup
   - ✅ Port configuration (5001)
   
2. **Docker Networking**
   - ✅ All services connected automatically
   - ✅ Service discovery configured

3. **Code Reloading**
   - ✅ Live updates when you edit code
   - ✅ No need to restart containers

### 📋 **Manual Steps (One-Time Setup):**

1. **Environment Variables**
   - ☐ Create `.env` file
   - ☐ Add API keys (LLM, optional: Hugging Face, ResLens)

2. **Document Ingestion (REQUIRED - Each teammate must do this)**
   ```bash
   # After ResilientDB and backend are running, ingest documentation
   docker-compose -f docker-compose.dev.yml exec graphq-llm-backend npm run ingest
   ```
   
   **⚠️ IMPORTANT:** Each teammate must run this on their own setup!
   - Your teammate's ingestion stores docs in **their** local ResilientDB
   - Your ingestion stored docs in **your** local ResilientDB
   - Each person has their own separate ResilientDB instance
   - This is a **one-time setup** per teammate (takes 2-5 minutes)
   
   **What it does:**
   - Loads all files from `docs/` directory
   - Loads GraphQL schema via introspection
   - Chunks documents into smaller pieces
   - Generates embeddings (Hugging Face)
   - Stores ~100-130 chunks in **your local ResilientDB**
   
   **Expected output:**
   ```
   📚 Document Ingestion for RAG
   ✅ Loaded X documents
   ✅ GraphQL schema loaded
   📊 Progress: 100/100 chunks (100%)
   ✅ Ingestion completed successfully!
   📦 129 document chunks are now available for RAG
   ```

---

## ❓ Common Questions

### **Q: Do I need to set up ResilientDB separately first?**
**A: ⚠️ RECOMMENDATION - Set up ResilientDB separately for reliability:**

**The Problem:**
- The `expolab/resdb` Docker image **should** include ResilientDB KV service
- **BUT:** We're overriding the container's command to run `setup-graphql.sh`
- This **might prevent** the KV service from starting automatically
- The healthcheck tests port 18000, but if it fails, KV service isn't running

**Recommended Approach (Most Reliable):**
1. **Set up ResilientDB separately first** (outside Docker Compose)
   - Follow ResilientDB setup instructions
   - Start ResilientDB KV service on port 18000
   - Verify it's running: `curl http://localhost:18000/v1/transactions/test`
2. **Then use this project's Docker setup** for GraphQL and backend
   - Comment out the `resilientdb` service in `docker-compose.dev.yml`
   - Update `.env` to point to your existing ResilientDB: `RESILIENTDB_GRAPHQL_URL=http://localhost:5001/graphql`
   - Run `docker-compose up` for backend and GraphQL setup only

**Alternative (If you want everything in Docker):**
- Try the current setup first
- Check if KV service starts: `curl http://localhost:18000/v1/transactions/test`
- If it doesn't work, you'll need to set up ResilientDB separately anyway

**How to Verify KV Service is Running:**
```bash
# Test if port 18000 is responding (KV service)
curl http://localhost:18000/v1/transactions/test

# Check what processes are running in the container
docker-compose -f docker-compose.dev.yml exec resilientdb ps aux | grep -i resdb

# Check the image's default entrypoint
docker inspect expolab/resdb:amd64 | grep -A 10 "Entrypoint\|Cmd"
```

### **Q: Do I need to set up ResilientDB separately?**
**A: It depends - we need to verify the image behavior:**
- ✅ **ResilientDB KV service** - Already included in `expolab/resdb` Docker image
  - Starts automatically when container starts
  - Runs on port 18000
  - No manual setup needed
- ✅ **ResilientDB GraphQL** - Also included in the image
  - Located at `/app/ecosystem/graphql` inside the container
  - The `setup-graphql.sh` script sets it up (installs Python deps, fixes compatibility)
  - Runs on port 5001

**What happens:**
1. Container starts → **ResilientDB KV service should start automatically** (from `expolab/resdb` image's default entrypoint)
   - ⚠️ **If KV service doesn't start**, the image's entrypoint may need to be checked
   - The healthcheck tests port 18000 to confirm KV service is running
2. `setup-graphql.sh` runs → Sets up GraphQL server on top of KV service
3. Both services run together in the same container

**⚠️ Important Note:**
- The `expolab/resdb` Docker image should include and start the ResilientDB KV service automatically
- If the healthcheck fails (port 18000 not responding), the KV service may not be starting
- Check the image documentation or logs to verify the KV service starts with the image
- If needed, we may need to explicitly start the KV service in the docker-compose command

**You don't need to:**
- ❌ Clone ResilientDB separately
- ❌ Install ResilientDB manually
- ❌ Start KV service manually
- ✅ Just run `docker-compose up` - everything is automated!

### **Q: What if I already have ResilientDB running?**
**A: ✅ RECOMMENDED APPROACH - Use your existing ResilientDB:**

**Steps:**
1. **Comment out the `resilientdb` service** in `docker-compose.dev.yml`:
   ```yaml
   # resilientdb:
   #   image: expolab/resdb:${ARCH:-amd64}
   #   ... (comment out entire service)
   ```

2. **Update `.env` file:**
   ```env
   # Use localhost since ResilientDB is running on your host machine
   RESILIENTDB_GRAPHQL_URL=http://localhost:5001/graphql
   ```

3. **Start only the backend service:**
   ```bash
   docker-compose -f docker-compose.dev.yml up -d graphq-llm-backend
   ```

**Benefits:**
- ✅ More reliable (you control ResilientDB setup)
- ✅ Avoids Docker networking issues
- ✅ Easier to debug (ResilientDB logs separate)
- ✅ Can use existing ResilientDB installation

### **Q: Do I need Docker credentials?**
**A: NO!** ✅
- All Docker images are public
- No authentication required
- Just run `docker-compose up`

### **Q: What ports are used?**
- **18000:** ResilientDB KV service
- **5001:** ResilientDB GraphQL server
- **3001:** GraphQ-LLM backend API
- **9229:** Node.js debug port (optional)

### **Q: How does `http://resilientdb:5001/graphql` work?**
**A:** Docker Compose automatically creates a network where services can find each other by name:
- ✅ `resilientdb` is the service name in `docker-compose.dev.yml`
- ✅ Docker's internal DNS resolves `resilientdb` to the container's IP
- ✅ The backend waits for ResilientDB to be healthy before starting (see `depends_on` in docker-compose)
- ✅ This only works **inside** the Docker network, not from your host machine
- ✅ From your host machine, use `http://localhost:5001/graphql`

**Why not `localhost`?**
- Inside Docker containers, `localhost` refers to the container itself, not other containers
- Service names like `resilientdb` are resolved by Docker's DNS to the correct container IP
- This is how containers communicate with each other in Docker Compose

### **Q: How do I verify everything is working?**
```bash
# Check service health
docker-compose -f docker-compose.dev.yml ps

# Check logs
docker-compose -f docker-compose.dev.yml logs graphq-llm-backend
docker-compose -f docker-compose.dev.yml logs resilientdb

# Test the backend health endpoint (from your host machine)
curl http://localhost:3001/health

# Test ResilientDB GraphQL (from your host machine)
curl http://localhost:5001/graphql
```

### **Q: How do I ingest documents?**
**A: Each teammate must run this on their own setup:**

```bash
# After ResilientDB and backend services are running
docker-compose -f docker-compose.dev.yml exec graphq-llm-backend npm run ingest
```

**Why each person needs to do this:**
- ✅ Each teammate has their own local ResilientDB instance
- ✅ Documents are stored in **your local ResilientDB**, not shared
- ✅ This is a one-time setup (takes 2-5 minutes)
- ✅ After ingestion, you'll have ~100-130 chunks stored locally

**What gets stored:**
- All files from `docs/` directory (GraphQL setup, KV service, HTTP server, etc.)
- GraphQL schema information (via introspection)
- Each chunk includes: text, embedding vector, metadata
- Stored via GraphQL mutation: `postTransaction`

**If ingestion fails:**
- Check ResilientDB is running: `curl http://localhost:18000/v1/transactions/test`
- Check GraphQL is accessible: `curl http://localhost:5001/graphql`
- Check logs: `docker-compose -f docker-compose.dev.yml logs graphq-llm-backend`

### **Q: Can I develop locally without Docker?**
**A:** Yes, but you'll need:
- ResilientDB running locally (or use Docker just for ResilientDB)
- Node.js 18+ installed
- Run `npm install` and `npm run dev`

---

## 🛠️ Troubleshooting

### **Issue: ResilientDB container not starting**
```bash
# Check ResilientDB logs for errors
docker-compose -f docker-compose.dev.yml logs resilientdb

# Check if container is running
docker-compose -f docker-compose.dev.yml ps resilientdb

# Check if GraphQL server is running inside container
docker-compose -f docker-compose.dev.yml exec resilientdb ps aux | grep python

# Check GraphQL server logs
docker-compose -f docker-compose.dev.yml exec resilientdb tail -f /tmp/graphql.log

# If container fails to start, check Docker logs
docker logs resilientdb
```

**Common Issues:**
- **ResilientDB KV service not starting:** Port 18000 not responding
  - **This is the most common issue!** The `expolab/resdb` image should start KV service automatically
  - Check logs: `docker-compose -f docker-compose.dev.yml logs resilientdb`
  - Verify image entrypoint: `docker inspect expolab/resdb:amd64 | grep -A 5 Entrypoint`
  - If KV service doesn't start, we may need to explicitly start it in docker-compose
  - Solution: Check if image has a default command/entrypoint that starts KV service
- **Port conflict:** Port 18000 or 5001 already in use
  - Solution: Stop conflicting services or change ports in `docker-compose.dev.yml`
- **Permission issues:** Script not executable
  - Solution: `chmod +x setup-graphql.sh` before starting
- **GraphQL directory missing:** `/app/ecosystem/graphql` doesn't exist
  - Solution: Check if ResilientDB image includes GraphQL (should be in `expolab/resdb` image)
- **Python dependencies fail:** Installation errors
  - Solution: Check logs for specific package errors, may need to update `setup-graphql.sh`

### **Issue: Backend can't connect to ResilientDB**
- ✅ **IMPORTANT:** In Docker, use `http://resilientdb:5001/graphql` (service name), NOT `http://localhost:5001/graphql`
- ✅ Check `RESILIENTDB_GRAPHQL_URL` in `.env` - should be `http://resilientdb:5001/graphql` for Docker
- ✅ Verify ResilientDB container is healthy: `docker-compose -f docker-compose.dev.yml ps`
- ✅ Check network connectivity: `docker network ls` and verify `graphq-llm-network` exists
- ✅ Test connection from backend container:
  ```bash
  docker-compose -f docker-compose.dev.yml exec graphq-llm-backend curl http://resilientdb:5001/graphql
  ```

### **Issue: Port already in use**
- Change ports in `docker-compose.dev.yml`
- Or stop conflicting services

### **Issue: Code changes not reflecting**
- ✅ Ensure volume mounts are correct in `docker-compose.dev.yml`
- ✅ Check if `npm run dev` is running (should auto-reload)
- ✅ Restart backend: `docker-compose restart graphq-llm-backend`

---

## 📦 What's Included

### **Services:**
1. **ResilientDB** (`resilientdb`)
   - KV service (port 18000)
   - GraphQL server (port 5001) - auto-configured
   - Persistent data volume

2. **GraphQ-LLM Backend** (`graphq-llm-backend`)
   - Node.js/TypeScript application
   - MCP server (port 3001)
   - Live code reloading
   - Health checks

### **Optional (Commented Out):**
- **ResLens Middleware** - Uncomment when ready for Live Mode

---

## ✅ Verification & Testing

### Step 1: Check Container Status
```bash
# Check if all containers are running and healthy
docker-compose -f docker-compose.dev.yml ps
```

**Expected Output:**
- `resilientdb` - Status: `Up (healthy)`
- `graphq-llm-backend` - Status: `Up (healthy)` or `Up`

### Step 2: Test ResilientDB KV Service
```bash
# Test from your host machine
curl http://localhost:18000/v1/transactions/test

# Or test from inside the container
docker-compose -f docker-compose.dev.yml exec resilientdb curl http://localhost:18000/v1/transactions/test
```

**Expected:** Should return a response (may be an error, but connection should work)

### Step 3: Test ResilientDB GraphQL Server
```bash
# Test GraphQL endpoint from your host machine
curl -X POST http://localhost:5001/graphql \
  -H "Content-Type: application/json" \
  -d '{"query": "{ __typename }"}'

# Or test introspection
curl -X POST http://localhost:5001/graphql \
  -H "Content-Type: application/json" \
  -d '{"query": "query IntrospectionQuery { __schema { queryType { name } } }"}'
```

**Expected:** Should return JSON with GraphQL schema information

### Step 4: Test Backend Health Endpoint
```bash
# Test backend health check
curl http://localhost:3001/health
```

**Expected:** Should return JSON with health status:
```json
{
  "status": "healthy",
  "services": {
    "resilientdb": true,
    "embedding": true,
    "vectorStore": true
  }
}
```

### Step 5: Test Backend → ResilientDB Connection
```bash
# Test if backend can reach ResilientDB from inside the container
docker-compose -f docker-compose.dev.yml exec graphq-llm-backend curl http://resilientdb:5001/graphql

# Or test with a GraphQL query
docker-compose -f docker-compose.dev.yml exec graphq-llm-backend curl -X POST http://resilientdb:5001/graphql \
  -H "Content-Type: application/json" \
  -d '{"query": "{ __typename }"}'
```

**Expected:** Should return GraphQL response (proves Docker networking works)

### Step 6: Check Logs for Errors
```bash
# Check ResilientDB logs
docker-compose -f docker-compose.dev.yml logs resilientdb | tail -50

# Check backend logs
docker-compose -f docker-compose.dev.yml logs graphq-llm-backend | tail -50

# Watch logs in real-time
docker-compose -f docker-compose.dev.yml logs -f
```

**What to look for:**
- ✅ No error messages
- ✅ "GraphQL server started" in ResilientDB logs
- ✅ "Server started" or similar in backend logs
- ✅ No connection refused errors

### Step 7: Test Document Ingestion (Full System Test)
```bash
# Ingest documentation (this tests the full pipeline)
# ⚠️ IMPORTANT: Each teammate must run this on their own setup
docker-compose -f docker-compose.dev.yml exec graphq-llm-backend npm run ingest
```

**Expected Output:**
- ✅ Documents loaded from `docs/` directory (6-10 files)
- ✅ GraphQL schema loaded via introspection
- ✅ Chunks created (~100-130 chunks total)
- ✅ Embeddings generated (Hugging Face, 384 dimensions each)
- ✅ Chunks stored in **your local ResilientDB** via GraphQL
- ✅ Success message: "📦 X document chunks are now available for RAG"

**What gets stored:**
- Each chunk is stored as a transaction in ResilientDB
- Contains: text content, embedding vector, source file, metadata
- Stored via: GraphQL `postTransaction` mutation (primary) or HTTP API (fallback)
- Location: Your local ResilientDB instance (not shared with teammates)

---

## 🎯 Next Steps After Setup

1. **Ingest Documentation (REQUIRED - Each teammate):**
   ```bash
   docker-compose -f docker-compose.dev.yml exec graphq-llm-backend npm run ingest
   ```
   
   **⚠️ IMPORTANT:** Each teammate must run this separately!
   - Stores docs in **your local ResilientDB** (not shared)
   - Takes 2-5 minutes (one-time setup)
   - Stores ~100-130 chunks from `docs/` directory + GraphQL schema
   - After this, RAG system can retrieve relevant documentation
   
   **What happens:**
   1. Loads all `.md` files from `docs/` directory
   2. Loads GraphQL schema via introspection
   3. Chunks documents (max 512 tokens per chunk)
   4. Generates embeddings using Hugging Face
   5. Stores each chunk in ResilientDB via GraphQL mutation
   
   **Verify it worked:**
   - Should see "✅ Ingestion completed successfully!"
   - Should see chunk count (e.g., "📦 129 document chunks")
   - If errors occur, check ResilientDB and GraphQL are running

2. **Verify Services are Running:**
   ```bash
   # Check all services
   docker-compose -f docker-compose.dev.yml ps
   
   # Check ResilientDB logs
   docker-compose -f docker-compose.dev.yml logs resilientdb
   
   # Check backend logs
   docker-compose -f docker-compose.dev.yml logs graphq-llm-backend
   ```

---

## 💡 Benefits of This Setup

✅ **Consistent Environment:** Everyone gets the same setup  
✅ **No Manual Configuration:** GraphQL setup is automated  
✅ **Live Updates:** Code changes reflect immediately  
✅ **Isolated:** Each developer has their own containers  
✅ **Easy Cleanup:** `docker-compose down` removes everything  
✅ **Team Collaboration:** Shared Docker configuration  

---

## 📚 Additional Resources

- **Project Status:** See `FINAL_PHASES_AND_STEPS.md`
- **Integration Details:** See `WHY_INTEGRATE_RESLENS_NEXUS.md`
- **ResilientDB Docs:** See `docs/` directory (ingested for RAG)

---

## 🆘 Need Help?

If you encounter issues:
1. Check the troubleshooting section above
2. Review logs: `docker-compose logs <service-name>`
3. Check service health: `docker-compose ps`
4. Verify environment variables are set correctly


