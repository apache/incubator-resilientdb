# Troubleshooting ResLens Middleware Connection Errors

## Error: "Network error: Cannot connect to middleware. Please ensure ResLens Middleware is running on port 3003."

### Quick Fix (30 seconds)

1. **Check if middleware is running:**
   ```bash
   curl http://localhost:3003/api/v1/healthcheck
   ```
   - ✅ If you see `{"status":"UP",...}` → Middleware is running, refresh ResLens
   - ❌ If you get connection error → Middleware is not running, continue below

2. **Start middleware:**
   ```bash
   cd /Users/sandhya/UCD/ECS_DDS/graphq-llm
   ./start-middleware.sh
   ```

3. **Verify it's running:**
   ```bash
   curl http://localhost:3003/api/v1/healthcheck
   ```

4. **Refresh ResLens:**
   - Go to: http://localhost:5173/dashboard?tab=query_stats
   - Hard refresh: Ctrl+Shift+R (or Cmd+Shift+R on Mac)

---

## Manual Troubleshooting Steps

### Step 1: Check if Middleware is Running

```bash
# Check if process is running
ps aux | grep "node.*server.js" | grep middleware

# Check if port 3003 is in use
lsof -i:3003

# Test the API
curl http://localhost:3003/api/v1/healthcheck
```

**Expected Result:** `{"status":"UP","timestamp":"..."}`

---

### Step 2: If Middleware is Not Running

#### Option A: Use the Start Script (Recommended)
```bash
cd /Users/sandhya/UCD/ECS_DDS/graphq-llm
./start-middleware.sh
```

#### Option B: Start Manually
```bash
cd /Users/sandhya/UCD/ECS_DDS/ResLens-Middleware/middleware
PORT=3003 npm start
```

#### Option C: Start in Background (Keeps Running)
```bash
cd /Users/sandhya/UCD/ECS_DDS/ResLens-Middleware/middleware
PORT=3003 nohup npm start > /tmp/reslens-middleware.log 2>&1 &
```

---

### Step 3: Check Middleware Logs

```bash
# View recent logs
tail -50 /tmp/reslens-middleware.log

# Follow logs in real-time
tail -f /tmp/reslens-middleware.log
```

**Common Issues:**
- **Port already in use:** Kill the process: `lsof -ti:3003 | xargs kill -9`
- **Missing dependencies:** Run `npm install` in the middleware directory
- **Environment variables:** Check `.env` file in middleware directory

---

### Step 4: Verify ResLens Configuration

```bash
# Check ResLens .env file
cd /Users/sandhya/UCD/ECS_DDS/ResLens
cat .env | grep VITE_MIDDLEWARE
```

**Should show:**
```
VITE_MIDDLEWARE_BASE_URL=http://localhost:3003/api/v1
```

**If missing or wrong:**
1. Edit `.env` file
2. Add: `VITE_MIDDLEWARE_BASE_URL=http://localhost:3003/api/v1`
3. Restart ResLens: Stop (Ctrl+C) and run `npm run dev` again

---

### Step 5: Test the Full Connection

```bash
# Test middleware health
curl http://localhost:3003/api/v1/healthcheck

# Test query stats endpoint
curl http://localhost:3003/api/v1/queryStats

# Test from ResLens perspective (CORS)
curl -H "Origin: http://localhost:5173" http://localhost:3003/api/v1/queryStats
```

---

## Common Solutions

### Solution 1: Middleware Stopped Unexpectedly

**Cause:** Process was killed or crashed

**Fix:**
```bash
cd /Users/sandhya/UCD/ECS_DDS/graphq-llm
./start-middleware.sh
```

---

### Solution 2: Port 3003 Already in Use

**Cause:** Another process is using port 3003

**Fix:**
```bash
# Find and kill the process
lsof -ti:3003 | xargs kill -9

# Then restart
cd /Users/sandhya/UCD/ECS_DDS/graphq-llm
./start-middleware.sh
```

---

### Solution 3: CORS Error in Browser

**Cause:** Browser blocking cross-origin requests

**Fix:**
- Middleware already has CORS enabled
- Check browser console (F12) for specific CORS errors
- Verify middleware is running: `curl http://localhost:3003/api/v1/healthcheck`

---

### Solution 4: ResLens Not Reading Environment Variables

**Cause:** ResLens needs to be restarted after .env changes

**Fix:**
1. Stop ResLens (Ctrl+C)
2. Verify `.env` has `VITE_MIDDLEWARE_BASE_URL=http://localhost:3003/api/v1`
3. Restart: `npm run dev`

---

## Keep Middleware Running Permanently

### Option 1: Use nohup (Simple)
```bash
cd /Users/sandhya/UCD/ECS_DDS/ResLens-Middleware/middleware
PORT=3003 nohup npm start > /tmp/reslens-middleware.log 2>&1 &
```

### Option 2: Use PM2 (Advanced - Auto-restart)
```bash
# Install PM2 globally
npm install -g pm2

# Start middleware with PM2
cd /Users/sandhya/UCD/ECS_DDS/ResLens-Middleware/middleware
PORT=3003 pm2 start npm --name "reslens-middleware" -- start

# Save PM2 configuration
pm2 save

# Enable PM2 to start on system boot
pm2 startup
```

---

## Quick Reference Commands

```bash
# Start middleware
cd /Users/sandhya/UCD/ECS_DDS/graphq-llm && ./start-middleware.sh

# Check if running
curl http://localhost:3003/api/v1/healthcheck

# View logs
tail -f /tmp/reslens-middleware.log

# Kill middleware
lsof -ti:3003 | xargs kill -9

# Check process
ps aux | grep middleware | grep node
```

---

## Still Having Issues?

1. **Check all services are running:**
   - Middleware: `curl http://localhost:3003/api/v1/healthcheck`
   - ResLens: `curl http://localhost:5173`
   - Nexus: `curl http://localhost:3002`

2. **Check browser console (F12):**
   - Look for specific error messages
   - Check Network tab for failed requests

3. **Verify directory structure:**
   ```bash
   ls -d /Users/sandhya/UCD/ECS_DDS/{ResLens,ResLens-Middleware,graphq-llm}
   ```

4. **Restart everything:**
   ```bash
   # Kill all
   lsof -ti:3003 | xargs kill -9
   lsof -ti:5173 | xargs kill -9
   
   # Start middleware
   cd /Users/sandhya/UCD/ECS_DDS/graphq-llm && ./start-middleware.sh
   
   # Start ResLens (in another terminal)
   cd /Users/sandhya/UCD/ECS_DDS/ResLens && npm run dev
   ```

