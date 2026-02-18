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

# Script to clone and set up Nexus for Docker Compose deployment

set -e

NEXUS_DIR="./nexus"
NEXUS_REPO="https://github.com/sophiequynn/nexus.git"

echo "🔍 Checking for Nexus directory..."

if [ ! -d "$NEXUS_DIR" ]; then
    echo "📦 Nexus not found. Cloning from GitHub..."
    git clone $NEXUS_REPO $NEXUS_DIR
    echo "✅ Nexus cloned successfully"
else
    echo "✅ Nexus directory already exists"
fi

# Check if GraphQL Tutor integration files exist
if [ ! -f "$NEXUS_DIR/src/app/api/graphql-tutor/analyze/route.ts" ]; then
    echo "⚠️  Warning: GraphQL Tutor integration not found in Nexus"
    echo "   Please add the integration files from NEXUS_UI_EXTENSION_GUIDE.md"
    echo "   Or Nexus will work but without the GraphQL Tutor feature"
fi

echo "✅ Nexus setup complete!"
echo ""
echo "Next steps:"
echo "1. If GraphQL Tutor integration is needed, add files from NEXUS_UI_EXTENSION_GUIDE.md"
echo "2. Run: docker-compose up -d"

