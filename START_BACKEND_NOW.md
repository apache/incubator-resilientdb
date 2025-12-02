# 🚀 Start GraphQ-LLM Backend Now

## Quick Start Command

```bash
cd /Users/sandhya/UCD/ECS_DDS/graphq-llm
chmod +x start-backend.sh
./start-backend.sh
```

---

## What It Does

The script will:
1. ✅ Check if backend is already running
2. ✅ Start ResilientDB (Docker)
3. ✅ Wait for ResilientDB to be ready
4. ✅ Start GraphQ-LLM Backend (Docker)
5. ✅ Verify backend is accessible

---

## Manual Start (If Script Doesn't Work)

```bash
cd /Users/sandhya/UCD/ECS_DDS/graphq-llm

# Start services
docker-compose -f docker-compose.dev.yml up -d resilientdb graphq-llm-backend

# Wait 60 seconds for services to start
sleep 60

# Verify backend
curl http://localhost:3001/health
```

---

## Verify Backend is Running

After starting, test:

```bash
# Health check
curl http://localhost:3001/health

# Should return: {"status":"ok","service":"graphq-llm-api"}
```

---

## Check Logs

If backend doesn't start:

```bash
# View backend logs
docker-compose -f docker-compose.dev.yml logs graphq-llm-backend

# View ResilientDB logs
docker-compose -f docker-compose.dev.yml logs resilientdb

# Check container status
docker-compose -f docker-compose.dev.yml ps
```

---

## After Backend Starts

Then run the test script again:

```bash
./test-query-stats-flow.sh
```

---

**Run `./start-backend.sh` now to start the backend!**

