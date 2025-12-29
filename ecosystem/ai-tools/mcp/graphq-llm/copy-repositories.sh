#!/bin/bash
# Copy ResLens, ResLens-Middleware, and nexus into graphq-llm directory
# Excludes .git files and node_modules

set -e

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "📦 Copying Repositories into graphq-llm"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

BASE_DIR="/Users/sandhya/UCD/ECS_DDS"
TARGET_DIR="/Users/sandhya/UCD/ECS_DDS/graphq-llm"

# Create target directories
echo "Creating target directories..."
mkdir -p "$TARGET_DIR/ResLens"
mkdir -p "$TARGET_DIR/ResLens-Middleware"
mkdir -p "$TARGET_DIR/nexus"
echo "✅ Target directories created"
echo ""

# Copy ResLens
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Copying ResLens..."
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
if [ -d "$BASE_DIR/ResLens" ]; then
    echo "Source: $BASE_DIR/ResLens"
    echo "Target: $TARGET_DIR/ResLens"
    rsync -av --progress \
        --exclude='.git' \
        --exclude='.gitignore' \
        --exclude='node_modules' \
        --exclude='.next' \
        --exclude='dist' \
        --exclude='build' \
        "$BASE_DIR/ResLens/" "$TARGET_DIR/ResLens/"
    echo "✅ ResLens copied successfully"
else
    echo "❌ ResLens directory not found at: $BASE_DIR/ResLens"
fi
echo ""

# Copy ResLens-Middleware
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Copying ResLens-Middleware..."
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
if [ -d "$BASE_DIR/ResLens-Middleware" ]; then
    echo "Source: $BASE_DIR/ResLens-Middleware"
    echo "Target: $TARGET_DIR/ResLens-Middleware"
    rsync -av --progress \
        --exclude='.git' \
        --exclude='.gitignore' \
        --exclude='node_modules' \
        --exclude='.next' \
        --exclude='dist' \
        --exclude='build' \
        "$BASE_DIR/ResLens-Middleware/" "$TARGET_DIR/ResLens-Middleware/"
    echo "✅ ResLens-Middleware copied successfully"
else
    echo "❌ ResLens-Middleware directory not found at: $BASE_DIR/ResLens-Middleware"
fi
echo ""

# Copy nexus
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Copying nexus..."
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
if [ -d "$BASE_DIR/nexus" ]; then
    echo "Source: $BASE_DIR/nexus"
    echo "Target: $TARGET_DIR/nexus"
    rsync -av --progress \
        --exclude='.git' \
        --exclude='.gitignore' \
        --exclude='node_modules' \
        --exclude='.next' \
        --exclude='dist' \
        --exclude='build' \
        "$BASE_DIR/nexus/" "$TARGET_DIR/nexus/"
    echo "✅ nexus copied successfully"
else
    echo "❌ nexus directory not found at: $BASE_DIR/nexus"
fi
echo ""

# Verify no .git folders were copied
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Verifying .git folders were excluded..."
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
GIT_COUNT=$(find "$TARGET_DIR/ResLens" "$TARGET_DIR/ResLens-Middleware" "$TARGET_DIR/nexus" -type d -name ".git" 2>/dev/null | wc -l | tr -d ' ')
if [ "$GIT_COUNT" -eq 0 ]; then
    echo "✅ No .git folders found (as expected)"
else
    echo "⚠️  Found $GIT_COUNT .git folder(s) - removing them..."
    find "$TARGET_DIR/ResLens" "$TARGET_DIR/ResLens-Middleware" "$TARGET_DIR/nexus" -type d -name ".git" -exec rm -rf {} + 2>/dev/null || true
    echo "✅ .git folders removed"
fi
echo ""

# Show summary
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "📊 Copy Summary"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "Copied directories:"
ls -d "$TARGET_DIR"/ResLens "$TARGET_DIR"/ResLens-Middleware "$TARGET_DIR"/nexus 2>/dev/null | while read dir; do
    if [ -d "$dir" ]; then
        size=$(du -sh "$dir" 2>/dev/null | cut -f1)
        file_count=$(find "$dir" -type f 2>/dev/null | wc -l | tr -d ' ')
        dir_name=$(basename "$dir")
        echo "  ✅ $dir_name: $size ($file_count files)"
    fi
done
echo ""

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "✅ Copy Complete!"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "Next steps:"
echo "  1. Install dependencies in each directory:"
echo "     cd $TARGET_DIR/ResLens && npm install"
echo "     cd $TARGET_DIR/ResLens-Middleware/middleware && npm install"
echo "     cd $TARGET_DIR/nexus && npm install"
echo ""
echo "  2. Update fix-reslens.sh to use local directories:"
echo "     RESLENS_DIR=\"$TARGET_DIR/ResLens\""
echo "     MIDDLEWARE_DIR=\"$TARGET_DIR/ResLens-Middleware/middleware\""
echo "     NEXUS_DIR=\"$TARGET_DIR/nexus\""
echo ""

