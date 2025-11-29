# 🧪 Testing Dockerized Services - Communication Verification

This guide helps you verify that all dockerized services are running and communicating correctly.

---

## 📊 Current Service Architecture

### **Dockerized Services (docker-compose.dev.yml):**
1. ✅ **ResilientDB** - Database with GraphQL server
   - Container: `resilientdb`
   - Ports: `18000` (KV service), `5001` (GraphQL server)
   - Network: `graphq-llm-network`

2. ✅ **GraphQ-LLM Backend** - HTTP API server for Nexus
   - Container: `graphq-llm-backend`
   - Ports: `3001` (HTTP API), `9229` (Debug)
   - Network: `graphq-llm-network`
   - Connects to: ResilientDB via `http://resilientdb:5001/graphql`

3. ✅ **GraphQ-LLM MCP Server** - Model Context Protocol server
   - Container: `graphq-llm-mcp-server`
   - Transport: stdio (no HTTP port)
   - Network: `graphq-llm-network`
   - Connects to: ResilientDB via `http://resilientdb:5001/graphql`

### **External Services (Not Dockerized in dev):**
4. ⚠️ **Nexus** - Next.js frontend application
   - Runs: Locally with `npm run dev` (port 3000)
   - Connects to: GraphQ-LLM Backend via `http://localhost:3001`

**Note:** Nexus is NOT dockerized in `docker-compose.dev.yml` but runs separately. It connects to the dockerized backend via exposed ports.

---

## ✅ Step-by-Step Verification

### **Step 1: Check All Docker Services Are Running**

```bash
# Check status of all dockerized services
docker-compose -f docker-compose.dev.yml ps
```

**Expected Output:**
```
NAME                    STATUS
resilientdb             Up X minutes (healthy)
graphq-llm-backend      Up X minutes (healthy)
graphq-llm-mcp-server   Up X minutes
```

**✅ Success Criteria:**
- All three services show "Up" status
- ResilientDB shows "healthy"
- GraphQ-LLM Backend shows "healthy"
- MCP Server shows "Up" (may show "unhealthy" - that's OK, it uses stdio)

---

### **Step 2: Test ResilientDB Services**

#### **2.1: Test ResilientDB KV Service (Port 18000)**

```bash
# Test KV service from host
curl http://localhost:18000/v1/transactions/test
```

**Expected:** Any response (even error) means service is running

#### **2.2: Test ResilientDB GraphQL Server (Port 5001)**

```bash
# Test GraphQL server from host
curl -X POST http://localhost:5001/graphql \
  -H "Content-Type: application/json" \
  -d '{"query": "{ __typename }"}'
```

**Expected Response:**
```json
{"data":{"__typename":"Query"}}
```

**✅ Success Criteria:** Returns GraphQL response (not connection error)

#### **2.3: Test ResilientDB from Inside Docker Network**

```bash
# Test from backend container (internal network communication)
docker-compose -f docker-compose.dev.yml exec graphq-llm-backend \
  curl -X POST http://resilientdb:5001/graphql \
    -H "Content-Type: application/json" \
    -d '{"query": "{ __typename }"}'
```

**✅ Success Criteria:** Same GraphQL response - proves internal Docker network works

---

### **Step 3: Test GraphQ-LLM Backend (HTTP API)**

#### **3.1: Test Health Endpoint**

```bash
# Test health endpoint
curl http://localhost:3001/health
```

**Expected Response:**
```json
{"status":"ok","service":"graphq-llm-api"}
```

**✅ Success Criteria:** Returns health status

#### **3.2: Test Backend Connection to ResilientDB**

```bash
# Test that backend can query ResilientDB
curl -X POST http://localhost:3001/api/explanations/explain \
  -H "Content-Type: application/json" \
  -d '{"query": "{ getTransaction(id: \"test\") { id } }"}'
```

**Expected:** Returns explanation with documentation context

**✅ Success Criteria:** 
- Returns 200 OK
- Response includes explanation text
- No connection errors to ResilientDB

#### **3.3: Test Backend Logs for ResilientDB Connection**

```bash
# Check backend logs for connection status
docker-compose -f docker-compose.dev.yml logs graphq-llm-backend | grep -i resilientdb
```

**✅ Success Criteria:** Should show successful connections or health checks

---

### **Step 4: Test MCP Server**

#### **4.1: Check MCP Server Container Status**

```bash
# Check MCP server is running
docker-compose -f docker-compose.dev.yml ps graphq-llm-mcp-server
```

**✅ Success Criteria:** Container shows "Up" status

#### **4.2: Check MCP Server Logs**

```bash
# Check MCP server logs
docker-compose -f docker-compose.dev.yml logs graphq-llm-mcp-server | tail -20
```

**Expected Output:**
```
MCP Server started and listening on stdio
Live Stats Service: Enabled/Disabled
```

**✅ Success Criteria:** Shows "MCP Server started and listening on stdio"

#### **4.3: Test MCP Server Connection to ResilientDB (Internal)**

```bash
# Test from MCP server container that it can reach ResilientDB
docker-compose -f docker-compose.dev.yml exec graphq-llm-mcp-server \
  curl -X POST http://resilientdb:5001/graphql \
    -H "Content-Type: application/json" \
    -d '{"query": "{ __typename }"}'
```

**✅ Success Criteria:** Returns GraphQL response - proves MCP server can communicate with ResilientDB

**Note:** MCP server uses stdio transport, so it can't be tested via HTTP directly. It's designed to be spawned as a subprocess by MCP clients.

---

### **Step 5: Test Nexus Connection to GraphQ-LLM Backend**

#### **5.1: Verify Nexus Is Running**

```bash
# Check if Nexus is running (should be running locally, not in Docker)
curl http://localhost:3000/api/research/documents
```

**✅ Success Criteria:** Returns response (even if empty array)

#### **5.2: Test Nexus API Route That Calls GraphQ-LLM Backend**

```bash
# Test Nexus API route that proxies to GraphQ-LLM
curl -X POST http://localhost:3000/api/graphql-tutor/analyze \
  -H "Content-Type: application/json" \
  -d '{"query": "{ getTransaction(id: \"test\") { id asset } }"}'
```

**Expected Response:**
```json
{
  "explanation": "...",
  "optimizations": [...],
  "efficiency": {...}
}
```

**✅ Success Criteria:**
- Returns 200 OK
- Response includes explanation, optimizations, and efficiency data
- No errors connecting to GraphQ-LLM backend

#### **5.3: Verify Nexus Environment Configuration**

Check that Nexus has the correct backend URL configured:

```bash
# If Nexus .env file exists, check configuration
grep GRAPHQ_LLM_API_URL /path/to/nexus/.env
```

**Expected:**
```
GRAPHQ_LLM_API_URL=http://localhost:3001
# or
NEXT_PUBLIC_GRAPHQ_LLM_API_URL=http://localhost:3001
```

**✅ Success Criteria:** Points to `http://localhost:3001` (the dockerized backend)

---

### **Step 6: End-to-End Test - Full Communication Chain**

#### **6.1: Test Complete Flow**

1. **Nexus UI** → `http://localhost:3000/graphql-tutor`
2. **Nexus API** → `http://localhost:3001` (GraphQ-LLM Backend)
3. **GraphQ-LLM Backend** → `http://resilientdb:5001/graphql` (ResilientDB)
4. **ResilientDB** → Returns data

```bash
# Simulate the full chain
# Step 1: Nexus calls backend
curl -X POST http://localhost:3000/api/graphql-tutor/analyze \
  -H "Content-Type: application/json" \
  -d '{"query": "{ getTransaction(id: \"123\") { id asset } }"}' \
  | jq '.'
```

**✅ Success Criteria:**
- Request succeeds through all layers
- Response includes all expected data
- No connection timeouts or errors

---

## 🔍 Network Verification

### **Check Docker Network**

```bash
# Inspect the Docker network
docker network inspect graphq-llm-network
```

**Expected:** Should show all three containers connected:
- `resilientdb`
- `graphq-llm-backend`
- `graphq-llm-mcp-server`

**✅ Success Criteria:** All containers are connected to the same network

---

## 📋 Quick Verification Checklist

Run all these commands to verify everything:

```bash
# 1. Check all services are running
docker-compose -f docker-compose.dev.yml ps

# 2. Test ResilientDB GraphQL
curl -X POST http://localhost:5001/graphql \
  -H "Content-Type: application/json" \
  -d '{"query": "{ __typename }"}'

# 3. Test GraphQ-LLM Backend health
curl http://localhost:3001/health

# 4. Test GraphQ-LLM Backend query explanation
curl -X POST http://localhost:3001/api/explanations/explain \
  -H "Content-Type: application/json" \
  -d '{"query": "{ __typename }"}'

# 5. Check MCP server logs
docker-compose -f docker-compose.dev.yml logs graphq-llm-mcp-server | tail -5

# 6. Test Nexus backend connection (if Nexus is running)
curl -X POST http://localhost:3000/api/graphql-tutor/analyze \
  -H "Content-Type: application/json" \
  -d '{"query": "{ __typename }"}'

# 7. Check Docker network
docker network inspect graphq-llm-network | grep -A 5 "Containers"
```

---

## ⚠️ Common Issues

### **Issue 1: MCP Server Shows "unhealthy"**

**Status:** ✅ **This is NORMAL**
- MCP server uses stdio transport (not HTTP)
- No HTTP health endpoint exists
- Check logs instead: `docker-compose -f docker-compose.dev.yml logs graphq-llm-mcp-server`

### **Issue 2: Nexus Can't Connect to Backend**

**Check:**
1. Is GraphQ-LLM backend running? `curl http://localhost:3001/health`
2. Is Nexus `.env` configured correctly? Should have `GRAPHQ_LLM_API_URL=http://localhost:3001`
3. Is Nexus running? `curl http://localhost:3000`

### **Issue 3: Backend Can't Connect to ResilientDB**

**Check:**
1. Is ResilientDB running? `docker-compose -f docker-compose.dev.yml ps resilientdb`
2. Test GraphQL server: `curl -X POST http://localhost:5001/graphql ...`
3. Check backend logs: `docker-compose -f docker-compose.dev.yml logs graphq-llm-backend`

---

## 🎯 Success Summary

All services are communicating correctly when:

- ✅ ResilientDB responds to GraphQL queries
- ✅ GraphQ-LLM Backend health check passes
- ✅ GraphQ-LLM Backend can query ResilientDB
- ✅ MCP Server is running (logs show "started")
- ✅ MCP Server can reach ResilientDB (internal network)
- ✅ Nexus can call GraphQ-LLM Backend API
- ✅ All containers are on the same Docker network

---

**Ready to test? Start with Step 1! 🚀**

