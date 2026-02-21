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

# Script to set up ResLens forks and configure docker-compose

set -e

# Configuration
GITHUB_USERNAME="sophiequynn"
RESLENS_FRONTEND_FORK="incubator-resilientdb-ResLens"
RESLENS_MIDDLEWARE_FORK="incubator-resilientdb-ResLens-Middleware"

BASE_DIR="/Users/${GITHUB_USERNAME}"
GRAPHQ_LLM_DIR="/Users/${GITHUB_USERNAME}/graphq-llm"

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "🔧 Setting Up ResLens Forks"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

# Fork URLs
FRONTEND_FORK_URL="https://github.com/${GITHUB_USERNAME}/${RESLENS_FRONTEND_FORK}.git"
MIDDLEWARE_FORK_URL="https://github.com/${GITHUB_USERNAME}/${RESLENS_MIDDLEWARE_FORK}.git"

echo "📋 Configuration:"
echo "   GitHub Username: ${GITHUB_USERNAME}"
echo "   ResLens Frontend Fork: ${FRONTEND_FORK_URL}"
echo "   ResLens Middleware Fork: ${MIDDLEWARE_FORK_URL}"
echo ""

# Step 1: Check if forks need to be cloned
echo "📦 Step 1: Setting up ResLens Frontend..."
echo ""

if [ ! -d "${BASE_DIR}/ResLens" ]; then
    echo "   Cloning ResLens Frontend fork..."
    cd "${BASE_DIR}"
    git clone "${FRONTEND_FORK_URL}" ResLens
    cd ResLens
    git remote add upstream https://github.com/apache/incubator-resilientdb-ResLens.git 2>/dev/null || echo "   Upstream already exists"
    echo "   ✅ ResLens Frontend cloned"
else
    echo "   ✅ ResLens Frontend directory already exists"
    cd "${BASE_DIR}/ResLens"
    if [ -d ".git" ]; then
        echo "   Updating remotes..."
        git remote set-url origin "${FRONTEND_FORK_URL}" 2>/dev/null || git remote add origin "${FRONTEND_FORK_URL}"
        git remote add upstream https://github.com/apache/incubator-resilientdb-ResLens.git 2>/dev/null || echo "   Upstream already exists"
    else
        echo "   ⚠️  Not a git repository - initializing..."
        git init
        git remote add origin "${FRONTEND_FORK_URL}"
        git remote add upstream https://github.com/apache/incubator-resilientdb-ResLens.git
    fi
fi

echo ""
echo "   Remotes:"
git remote -v | sed 's/^/   /'
echo ""

# Step 2: Check if middleware needs to be cloned
echo "📦 Step 2: Setting up ResLens Middleware..."
echo ""

if [ ! -d "${BASE_DIR}/ResLens-Middleware" ]; then
    echo "   Cloning ResLens Middleware fork..."
    cd "${BASE_DIR}"
    git clone "${MIDDLEWARE_FORK_URL}" ResLens-Middleware
    cd ResLens-Middleware
    git remote add upstream https://github.com/apache/incubator-resilientdb-ResLens-Middleware.git 2>/dev/null || echo "   Upstream already exists"
    echo "   ✅ ResLens Middleware cloned"
else
    echo "   ✅ ResLens Middleware directory already exists"
    cd "${BASE_DIR}/ResLens-Middleware"
    if [ -d ".git" ]; then
        echo "   Updating remotes..."
        git remote set-url origin "${MIDDLEWARE_FORK_URL}" 2>/dev/null || git remote add origin "${MIDDLEWARE_FORK_URL}"
        git remote add upstream https://github.com/apache/incubator-resilientdb-ResLens-Middleware.git 2>/dev/null || echo "   Upstream already exists"
    else
        echo "   ⚠️  Not a git repository - initializing..."
        git init
        git remote add origin "${MIDDLEWARE_FORK_URL}"
        git remote add upstream https://github.com/apache/incubator-resilientdb-ResLens-Middleware.git
    fi
fi

echo ""
echo "   Remotes:"
git remote -v | sed 's/^/   /'
echo ""

# Step 3: Update docker-compose.dev.yml
echo "📝 Step 3: Updating docker-compose.dev.yml..."
echo ""

cd "${GRAPHQ_LLM_DIR}"

if [ -f "docker-compose.dev.yml" ]; then
    # Backup
    cp docker-compose.dev.yml docker-compose.dev.yml.bak
    
    # Update paths (use absolute paths for forks)
    sed -i '' "s|context: ./ResLens-Middleware/middleware|context: ${BASE_DIR}/ResLens-Middleware/middleware|g" docker-compose.dev.yml
    sed -i '' "s|context: ./ResLens|context: ${BASE_DIR}/ResLens|g" docker-compose.dev.yml
    
    echo "   ✅ docker-compose.dev.yml updated with fork paths"
    echo ""
    echo "   Changes:"
    echo "   - reslens-middleware: ${BASE_DIR}/ResLens-Middleware/middleware"
    echo "   - reslens-frontend: ${BASE_DIR}/ResLens"
else
    echo "   ⚠️  docker-compose.dev.yml not found"
fi

# Step 4: Remove from git tracking
echo ""
echo "🗑️  Step 4: Removing ResLens from git tracking..."
echo ""

if [ -d "ResLens" ] || [ -d "ResLens-Middleware" ]; then
    git rm -r --cached ResLens ResLens-Middleware 2>/dev/null || echo "   No tracked files to remove"
    echo "   ✅ Removed from git tracking"
else
    echo "   ℹ️  ResLens directories not found in graphq-llm (already removed or never tracked)"
fi

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "✅ Setup Complete!"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "Summary:"
echo "  ✅ ResLens Frontend: ${BASE_DIR}/ResLens"
echo "  ✅ ResLens Middleware: ${BASE_DIR}/ResLens-Middleware"
echo "  ✅ docker-compose.dev.yml updated"
echo "  ✅ ResLens removed from git tracking"
echo ""
echo "Next steps:"
echo "  1. Verify forks exist at:"
echo "     - ${FRONTEND_FORK_URL}"
echo "     - ${MIDDLEWARE_FORK_URL}"
echo "  2. Commit docker-compose.dev.yml changes"
echo "  3. Update TEAM_SETUP.md with fork information"
echo ""

