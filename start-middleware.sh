#!/bin/bash

# Start ResLens Middleware Server
# This script ensures the middleware stays running

# Get the directory where graphq-llm is located
GRAPHQ_LLM_DIR="$(cd "$(dirname "$0")" && pwd)"
# ResLens-Middleware is in the same directory as graphq-llm
MIDDLEWARE_DIR="$(dirname "$GRAPHQ_LLM_DIR")/ResLens-Middleware/middleware"

if [ ! -d "$MIDDLEWARE_DIR" ]; then
    echo "❌ Error: ResLens-Middleware not found at: $MIDDLEWARE_DIR"
    echo "   Expected location: $(dirname "$GRAPHQ_LLM_DIR")/ResLens-Middleware/middleware"
    exit 1
fi

cd "$MIDDLEWARE_DIR"

echo "🚀 Starting ResLens Middleware on port 3003..."
echo "📍 Location: $(pwd)"
echo ""

# Check if port 3003 is already in use
if lsof -Pi :3003 -sTCP:LISTEN -t >/dev/null 2>&1 ; then
    echo "⚠️  Port 3003 is already in use. Killing existing process..."
    lsof -ti:3003 | xargs kill -9 2>/dev/null
    sleep 2
fi

# Start the middleware with nohup to keep it running
PORT=3003 nohup npm start > /tmp/reslens-middleware.log 2>&1 &
MIDDLEWARE_PID=$!

echo "✅ Middleware started with PID: $MIDDLEWARE_PID"
echo "📋 Logs: tail -f /tmp/reslens-middleware.log"
echo ""

# Wait a moment and check if it's running
sleep 3
if ps -p $MIDDLEWARE_PID > /dev/null 2>&1; then
    echo "✅ Middleware is running"
    echo "🧪 Test: curl http://localhost:3003/api/v1/healthcheck"
    echo ""
    echo "💡 To stop: kill $MIDDLEWARE_PID"
    echo "💡 To check status: ps aux | grep $MIDDLEWARE_PID"
else
    echo "❌ Middleware failed to start. Check logs:"
    tail -20 /tmp/reslens-middleware.log
    exit 1
fi
