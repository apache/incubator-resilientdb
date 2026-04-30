<!--
 Licensed to the Apache Software Foundation (ASF) under one
 or more contributor license agreements.  See the NOTICE file
 distributed with this work for additional information
 regarding copyright ownership.  The ASF licenses this file
 to you under the Apache License, Version 2.0 (the
 "License"); you may not use this file except in compliance
 with the License.  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing,
 software distributed under the License is distributed on an
 "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 KIND, either express or implied.  See the License for the
 specific language governing permissions and limitations
 under the License.
--> 

# ResilientDB Ecosystem

This directory contains the ResilientDB ecosystem components organized as git subtrees.

## Structure

```
ecosystem/
├── ai-tools/
│   ├── beacon/
│   ├── mcp/
│   │   ├── graphq-llm/
│   │   ├── resilientdb-mcp/
│   │   └── ResInsight/
│   └── nexus/
├── cache/
│   ├── resilient-node-cache/
│   └── resilient-python-cache/
├── deployment/
│   ├── ansible/
│   └── orbit/
├── graphql/
├── monitoring/
│   ├── reslens/
│   └── reslens-middleware/
├── sdk/
│   ├── resdb-orm/
│   ├── resvault-sdk/
│   └── rust-sdk/
├── smart-contract/
│   ├── rescontract/
│   ├── resilient-contract-kit/
│   └── smart-contract-graphql/
├── third_party/
│   └── pocketflow/
└── tools/
    ├── create-resilient-app/
    ├── drawing-lib/
    ├── reshare-lib/                # ResShare SDK
    └── resvault/
```

## Clone Without Ecosystem

To clone ResilientDB without the ecosystem directory (faster, smaller):

```bash
git clone --filter=tree:0 --sparse https://github.com/apache/incubator-resilientdb.git
cd incubator-resilientdb
git sparse-checkout set --no-cone
echo "/*" > .git/info/sparse-checkout
echo "\!ecosystem/" >> .git/info/sparse-checkout
git read-tree -m -u HEAD
```

## Clone Specific Ecosystem Components

```bash
# Only SDK components. Nothing else
git sparse-checkout set ecosystem/sdk

# Only monitoring tools. Nothing else
git sparse-checkout set ecosystem/monitoring

# Multiple ecosystem components. Multiple components can be added here, if the first method is not your preference.
git sparse-checkout set ecosystem/sdk ecosystem/graphql ecosystem/monitoring
```
