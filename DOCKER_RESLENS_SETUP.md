# Docker Setup for ResLens and ResLens-Middleware

This document describes the Docker setup for ResLens Frontend and ResLens-Middleware services.

## Overview

Both services are now containerized and can be started using Docker Compose:

- **ResLens Frontend**: React/Vite application served via Nginx
- **ResLens Middleware**: Node.js Express API server

## Dockerfiles

### ResLens Frontend (`ResLens/Dockerfile`)

Multi-stage build:
1. **Base**: Node.js 20 Alpine with build dependencies
2. **Deps**: Install npm dependencies
3. **Builder**: Build the Vite application with environment variables
4. **Runner**: Nginx Alpine serving the built static files

**Features:**
- Build-time environment variables for middleware URL
- Nginx configuration for SPA routing
- Gzip compression enabled
- Static asset caching
- Health check endpoint

**Port:** 80 (mapped to 5173 in docker-compose)

### ResLens Middleware (`ResLens-Middleware/middleware/Dockerfile`)

Single-stage production build:
- Node.js 18 Alpine
- Production dependencies only
- Go installed for pprof tool (if needed)
- Health check endpoint

**Port:** 3003

## Docker Compose Configuration

Both services are included in `docker-compose.dev.yml`:

```yaml
reslens-middleware:
  build:
    context: ./ResLens-Middleware/middleware
    dockerfile: Dockerfile
  ports:
    - "3003:3003"
  depends_on:
    resilientdb:
      condition: service_started

reslens-frontend:
  build:
    context: ./ResLens
    dockerfile: Dockerfile
    args:
      - VITE_MIDDLEWARE_BASE_URL=http://reslens-middleware:3003/api/v1
  ports:
    - "5173:80"
  depends_on:
    reslens-middleware:
      condition: service_healthy
```

## Usage

### Start All Services

```bash
cd /Users/sandhya/UCD/ECS_DDS/graphq-llm
docker-compose -f docker-compose.dev.yml up -d reslens-middleware reslens-frontend
```

### Start Individual Services

**ResLens Middleware only:**
```bash
docker-compose -f docker-compose.dev.yml up -d reslens-middleware
```

**ResLens Frontend only:**
```bash
docker-compose -f docker-compose.dev.yml up -d reslens-frontend
```

### Build Images

```bash
# Build both
docker-compose -f docker-compose.dev.yml build reslens-middleware reslens-frontend

# Build individual
docker-compose -f docker-compose.dev.yml build reslens-middleware
docker-compose -f docker-compose.dev.yml build reslens-frontend
```

### View Logs

```bash
# Both services
docker-compose -f docker-compose.dev.yml logs -f reslens-middleware reslens-frontend

# Individual
docker-compose -f docker-compose.dev.yml logs -f reslens-middleware
docker-compose -f docker-compose.dev.yml logs -f reslens-frontend
```

### Stop Services

```bash
docker-compose -f docker-compose.dev.yml stop reslens-middleware reslens-frontend
```

### Remove Services

```bash
docker-compose -f docker-compose.dev.yml down reslens-middleware reslens-frontend
```

## Environment Variables

### ResLens Frontend

Build-time variables (set during `docker build`):
- `VITE_MIDDLEWARE_BASE_URL`: URL to middleware API (default: `http://reslens-middleware:3003/api/v1`)
- `VITE_MIDDLEWARE_SECONDARY_BASE_URL`: Secondary middleware URL

### ResLens Middleware

Runtime environment variables (set in docker-compose):
- `PORT`: Server port (default: 3003)
- `CPP_STATS_API_BASE_URL`: ResilientDB stats API
- `CPP_TRANSACTIONS_API_BASE_URL`: ResilientDB transactions API
- `PYROSCOPE_SERVER_URL`: Pyroscope server URL
- `PROMETHEUS_URL`: Prometheus server URL
- `NODE_EXPORTER_BASE_URL`: Node exporter URL
- `PROCESS_EXPORTER_BASE_URL`: Process exporter URL
- `EXPLORER_BASE_URL`: Explorer API URL

## Networking

Both services are on the `graphq-llm-network` Docker network, allowing them to communicate with:
- ResilientDB (via service name `resilientdb`)
- GraphQ-LLM Backend (via service name `graphq-llm-backend`)
- Each other (via service names `reslens-middleware` and `reslens-frontend`)

## Health Checks

### ResLens Middleware
- Endpoint: `http://localhost:3003/api/v1/healthcheck`
- Check: HTTP 200 response

### ResLens Frontend
- Endpoint: `http://localhost:5173/`
- Check: Nginx serving index.html

## Troubleshooting

### ResLens Frontend can't connect to Middleware

1. **Check middleware is running:**
   ```bash
   docker-compose -f docker-compose.dev.yml ps reslens-middleware
   curl http://localhost:3003/api/v1/healthcheck
   ```

2. **Check middleware logs:**
   ```bash
   docker-compose -f docker-compose.dev.yml logs reslens-middleware
   ```

3. **Verify network connectivity:**
   ```bash
   docker-compose -f docker-compose.dev.yml exec reslens-frontend wget -O- http://reslens-middleware:3003/api/v1/healthcheck
   ```

### Build Failures

1. **Clear build cache:**
   ```bash
   docker-compose -f docker-compose.dev.yml build --no-cache reslens-middleware reslens-frontend
   ```

2. **Check Dockerfile syntax:**
   ```bash
   docker build -t test-reslens -f ResLens/Dockerfile ResLens/
   docker build -t test-middleware -f ResLens-Middleware/middleware/Dockerfile ResLens-Middleware/middleware/
   ```

### Port Conflicts

If ports 3003 or 5173 are already in use:

1. **Find process using port:**
   ```bash
   lsof -i:3003
   lsof -i:5173
   ```

2. **Kill process or change port in docker-compose.dev.yml:**
   ```yaml
   ports:
     - "3004:3003"  # Change host port
   ```

## Development vs Production

### Development (Local)
- Run services locally with `npm run dev` / `npm start`
- Hot reload enabled
- Easier debugging

### Production (Docker)
- Optimized builds
- Static file serving via Nginx
- Production dependencies only
- Health checks enabled
- Better resource management

## Next Steps

1. **Start all services:**
   ```bash
   docker-compose -f docker-compose.dev.yml up -d
   ```

2. **Verify services:**
   ```bash
   curl http://localhost:3003/api/v1/healthcheck
   curl http://localhost:5173
   ```

3. **Access ResLens:**
   - Frontend: http://localhost:5173
   - Middleware API: http://localhost:3003/api/v1

