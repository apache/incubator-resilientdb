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

# Setting Up Your Nexus Fork - Quick Guide

## Current Status ✅

You already have:
- ✅ Separate `nexus` repository at `/Users/sophiequynn/nexus`
- ✅ GraphQ-LLM integration files added
- ✅ Modified files ready to commit

**Current remote:** `https://github.com/ResilientApp/nexus.git` (original)

**What you need to do:** Fork on GitHub, change remote, commit & push

---

## Step 1: Fork Nexus on GitHub

1. Go to: https://github.com/ResilientApp/nexus
2. Click **"Fork"** button (top-right)
3. Select your GitHub account
4. Wait for fork to complete
5. **Copy your fork URL** - it will be:
   ```
   https://github.com/YOUR_USERNAME/nexus.git
   ```

---

## Step 2: Update Remote to Your Fork

```bash
cd /Users/sophiequynn/nexus

# Add your fork as a new remote (or rename origin)
git remote rename origin upstream
git remote add origin https://github.com/YOUR_USERNAME/nexus.git

# Verify remotes
git remote -v
```

**Expected output:**
```
origin    https://github.com/YOUR_USERNAME/nexus.git (fetch)
origin    https://github.com/YOUR_USERNAME/nexus.git (push)
upstream  https://github.com/ResilientApp/nexus.git (fetch)
upstream  https://github.com/ResilientApp/nexus.git (push)
```

---

## Step 3: Commit Your Changes

```bash
cd /Users/sophiequynn/nexus

# Stage all changes
git add .

# Check what will be committed
git status

# Commit with descriptive message
git commit -m "Add GraphQ-LLM integration: GraphQL Tutor UI and API routes

- Add GraphQL Tutor UI page and components
- Add API route for query analysis
- Update main page navigation
- Add required dependencies
- Add UI components (progress bar)"
```

---

## Step 4: Push to Your Fork

```bash
cd /Users/sophiequynn/nexus

# Push to your fork
git push -u origin main

# If you get an error about upstream, use:
# git push -u origin main --force
# (Only use --force if you're sure you want to overwrite your fork)
```

---

## Step 5: Update TEAM_SETUP.md

Update the Nexus URL in `graphq-llm/TEAM_SETUP.md`:

1. Find this section (around line 14-17):
   ```markdown
   2. **Nexus** (separate repository - must be cloned)
      - **URL:** https://github.com/ResilientApp/nexus.git
      - **Purpose:** Next.js frontend application
      - **Note:** Requires modifications (see Step 7.6)
   ```

2. Replace with:
   ```markdown
   2. **Nexus** (separate repository - must be cloned)
      - **URL:** https://github.com/YOUR_USERNAME/nexus.git
      - **Purpose:** Next.js frontend application
      - **Note:** This fork includes GraphQ-LLM integration (see Step 7.6 for details)
   ```

3. Also update Step 7.1 (around line 637):
   ```markdown
   git clone https://github.com/YOUR_USERNAME/nexus.git
   ```

---

## Step 6: Commit TEAM_SETUP.md Update

```bash
cd /Users/sophiequynn/graphq-llm

# Update TEAM_SETUP.md with your fork URL
# (Do this manually or use the script below)

# Commit and push
git add TEAM_SETUP.md
git commit -m "Update TEAM_SETUP.md to reference forked Nexus repository"
git push
```

---

## Quick Script (All Steps Combined)

Save this as a script and run it (replace `YOUR_USERNAME`):

```bash
#!/bin/bash

# Configuration
GITHUB_USERNAME="YOUR_USERNAME"  # Change this!
NEXUS_DIR="/Users/sophiequynn/nexus"
GRAPHQ_LLM_DIR="/Users/sophiequynn/graphq-llm"

echo "🔧 Setting up Nexus fork..."

cd "$NEXUS_DIR"

# Step 1: Update remotes
echo "📡 Updating git remotes..."
git remote rename origin upstream 2>/dev/null || echo "Remote already renamed or doesn't exist"
git remote add origin "https://github.com/${GITHUB_USERNAME}/nexus.git" 2>/dev/null || git remote set-url origin "https://github.com/${GITHUB_USERNAME}/nexus.git"
git remote -v

# Step 2: Commit changes
echo "📝 Committing GraphQ-LLM integration..."
git add .
git commit -m "Add GraphQ-LLM integration: GraphQL Tutor UI and API routes" || echo "No changes to commit or already committed"

# Step 3: Push to fork
echo "🚀 Pushing to fork..."
git push -u origin main || echo "Push failed - check if fork exists on GitHub"

echo ""
echo "✅ Nexus fork setup complete!"
echo ""
echo "Next steps:"
echo "1. Verify fork at: https://github.com/${GITHUB_USERNAME}/nexus"
echo "2. Update TEAM_SETUP.md with your fork URL"
echo "3. Commit TEAM_SETUP.md update to graphq-llm repo"
```

---

## Verification Checklist

After completing all steps:

- [ ] Fork exists at `https://github.com/YOUR_USERNAME/nexus`
- [ ] All GraphQ-LLM files are in the fork
- [ ] Changes are pushed to your fork
- [ ] `git remote -v` shows your fork as `origin`
- [ ] TEAM_SETUP.md references your fork URL
- [ ] TEAM_SETUP.md changes are committed to graphq-llm

---

## Future Updates

To pull updates from the original Nexus repository:

```bash
cd /Users/sophiequynn/nexus

# Fetch updates from original repo
git fetch upstream

# Merge updates
git merge upstream/main

# Push to your fork
git push origin main
```

This keeps your fork up-to-date while preserving your GraphQ-LLM integration.
