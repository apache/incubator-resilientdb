# 🚀 Run Fix ResLens Script

## Execute This Command

Since I cannot run the script directly, **please run this in your terminal:**

```bash
cd /Users/sandhya/UCD/ECS_DDS/graphq-llm
chmod +x fix-reslens.sh
./fix-reslens.sh
```

---

## What Will Happen

The script will:

1. ✅ Check ResLens directory exists
2. ✅ Free port 5173 (kill any existing process)
3. ✅ Navigate to ResLens directory
4. ✅ Install dependencies if needed
5. ✅ Create/update `.env` file
6. ✅ Clear Vite cache
7. ✅ Start ResLens on port 5173
8. ✅ Verify it's accessible

---

## Expected Output

You should see output like:

```
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
🔧 Fixing ResLens on Port 5173
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

Step 1: Checking ResLens directory...
✅ ResLens directory found

Step 2: Checking port 5173...
✅ Port 5173 is free

Step 3: Navigated to ResLens directory
📍 Location: /Users/sandhya/UCD/ECS_DDS/ResLens

...

Step 9: Starting ResLens...
🚀 Starting ResLens development server on port 5173...

✅ ResLens started with PID: xxxxx
📋 Logs: tail -f /tmp/reslens.log

Step 10: Waiting for ResLens to start (10 seconds)...
Checking if ResLens is accessible...
✅ ResLens is accessible at http://localhost:5173

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
✅ ResLens is now running!
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

---

## After Script Completes

1. **Open ResLens in browser:**
   - URL: http://localhost:5173

2. **Check logs if needed:**
   ```bash
   tail -f /tmp/reslens.log
   ```

3. **Verify it's working:**
   - You should see the ResLens dashboard
   - Query Stats tab should be available

---

## If Script Fails

### Check Logs:
```bash
tail -20 /tmp/reslens.log
```

### Manual Steps:
If the script fails, follow these manual steps:

```bash
# 1. Kill any process on port 5173
lsof -ti:5173 | xargs kill -9 2>/dev/null || true

# 2. Navigate to ResLens
cd /Users/sandhya/UCD/ECS_DDS/ResLens

# 3. Install dependencies
npm install

# 4. Create .env file
cat > .env << 'EOF'
VITE_MIDDLEWARE_BASE_URL=http://localhost:3003/api/v1
VITE_MIDDLEWARE_SECONDARY_BASE_URL=http://localhost:3003/api/v1
VITE_DEV_RESLENS_TOOLS_URL=http://localhost:3003
EOF

# 5. Clear Vite cache
rm -rf node_modules/.vite

# 6. Start ResLens
npm run dev
```

---

## Troubleshooting

### Issue: "ResLens directory not found"
**Fix:** Clone ResLens first:
```bash
cd /Users/sandhya/UCD/ECS_DDS
git clone https://github.com/harish876/ResLens.git
```

### Issue: "Port 5173 already in use"
**Fix:** The script should handle this, but if it doesn't:
```bash
lsof -ti:5173 | xargs kill -9
```

### Issue: "Module not found" errors
**Fix:** Reinstall dependencies:
```bash
cd /Users/sandhya/UCD/ECS_DDS/ResLens
rm -rf node_modules
npm install
```

### Issue: ResLens starts but shows errors
**Fix:** Check if middleware is running:
```bash
curl http://localhost:3003/api/v1/healthcheck
```

If middleware is not running, start it:
```bash
cd /Users/sandhya/UCD/ECS_DDS/ResLens-Middleware/middleware
PORT=3003 npm start
```

---

**Run the command above in your terminal now! 🚀**

