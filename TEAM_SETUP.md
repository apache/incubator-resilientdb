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
- Docker and Docker Compose installed
- Git (to clone the repository)
- Node.js 18+ (optional, for local development)

### Step 1: Clone the Repository
```bash
git clone <repository-url>
cd graphq-llm
```

### Step 2: Set Up Environment Variables
Create a `.env` file in the project root:
```bash
# Copy example if available, or create manually
cp .env.example .env  # if exists
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
- ✅ GraphQL server automatically installs dependencies
- ✅ GraphQL server fixes Python 3.8 compatibility issues
- ✅ GraphQL server starts on port 5001
- ✅ Health check confirms ResilientDB is ready
- ✅ **Then** backend service starts (waits for ResilientDB to be healthy)
- ✅ Backend connects to ResilientDB using service name `resilientdb`
- ✅ All services are networked together via Docker's internal network

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

2. **Document Ingestion (First Time Only)**
   ```bash
   # After services are running, ingest documentation
   docker-compose -f docker-compose.dev.yml exec graphq-llm-backend npm run ingest
   ```
   This stores 129 chunks of ResilientDB documentation in the vector database.

---

## ❓ Common Questions

### **Q: Do I need to clone ResilientDB GraphQL separately?**
**A: NO!** ✅
- ResilientDB GraphQL is already included in the ResilientDB Docker image
- The `setup-graphql.sh` script automatically sets it up
- Location: `/app/ecosystem/graphql` inside the container
- **You don't need to do anything** - it's all automated!

### **Q: What if I already have ResilientDB running?**
**A:** You can skip the `resilientdb` service in docker-compose:
- Comment out the `resilientdb` service in `docker-compose.dev.yml`
- Update `RESILIENTDB_GRAPHQL_URL` in `.env` to point to your existing instance

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
```bash
# After services are running
docker-compose -f docker-compose.dev.yml exec graphq-llm-backend npm run ingest
```

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
docker-compose -f docker-compose.dev.yml exec graphq-llm-backend npm run ingest
```

**Expected Output:**
- ✅ Documents loaded from `docs/` directory
- ✅ Chunks created
- ✅ Embeddings generated
- ✅ Chunks stored in ResilientDB
- ✅ Success message with chunk count

---

## 🎯 Next Steps After Setup

1. **Ingest Documentation:**
   ```bash
   docker-compose -f docker-compose.dev.yml exec graphq-llm-backend npm run ingest
   ```
   This stores ResilientDB documentation in the vector database for RAG.

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

---

**Happy Coding! 🚀**

