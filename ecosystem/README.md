# ResilientDB Ecosystem

This directory contains the ResilientDB ecosystem components organized as git subtrees.

## Structure

```
ecosystem/
├── graphql/                          # GraphQL service
├── smart-contract/                   # Smart contract ecosystem
│   ├── rescontract/                  # ResContract repository
│   ├── smart-contract-graphql/       # Smart contract GraphQL service
│   └── resilient-contract-kit/       # Contract development toolkit
├── monitoring/                       # Monitoring and observability
│   ├── reslens/                      # ResLens monitoring tool
│   └── reslens-middleware/           # ResLens middleware
├── cache/                           # Caching implementations
│   ├── resilient-node-cache/        # Node.js caching
│   └── resilient-python-cache/      # Python caching
├── sdk/                             # Software Development Kits
│   ├── rust-sdk/                    # Rust SDK
│   ├── resvault-sdk/                # ResVault SDK
│   └── resdb-orm/                   # Python ORM
├── deployment/                      # Deployment and infrastructure
│   ├── ansible/                     # Ansible playbooks
│   └── orbit/                       # Orbit deployment tool
└── tools/                           # Development and operational tools
    ├── resvault/                    # ResVault tool
    └── create-resilient-app/        # App scaffolding tool
```

## Clone Without Ecosystem

To clone ResilientDB without the ecosystem directory (faster, smaller):

```bash
git clone --filter=tree:0 --sparse https://github.com/harish876/incubator-resilientdb.git
cd incubator-resilientdb
git checkout monorepo-setup
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
