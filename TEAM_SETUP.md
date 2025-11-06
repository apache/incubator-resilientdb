# 🚀 Team Setup Guide - GraphQ-LLM

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
RESILIENTDB_GRAPHQL_URL=http://localhost:5001/graphql

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
- ✅ ResilientDB container starts
- ✅ GraphQL server automatically installs dependencies
- ✅ GraphQL server fixes Python 3.8 compatibility issues
- ✅ GraphQL server starts on port 5001
- ✅ Backend service starts with live code reload
- ✅ All services are networked together

**Wait for services to be healthy** (about 30-60 seconds):
```bash
docker-compose -f docker-compose.dev.yml ps
```

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

### **Q: How do I verify everything is working?**
```bash
# Check service health
docker-compose -f docker-compose.dev.yml ps

# Check logs
docker-compose -f docker-compose.dev.yml logs graphq-llm-backend
docker-compose -f docker-compose.dev.yml logs resilientdb

# Test the backend health endpoint
curl http://localhost:3001/health
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

### **Issue: GraphQL server not starting**
```bash
# Check ResilientDB logs
docker-compose -f docker-compose.dev.yml logs resilientdb

# Check if GraphQL server is running inside container
docker-compose -f docker-compose.dev.yml exec resilientdb ps aux | grep python
```

### **Issue: Backend can't connect to ResilientDB**
- ✅ Check `RESILIENTDB_GRAPHQL_URL` in `.env`
- ✅ Verify ResilientDB container is healthy: `docker-compose ps`
- ✅ Check network connectivity: `docker network ls`

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

## 🎯 Next Steps After Setup

1. **Ingest Documentation:**
   ```bash
   docker-compose -f docker-compose.dev.yml exec graphq-llm-backend npm run ingest
   ```

2. **Test RAG System:**
   ```bash
   docker-compose -f docker-compose.dev.yml exec graphq-llm-backend npm run test-retrieval
   ```

3. **Test LLM Integration:**
   ```bash
   docker-compose -f docker-compose.dev.yml exec graphq-llm-backend npm run test-rag-llm
   ```

4. **Verify Storage:**
   ```bash
   docker-compose -f docker-compose.dev.yml exec graphq-llm-backend npm run verify-storage
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

