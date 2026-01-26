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

# Setting Up Your ResLens Forks

## Overview

ResLens consists of two separate repositories that should be forked and maintained separately:
1. **ResLens Frontend** - React/Vite dashboard UI
2. **ResLens Middleware** - Node.js Express API server

This guide will help you set up forks of both repositories, similar to how Nexus was set up.

---

## Step 1: Fork ResLens Repositories on GitHub

### 1.1: Fork ResLens Frontend

1. Go to: https://github.com/Bismanpal-Singh/MemLens
2. Click the **"Fork"** button (top-right)
3. Select your GitHub account
4. Wait for fork to complete
5. **Copy your fork URL** - it will be:
   ```
   https://github.com/YOUR_USERNAME/MemLens.git
   ```

### 1.2: Fork ResLens Middleware

1. Go to: https://github.com/harish876/MemLens-middleware
2. Click the **"Fork"** button (top-right)
3. Select your GitHub account
4. Wait for fork to complete
5. **Copy your fork URL** - it will be:
   ```
   https://github.com/YOUR_USERNAME/MemLens-middleware.git
   ```

---

## Step 2: Clone Your Forks (Separate from graphq-llm)

```bash
# Navigate to your workspace (outside graphq-llm)
cd /Users/sophiequynn

# Clone ResLens Frontend fork
git clone https://github.com/YOUR_USERNAME/MemLens.git ResLens
cd ResLens

# Add the original repository as upstream
git remote add upstream https://github.com/Bismanpal-Singh/MemLens.git

# Verify remotes
git remote -v

# Go back and clone ResLens Middleware fork
cd ..
git clone https://github.com/YOUR_USERNAME/MemLens-middleware.git ResLens-Middleware
cd ResLens-Middleware

# Add the original repository as upstream
git remote add upstream https://github.com/harish876/MemLens-middleware.git

# Verify remotes
git remote -v
```

---

## Step 3: Update Docker Compose Configuration

Update `docker-compose.dev.yml` to use your forks. You have two options:

### Option A: Update Paths Directly

Change the build contexts to point to your forks:

```yaml
reslens-middleware:
  build:
    context: /Users/YOUR_USERNAME/ResLens-Middleware/middleware
    dockerfile: Dockerfile
    
reslens-frontend:
  build:
    context: /Users/YOUR_USERNAME/ResLens
    dockerfile: Dockerfile
```

### Option B: Use Environment Variables (Recommended)

Update `docker-compose.dev.yml` to use environment variables for paths:

```yaml
reslens-middleware:
  build:
    context: ${RESLENS_MIDDLEWARE_PATH:-./ResLens-Middleware/middleware}
    
reslens-frontend:
  build:
    context: ${RESLENS_FRONTEND_PATH:-./ResLens}
```

Then set in `.env`:
```env
RESLENS_MIDDLEWARE_PATH=/Users/YOUR_USERNAME/ResLens-Middleware/middleware
RESLENS_FRONTEND_PATH=/Users/YOUR_USERNAME/ResLens
```

---

## Step 4: Add ResLens Directories to .gitignore

Add to `.gitignore`:

```gitignore
# External repositories (cloned locally for Docker builds)
nexus/
ResLens/
ResLens-Middleware/
```

---

## Step 5: Remove ResLens from Git Tracking

```bash
cd /Users/sophiequynn/graphq-llm

# Remove from git tracking (keeps local files)
git rm -r --cached ResLens ResLens-Middleware

# Commit the change
git add .gitignore
git commit -m "Remove ResLens directories from git tracking (now using separate forks)"
git push
```

---

## Step 6: Update Documentation

Update `TEAM_SETUP.md` to reference your forks:

1. Add to Quick Reference section:
   ```markdown
   4. **ResLens Frontend** (separate repository - optional)
      - **URL:** https://github.com/YOUR_USERNAME/MemLens.git
      - **Purpose:** Performance monitoring dashboard UI
   
   5. **ResLens Middleware** (separate repository - optional)
      - **URL:** https://github.com/YOUR_USERNAME/MemLens-middleware.git
      - **Purpose:** Performance monitoring API server
   ```

2. Update Step 5.2 in `TEAM_SETUP.md` to include clone instructions for your forks.

---

## Step 7: Verify Setup

```bash
# Check ResLens Frontend
cd /Users/YOUR_USERNAME/ResLens
git remote -v  # Should show your fork as origin

# Check ResLens Middleware
cd /Users/YOUR_USERNAME/ResLens-Middleware
git remote -v  # Should show your fork as origin

# Verify graphq-llm ignores ResLens
cd /Users/YOUR_USERNAME/graphq-llm
git status  # Should not show ResLens directories
```

---

## Future Updates

To pull updates from the original repositories:

```bash
# ResLens Frontend
cd /Users/YOUR_USERNAME/ResLens
git fetch upstream
git merge upstream/main
git push origin main

# ResLens Middleware
cd /Users/YOUR_USERNAME/ResLens-Middleware
git fetch upstream
git merge upstream/main
git push origin main
```

---

## Summary

After completing these steps:
- ✅ Both ResLens repositories are forked on GitHub
- ✅ Forks are cloned separately (outside graphq-llm)
- ✅ Docker Compose can build from fork paths
- ✅ ResLens directories are removed from graphq-llm git tracking
- ✅ Documentation references your forks
- ✅ You can pull upstream updates independently

