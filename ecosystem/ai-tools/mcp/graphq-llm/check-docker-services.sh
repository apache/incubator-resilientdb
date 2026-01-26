#!/bin/bash
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.

# Check which services are running in Docker

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "🐳 Docker Services Status"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

cd /Users/sandhya/UCD/ECS_DDS/graphq-llm

# Check Docker Compose services
echo "📋 Services defined in docker-compose.dev.yml:"
docker-compose -f docker-compose.dev.yml config --services
echo ""

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "🔍 Running Docker Containers:"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
docker-compose -f docker-compose.dev.yml ps
echo ""

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "🌐 Service URLs and Status:"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

# Check each service
services=(
    "resilientdb:18000:ResilientDB KV"
    "resilientdb:5001:ResilientDB GraphQL"
    "graphq-llm-backend:3001:GraphQ-LLM Backend"
    "reslens-middleware:3003:ResLens Middleware"
    "reslens-frontend:5173:ResLens Frontend"
    "nexus-frontend:3002:Nexus Frontend"
)

for service_info in "${services[@]}"; do
    IFS=':' read -r service port name <<< "$service_info"
    
    # Check if container is running
    if docker ps --format '{{.Names}}' | grep -q "^${service}$"; then
        status="✅ Running"
        
        # Try to test the service
        if [ "$port" == "18000" ]; then
            # ResilientDB KV - hard to test, just check container
            test_result="Container running"
        elif [ "$port" == "5001" ]; then
            # ResilientDB GraphQL
            if curl -s -X POST http://localhost:5001/graphql -H "Content-Type: application/json" -d '{"query": "{ __typename }"}' > /dev/null 2>&1; then
                test_result="✅ Accessible"
            else
                test_result="⚠️  Not responding"
            fi
        elif [ "$port" == "3001" ]; then
            # GraphQ-LLM Backend
            if curl -s http://localhost:3001/health | grep -q "ok" 2>/dev/null; then
                test_result="✅ Healthy"
            else
                test_result="⚠️  Not responding"
            fi
        elif [ "$port" == "3003" ]; then
            # ResLens Middleware
            if curl -s http://localhost:3003/api/v1/healthcheck | grep -q "UP" 2>/dev/null; then
                test_result="✅ Healthy"
            else
                test_result="⚠️  Not responding"
            fi
        elif [ "$port" == "5173" ]; then
            # ResLens Frontend
            if curl -s http://localhost:5173 > /dev/null 2>&1; then
                test_result="✅ Accessible"
            else
                test_result="⚠️  Not responding"
            fi
        elif [ "$port" == "3002" ]; then
            # Nexus Frontend
            if curl -s http://localhost:3002 > /dev/null 2>&1; then
                test_result="✅ Accessible"
            else
                test_result="⚠️  Not responding"
            fi
        fi
    else
        status="❌ Not Running"
        test_result="Container not found"
    fi
    
    printf "%-25s %-15s %s\n" "$name" "$status" "$test_result"
done

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "📊 Summary:"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

# Count running containers
running_count=$(docker-compose -f docker-compose.dev.yml ps --services --filter "status=running" | wc -l | tr -d ' ')
total_count=$(docker-compose -f docker-compose.dev.yml config --services | wc -l | tr -d ' ')

echo "Running: $running_count / $total_count services"
echo ""

# Check for local services
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "💻 Local Services (Not Docker):"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

# Check for local Nexus on port 3002
if lsof -ti:3002 >/dev/null 2>&1 && ! docker ps --format '{{.Names}}' | grep -q "nexus-frontend"; then
    echo "✅ Nexus running locally on port 3002 (not Docker)"
fi

# Check for local ResLens on port 5173
if lsof -ti:5173 >/dev/null 2>&1 && ! docker ps --format '{{.Names}}' | grep -q "reslens-frontend"; then
    echo "✅ ResLens running locally on port 5173 (not Docker)"
fi

# Check for local middleware on port 3003
if lsof -ti:3003 >/dev/null 2>&1 && ! docker ps --format '{{.Names}}' | grep -q "reslens-middleware"; then
    echo "✅ ResLens Middleware running locally on port 3003 (not Docker)"
fi

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "✅ Check Complete!"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

