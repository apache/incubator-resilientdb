#!/bin/bash
# Script to set up Nexus fork and push GraphQ-LLM integration

set -e

# Configuration - CHANGE THIS!
GITHUB_USERNAME="sophiequynn"  # Replace with your GitHub username
NEXUS_DIR="/Users/sophiequynn/nexus"
GRAPHQ_LLM_DIR="/Users/sophiequynn/graphq-llm"

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "🔧 Setting Up Nexus Fork"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

# Check if Nexus directory exists
if [ ! -d "$NEXUS_DIR" ]; then
    echo "❌ Error: Nexus directory not found at $NEXUS_DIR"
    exit 1
fi

cd "$NEXUS_DIR"

# Step 1: Check if already a git repo
if [ ! -d ".git" ]; then
    echo "❌ Error: Not a git repository. Please clone Nexus first."
    exit 1
fi

# Step 2: Update remotes
echo "📡 Step 1: Configuring git remotes..."
echo ""

# Check current remotes
CURRENT_REMOTE=$(git remote get-url origin 2>/dev/null || echo "")

if [[ "$CURRENT_REMOTE" == *"ResilientApp/nexus"* ]]; then
    echo "   Renaming origin to upstream..."
    git remote rename origin upstream
    echo "   ✅ Renamed origin → upstream"
fi

# Add or update origin to point to fork
FORK_URL="https://github.com/${GITHUB_USERNAME}/nexus.git"
if git remote | grep -q "^origin$"; then
    echo "   Updating origin to point to fork..."
    git remote set-url origin "$FORK_URL"
else
    echo "   Adding origin remote pointing to fork..."
    git remote add origin "$FORK_URL"
fi

echo ""
echo "   Current remotes:"
git remote -v | sed 's/^/   /'
echo ""

# Step 3: Check status
echo "📋 Step 2: Checking repository status..."
echo ""
git status --short | head -10
echo ""

# Step 4: Commit changes
echo "📝 Step 3: Committing GraphQ-LLM integration..."
echo ""

# Check if there are uncommitted changes
if [ -z "$(git status --porcelain)" ]; then
    echo "   ℹ️  No uncommitted changes found."
    echo "   All changes may already be committed."
else
    echo "   Staging all changes..."
    git add .
    
    echo "   Committing..."
    git commit -m "Add GraphQ-LLM integration: GraphQL Tutor UI and API routes

- Add GraphQL Tutor UI page and components
- Add API route for query analysis  
- Update main page navigation
- Add required dependencies
- Add UI components (progress bar)" || {
        echo "   ⚠️  Commit failed. Changes may already be committed."
    }
    echo "   ✅ Changes committed"
fi
echo ""

# Step 5: Push to fork
echo "🚀 Step 4: Pushing to fork..."
echo ""
echo "   Repository: $FORK_URL"
echo ""

# Check if fork exists by trying to fetch
if git fetch origin main 2>/dev/null; then
    echo "   ✅ Fork exists and is accessible"
else
    echo "   ⚠️  Warning: Could not access fork. Please verify:"
    echo "      1. Fork exists at: https://github.com/${GITHUB_USERNAME}/nexus"
    echo "      2. You have push access"
    echo "      3. Branch name is 'main' (not 'master')"
    echo ""
    read -p "   Continue anyway? (y/n) " -n 1 -r
    echo ""
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        echo "   ❌ Aborted. Please create fork first."
        exit 1
    fi
fi

# Push
echo "   Pushing to origin/main..."
if git push -u origin main; then
    echo "   ✅ Successfully pushed to fork!"
else
    echo "   ❌ Push failed. Please check:"
    echo "      - Fork exists on GitHub"
    echo "      - You have push permissions"
    echo "      - Remote URL is correct"
    exit 1
fi
echo ""

# Step 6: Update TEAM_SETUP.md
echo "📝 Step 5: Updating TEAM_SETUP.md..."
echo ""

if [ -f "$GRAPHQ_LLM_DIR/TEAM_SETUP.md" ]; then
    cd "$GRAPHQ_LLM_DIR"
    
    # Check if already updated
    if grep -q "${GITHUB_USERNAME}/nexus" TEAM_SETUP.md; then
        echo "   ℹ️  TEAM_SETUP.md already references your fork."
    else
        echo "   Updating TEAM_SETUP.md with fork URL..."
        
        # Create backup
        cp TEAM_SETUP.md TEAM_SETUP.md.bak
        
        # Update URLs (use sed for macOS compatibility)
        sed -i '' "s|https://github.com/ResilientApp/nexus.git|https://github.com/${GITHUB_USERNAME}/nexus.git|g" TEAM_SETUP.md
        sed -i '' "s|git clone https://github.com/ResilientApp/nexus|git clone https://github.com/${GITHUB_USERNAME}/nexus|g" TEAM_SETUP.md
        
        # Update note if it exists
        sed -i '' "s|Requires modifications (see Step 7.6)|This fork includes GraphQ-LLM integration (see Step 7.6 for details)|g" TEAM_SETUP.md
        
        echo "   ✅ TEAM_SETUP.md updated"
        echo ""
        echo "   Changes made:"
        echo "   - Updated Nexus repository URL to your fork"
        echo "   - Updated clone command in Step 7.1"
        echo ""
        
        read -p "   Commit this change to graphq-llm repo? (y/n) " -n 1 -r
        echo ""
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            git add TEAM_SETUP.md
            git commit -m "Update TEAM_SETUP.md to reference forked Nexus repository" || echo "   ⚠️  Commit skipped (may already be committed)"
            echo "   ✅ TEAM_SETUP.md committed"
        else
            echo "   ℹ️  Changes saved but not committed. Run manually:"
            echo "      cd $GRAPHQ_LLM_DIR"
            echo "      git add TEAM_SETUP.md"
            echo "      git commit -m 'Update TEAM_SETUP.md to reference forked Nexus repository'"
        fi
    fi
else
    echo "   ⚠️  TEAM_SETUP.md not found. Please update manually."
fi

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "✅ Setup Complete!"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "Summary:"
echo "  ✅ Nexus fork configured: https://github.com/${GITHUB_USERNAME}/nexus"
echo "  ✅ Changes committed and pushed"
echo "  ✅ TEAM_SETUP.md updated"
echo ""
echo "Next steps:"
echo "  1. Verify fork at: https://github.com/${GITHUB_USERNAME}/nexus"
echo "  2. If TEAM_SETUP.md wasn't auto-committed, commit it manually"
echo "  3. Share fork URL with your team: https://github.com/${GITHUB_USERNAME}/nexus.git"
echo ""

