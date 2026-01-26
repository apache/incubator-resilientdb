<!--
  ~ Licensed to the Apache Software Foundation (ASF) under one
  ~ or more contributor license agreements.  See the NOTICE file
  ~ distributed with this work for additional information
  ~ regarding copyright ownership.  The ASF licenses this file
  ~ to you under the Apache License, Version 2.0 (the
  ~ "License"); you may not use this file except in compliance
  ~ with the License.  You may obtain a copy of the License at
  ~
  ~   http://www.apache.org/licenses/LICENSE-2.0
  ~
  ~ Unless required by applicable law or agreed to in writing,
  ~ software distributed under the License is distributed on an
  ~ "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
  ~ KIND, either express or implied.  See the License for the
  ~ specific language governing permissions and limitations
  ~ under the License.
-->

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

4. ✅ **ResLens Middleware** - Performance monitoring API server (Optional)
   - Container: `reslens-middleware`
   - Ports: `3003` (HTTP API)
   - Network: `graphq-llm-network`
   - Connects to: ResilientDB via `http://resilientdb:18000` (KV service)

5. ✅ **ResLens Frontend** - Performance monitoring UI (Optional)
   - Container: `reslens-frontend`
   - Ports: `5173` (host) → `80` (container)
   - Network: `graphq-llm-network`
   - Connects to: ResLens Middleware via `http://reslens-middleware:3003/api/v1`

### **External Services (Not Dockerized in dev):**
6. ⚠️ **Nexus** - Next.js frontend application
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
reslens-middleware      Up X minutes (healthy/unhealthy)
reslens-frontend        Up X minutes (healthy/unhealthy)
```

**✅ Success Criteria:**
- All core services (1-3) show "Up" status
- ResilientDB shows "healthy"
- GraphQ-LLM Backend shows "healthy"
- MCP Server shows "Up" (may show "unhealthy" - that's OK, it uses stdio)
- ResLens services (4-5) show "Up" (optional - may show "unhealthy" but still working)

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

### **Step 5: Test ResLens Services (Optional - Performance Monitoring)**

#### **5.1: Test ResLens Middleware**

```bash
# Test ResLens Middleware health endpoint from host
curl http://localhost:3003/api/v1/healthcheck
```

**Expected Response:**
```json
{"status":"UP"}
```

**✅ Success Criteria:** Returns health status JSON

#### **5.2: Test ResLens Middleware Connection to ResilientDB**

```bash
# Test that middleware can reach ResilientDB from inside Docker network
docker-compose -f docker-compose.dev.yml exec reslens-middleware \
  curl http://resilientdb:18000/v1/transactions/test
```

**✅ Success Criteria:** Returns any response (even error) - proves middleware can reach ResilientDB

#### **5.3: Test ResLens Frontend**

```bash
# Test ResLens Frontend from host
curl http://localhost:5173
```

**Expected Response:** HTML page content (should start with `<!DOCTYPE html>`)

**✅ Success Criteria:** Returns HTML page

#### **5.4: Test ResLens Frontend Connection to Middleware**

```bash
# Test that frontend container can reach middleware
docker-compose -f docker-compose.dev.yml exec reslens-frontend \
  wget -O- http://reslens-middleware:3003/api/v1/healthcheck
```

**Expected Response:**
```json
{"status":"UP"}
```

**✅ Success Criteria:** Returns middleware health status - proves frontend can communicate with middleware

#### **5.5: Check ResLens Service Logs**

```bash
# Check ResLens Middleware logs
docker-compose -f docker-compose.dev.yml logs reslens-middleware | tail -20

# Check ResLens Frontend logs
docker-compose -f docker-compose.dev.yml logs reslens-frontend | tail -20
```

**✅ Success Criteria:**
- Middleware logs show server started successfully
- Frontend logs show Nginx serving static files

#### **5.6: Access ResLens UI**

Open in browser: `http://localhost:5173`

**✅ Success Criteria:**
- Page loads successfully
- Dashboard displays (may show offline mode if profiling services aren't configured)
- No connection errors in browser console

**Note:** ResLens services are optional. If they show "unhealthy" but still respond to requests, that's acceptable. Health checks may fail due to timing or configuration, but the services can still function.

---

### **Step 6: Test Nexus Connection to GraphQ-LLM Backend**

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

### **Step 7: End-to-End Test - Full Communication Chain**

#### **7.1: Test Complete Flow (GraphQ-LLM)**

1. **Nexus UI** → `http://localhost:3000/graphql-tutor`
2. **Nexus API** → `http://localhost:3001` (GraphQ-LLM Backend)
3. **GraphQ-LLM Backend** → `http://resilientdb:5001/graphql` (ResilientDB)
4. **ResilientDB** → Returns data

#### **7.2: Test Complete Flow (ResLens)**

1. **ResLens Frontend** → `http://localhost:5173`
2. **ResLens Frontend** → `http://reslens-middleware:3003/api/v1` (Middleware API)
3. **ResLens Middleware** → `http://resilientdb:18000` (ResilientDB KV service)
4. **ResilientDB** → Returns performance metrics

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

**Expected:** Should show all containers connected:
- `resilientdb`
- `graphq-llm-backend`
- `graphq-llm-mcp-server`
- `reslens-middleware` (if running)
- `reslens-frontend` (if running)

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

# 6. Test ResLens Middleware (if running)
curl http://localhost:3003/api/v1/healthcheck

# 7. Test ResLens Frontend (if running)
curl http://localhost:5173

# 8. Test Nexus backend connection (if Nexus is running)
curl -X POST http://localhost:3000/api/graphql-tutor/analyze \
  -H "Content-Type: application/json" \
  -d '{"query": "{ __typename }"}'

# 9. Check Docker network
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

### **Issue 4: ResLens Services Show "unhealthy"**

**Status:** ⚠️ **May be normal**
- ResLens health checks may fail due to timing or configuration
- Services can still function even if health check shows "unhealthy"
- Check if services respond to requests:
  ```bash
  curl http://localhost:3003/api/v1/healthcheck  # Middleware
  curl http://localhost:5173                      # Frontend
  ```
- If services respond, they're working despite health check status

### **Issue 5: ResLens Frontend Can't Connect to Middleware**

**Check:**
1. Is middleware running? `curl http://localhost:3003/api/v1/healthcheck`
2. Check middleware logs: `docker-compose -f docker-compose.dev.yml logs reslens-middleware`
3. Test network connectivity: `docker-compose -f docker-compose.dev.yml exec reslens-frontend wget -O- http://reslens-middleware:3003/api/v1/healthcheck`
4. Verify both containers are on the same network: `docker network inspect graphq-llm-network`

### **Issue 6: ResLens Middleware Can't Connect to ResilientDB**

**Check:**
1. Is ResilientDB running? `docker-compose -f docker-compose.dev.yml ps resilientdb`
2. Test ResilientDB KV service: `curl http://localhost:18000/v1/transactions/test`
3. Test from middleware container: `docker-compose -f docker-compose.dev.yml exec reslens-middleware curl http://resilientdb:18000/v1/transactions/test`
4. Check middleware logs: `docker-compose -f docker-compose.dev.yml logs reslens-middleware`

---

## 🎯 Success Summary

All services are communicating correctly when:

**Core Services (Required):**
- ✅ ResilientDB responds to GraphQL queries
- ✅ GraphQ-LLM Backend health check passes
- ✅ GraphQ-LLM Backend can query ResilientDB
- ✅ MCP Server is running (logs show "started")
- ✅ MCP Server can reach ResilientDB (internal network)
- ✅ Nexus can call GraphQ-LLM Backend API
- ✅ All containers are on the same Docker network

**ResLens Services (Optional):**
- ✅ ResLens Middleware responds to health check (`http://localhost:3003/api/v1/healthcheck`)
- ✅ ResLens Middleware can reach ResilientDB (internal network)
- ✅ ResLens Frontend serves HTML (`http://localhost:5173`)
- ✅ ResLens Frontend can communicate with Middleware (internal network)
- ✅ ResLens UI is accessible in browser (`http://localhost:5173`)

---

**Ready to test? Start with Step 1! 🚀**

