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

![](https://img.shields.io/github/v/release/resilientdb/resilientdb)
![](https://img.shields.io/badge/language-c++-orange.svg)
![](https://img.shields.io/badge/platform-Ubuntu20.0+-lightgrey.svg)
![GitHub](https://img.shields.io/github/license/resilientdb/resilientdb)
![Generated Button](https://raw.githubusercontent.com/resilientdb/resilientdb/image-data/badge.svg)
![build](https://github.com/resilientdb/resilientdb/workflows/bazel-build%20CI/badge.svg)
![build](https://github.com/resilientdb/resilientdb/workflows/Unite%20Test/badge.svg)



# ResilientDB: Global-Scale Sustainable Blockchain Fabric

**[ResilientDB](https://resilientdb.com/)** is a **High Throughput Yielding Permissioned Blockchain Fabric** founded by **[ExpoLab](https://expolab.org/)** at **[UC Davis](https://www.ucdavis.edu/)** in 2018. ResilientDB advocates a **system-centric** design by adopting a **multi-threaded architecture** that encompasses **deep pipelines**. Further, ResilientDB **separates** the ordering of client transactions from their execution, which allows it to **process messages out-of-order**.

# Downloads:
Download address for run-directly software package: https://downloads.apache.org/incubator/resilientdb/

### Quick Facts on ResilientDB
1. ResilientDB orders client transactions through a highly optimized implementation of the  **[PBFT](https://pmg.csail.mit.edu/papers/osdi99.pdf)** [Castro and Liskov, 1998] protocol, which helps to achieve consensus among its replicas. ResilientDB also supports deploying other state-of-the-art consensus protocols *[release are planned]* such as **[GeoBFT](http://www.vldb.org/pvldb/vol13/p868-gupta.pdf)** [**[blog](https://blog.resilientdb.com/2023/03/07/GeoBFT.html), [released](https://github.com/resilientdb/resilientdb/releases/tag/nexres-v1.1.0)**], **[PoE](https://openproceedings.org/2021/conf/edbt/p111.pdf)**, **[RCC](https://arxiv.org/abs/1911.00837)**, **[RingBFT](https://openproceedings.org/2022/conf/edbt/paper-73.pdf)**, **[PoC](https://arxiv.org/abs/2302.02325)**, **[SpotLess](https://arxiv.org/abs/2302.02118)**, **[HotStuff](https://arxiv.org/abs/1803.05069)**, and **[DAG](https://arxiv.org/pdf/2105.11827.pdf)**.
2. ResilientDB requires deploying at least **3f+1** replicas, where **f (f > 0)** is the maximum number of arbitrary (or malicious) replicas.
3. ResilientDB supports primary-backup architecture, which designates one of the replicas as the **primary** (replica with identifier **0**). The primary replica initiates consensus on a client transaction, while backups agree to follow a non-malicious primary.
4. ResilientDB exposes a wide range of interfaces such as a **Key-Value** store, **Smart Contracts**, **UTXO**, and **Python SDK**. Following are some of the decentralized applications (DApps) built on top of ResilientDB: **[NFT Marketplace](https://nft.resilientdb.com/)** and **[Debitable](https://debitable.resilientdb.com/)**.
5. To persist blockchain, chain state, and metadata, ResilientDB provides durability through  **LevelDB**.
6. ResilientDB provides access to a seamless **GUI display** for deployment and maintenance, and supports  **Grafana** for plotting monitoring data. 
7. **[Historial Facts]** The ResilientDB project was founded by **[Mohammad Sadoghi](https://expolab.org/)** along with his students ([Suyash Gupta](https://gupta-suyash.github.io/index.html) as the lead Architect, [Sajjad Rahnama](https://sajjadrahnama.com/) as the lead System Designer, and [Jelle Hellings](https://www.jhellings.nl/)) at **[UC Davis](https://www.ucdavis.edu/)** in 2018 and was open-sourced in late 2019. On September 30, 2021, we released ResilientDB v-3.0. In 2022, ResilientDB was completely re-written and re-architected ([Junchao Chen](https://github.com/cjcchen) as the lead Architect, [Dakai Kang](https://github.com/DakaiKang) as the lead Recovery Architect along with the entire [NexRes Team](https://expolab.resilientdb.com/)), paving the way for a new sustainable foundation, referred to as NexRes (Next Generation ResilientDB). Thus, on September 30, 2022, NexRes-v1.0.0 was born, marking a new beginning for **[ResilientDB](https://resilientdb.com/)**. On October 21, 2023, **[ResilientDB](https://cwiki.apache.org/confluence/display/INCUBATOR/ResilientDBProposal)** was officially accepted into **[Apache Incubation](https://incubator.apache.org/projects/resilientdb.html)**.

<div align = "center">
<img src="./img/resdb-v2.png" width="220">
<img src="./img/apache-resdb.png" width="80">
<img src="./img/apache-incubator.png" width="250">
</div>

---


## Online Documentation:

The latest ResilientDB documentation, including a programming guide, is available on our **[blog repository](https://blog.resilientdb.com/archive.html?tag=NexRes)**. This README file provides basic setup instructions.

#### Table of Contents
1. Software Stack Architecture 
   - SDK, Interface/API, Platform, Execution, and Chain Layers 
   - Detailed API Documentation: **[Core](https://api.resilientdb.com/)** and **[SDK](https://sdk.resilientdb.com/)**
2. **SDK Layer:** **[Python SDK](https://blog.resilientdb.com/2023/02/01/UsingPythonSDK.html)** and **[Wallet - ResVault](https://blog.resilientdb.com/2023/09/21/ResVault.html)**
3. **Interface Layer:** **[Key-Value](https://blog.resilientdb.com/2022/09/28/GettingStartedNexRes.html)**, **[Solidity Smart Contract](https://blog.resilientdb.com/2023/01/15/GettingStartedSmartContract.html)**, **[Unspent Transaction Output (UTXO) Model](https://blog.resilientdb.com/2023/02/12/UtxoOnNexres.html)**, ResilientDB Database Connectivity (RDBC) API
4. **Platform Layer:** **[Consensus Manager Architecture (ordering, recovery, network, chain management)](https://blog.resilientdb.com/2022/09/27/What_Is_NexRes.html)**
   - **[Recovery & Checkpoint Design](https://blog.resilientdb.com/2023/08/22/ViewChangeInNexRes.html)**
5. **Execution Layer:** Transaction Manager Design (Runtime) 
6. **Chain Layer:** Chain State & Storage Manager Design (**[durability](https://blog.resilientdb.com/2023/02/15/NexResDurabilityLayer.html)**) 
7. **[Installing & Deploying ResilientDB](https://blog.resilientdb.com/2022/09/28/GettingStartedNexRes.html)**
   - Build Your First Application: **[KV Service](https://blog.resilientdb.com/2022/09/28/StartYourApplication.html)**, **[UTXO](https://blog.resilientdb.com/2023/02/12/GettingStartedOnUtxo.html)**
   - Dashboard: **[Monitoring](https://blog.resilientdb.com/2022/12/06/NexResGrafanaDashboardInstallation.html)**, **[Deployment](https://blog.resilientdb.com/2022/12/06/DeployGrafanaDashboardOnOracleCloud.html)**, **[Data Pipeline](https://blog.resilientdb.com/2022/12/12/NexResGrafanaDashboardPipeline.html)**
   - System Parameters & Configuration   
   - Continuous Integration & Testing 

<div align = "center">
<img src="./img/nexres.png" width="600">
</div>

## OS Requirements
Ubuntu 20+

---

## Project Structure

```
incubator-resilientdb/
├── api/                              # API layer and interfaces
├── benchmark/                        # Performance benchmarking tools
│   └── protocols/                    # Protocol-specific benchmarks
│       ├── pbft/                     # PBFT protocol benchmarks
│       └── poe/                      # PoE protocol benchmarks
├── chain/                           # Blockchain chain management
│   ├── state/                       # Chain state management
│   └── storage/                     # Storage layer (LevelDB, etc.)
├── common/                          # Common utilities and libraries
│   ├── crypto/                      # Cryptographic functions
│   ├── lru/                         # LRU cache implementation
│   ├── proto/                       # Protocol buffer definitions
│   ├── test/                        # Testing utilities
│   └── utils/                       # General utilities
├── ecosystem/                       # Ecosystem components (git subtrees)
│   ├── cache/                       # Caching implementations
│   │   ├── resilient-node-cache/    # Node.js caching
│   │   └── resilient-python-cache/  # Python caching
│   ├── deployment/                  # Deployment and infrastructure
│   │   ├── ansible/                 # Ansible playbooks
│   │   └── orbit/                   # Orbit deployment tool
│   ├── graphql/                     # GraphQL service
│   ├── monitoring/                  # Monitoring and observability
│   │   ├── reslens/                 # ResLens monitoring tool
│   │   └── reslens-middleware/      # ResLens middleware
│   ├── sdk/                         # Software Development Kits
│   │   ├── resdb-orm/               # Python ORM
│   │   ├── resvault-sdk/            # ResVault SDK
│   │   └── rust-sdk/                # Rust SDK
│   ├── smart-contract/              # Smart contract ecosystem
│   │   ├── rescontract/             # ResContract repository
│   │   ├── resilient-contract-kit/  # Contract development toolkit
│   │   └── smart-contract-graphql/  # Smart contract GraphQL service
│   └── tools/                       # Development and operational tools
│       ├── create-resilient-app/    # App scaffolding tool
│       └── resvault/                # ResVault tool
├── executor/                        # Transaction execution engine
│   ├── common/                      # Common execution utilities
│   ├── contract/                    # Smart contract execution
│   ├── kv/                          # Key-value execution
│   └── utxo/                        # UTXO execution
├── interface/                       # Client interfaces and APIs
│   ├── common/                      # Common interface utilities
│   ├── contract/                    # Smart contract interface
│   ├── kv/                          # Key-value interface
│   ├── rdbc/                        # ResilientDB Database Connectivity
│   └── utxo/                        # UTXO interface
├── monitoring/                      # Core monitoring components
├── platform/                        # Core platform components
│   ├── common/                      # Common platform utilities
│   ├── config/                      # Configuration management
│   ├── consensus/                   # Consensus protocols
│   │   ├── checkpoint/              # Checkpoint management
│   │   ├── execution/               # Transaction execution
│   │   ├── ordering/                # Transaction ordering
│   │   └── recovery/                # Recovery mechanisms
│   ├── networkstrate/               # Network strategy layer
│   ├── proto/                       # Protocol definitions
│   ├── rdbc/                        # RDBC implementation
│   └── statistic/                   # Statistics and metrics
├── proto/                           # Protocol buffer definitions
│   ├── contract/                    # Smart contract protos
│   ├── kv/                          # Key-value protos
│   └── utxo/                        # UTXO protos
├── scripts/                         # Deployment and utility scripts
│   └── deploy/                      # Deployment scripts
├── service/                         # Service implementations
│   ├── contract/                    # Smart contract service
│   ├── kv/                          # Key-value service
│   ├── tools/                       # Service tools
│   ├── utils/                       # Service utilities
│   └── utxo/                        # UTXO service
├── third_party/                     # Third-party dependencies
└── tools/                           # Development and build tools
```

**Note**: The `ecosystem/` directory contains git subtrees for ecosystem components. You can clone the repository without ecosystem components for a smaller, faster download. See [ecosystem/README.md](ecosystem/README.md) for details.

## Build and Deploy ResilientDB

Next, we show how to quickly build ResilientDB and deploy 4 replicas and 1 client proxy on your local machine. The proxy acts as an interface for all the clients. It batches client requests and forwards these batches to the replica designated as the leader. The 4 replicas participate in the PBFT consensus to order and execute these batches. Post execution, they return the response to the leader.

Install dependencies:

    ./INSTALL.sh

For non-root users, see [INSTALL/README.md](https://github.com/apache/incubator-resilientdb/blob/master/INSTALL/README.md)

Run ResilientDB (Providing a Key-Value Service):

    ./service/tools/kv/server_tools/start_kv_service.sh
    
- This script starts 4 replicas and 1 client. Each replica instantiates a key-value store.

Build Interactive Tools:

    bazel build service/tools/kv/api_tools/kv_service_tools

### Issues ###
If you cannot build the project successfully, try to reduce the bazel jobs [here](
https://github.com/apache/incubator-resilientdb/blob/master/.bazelrc#L1).

## Functions ##
ResilientDB supports two types of functions: version-based and non-version-based.
Version-based functions will leverage versions to protect each update, versions must be obtained before updating a key.

***Note***: Version-based functions are not compatible with non-version-based functions. Do not use both in your applications.

We show the functions below and show how to use [kv_service_tools](service/tools/kv/api_tools/kv_service_tools.cpp) to test the function.

### Version-Based Functions ###
#### Get ####
Obtain the value of `key` with a specific version `v`.

      kv_service_tools --config config_file --cmd get_with_version --key key --version v

|  parameters   |  descriptions |
|  ----  | ----  |
| config  | the path of the client config which points to the db entrance |
| cmd  | get_with_version |
| key  | the key you want to obtain |
| version | the version you want to obtain. (If the `v` is 0, it will return the latest version |


Example:

      bazel-bin/service/tools/kv/api_tools/kv_service_tools --config service/tools/config/interface/service.config --cmd get_with_version --key key1 --version 0

Results:
> get key = key1, value = value: "v2"
> version: 2

#### Set ####
Set `value` to the key `key` based on version `v`.

      kv_service_tools --config config_file --cmd set_with_version --key key --version v --value value

|  parameters   |  descriptions |
|  ----  | ----  |
| config  | the path of the client config which points to the db entrance |
| cmd  | set_with_version |
| key  | the key you want to set |
| version | the version you have obtained. (If the version has been changed during the update, the transaction will be ignored) |
| value | the new value |

Example:

      bazel-bin/service/tools/kv/api_tools/kv_service_tools --config service/tools/config/interface/service.config --cmd set_with_version --key key1 --version 0 --value v1

Results:
> set key = key1, value = v3, version = 2 done, ret = 0
> 
> current value = value: "v3"
> version: 3

#### Get Key History ####
Obtain the update history of key `key` within the versions [`v1`, `v2`].

      kv_service_tools --config config_file --cmd get_history --key key --min_version v1 --max_version v2


|  parameters   |  descriptions |
|  ----  | ----  |
| config  | the path of the client config which points to the db entrance |
| cmd  | get_history |
| key  | the key you want to obtain |
| min_version | the minimum version you want to obtain |
| max_version | the maximum version you want to obtain |

Example:

      bazel-bin/service/tools/kv/api_tools/kv_service_tools --config service/tools/config/interface/service.config --cmd get_history --key key1 --min_version 1 --max_version 2

Results:

> get history key = key1, min version = 1, max version = 2 <br>
>  value = <br>
> item { <br>
>  &ensp; key: "key1" <br>
>  &ensp; value_info { <br>
>  &ensp;&ensp; value: "v1" <br>
>  &ensp;&ensp; version: 2 <br>
> &ensp;} <br>
> } <br>
> item { <br>
> &ensp; key: "key1" <br>
> &ensp; value_info { <br>
> &ensp;&ensp; value: "v0" <br>
> &ensp;&ensp; version: 1 <br>
> &ensp;} <br>
> } 

#### Get Top ####
Obtain the recent `top_number` history of the key `key`.

      kv_service_tools --config config_path --cmd get_top --key key --top top_number

|  parameters   |  descriptions |
|  ----  | ----  |
| config  | the path of the client config which points to the db entrance |
| cmd  | get_top |
| key  | the key you want to obtain |
| top | the number of the recent updates |

Example:

      bazel-bin/service/tools/kv/api_tools/kv_service_tools --config service/tools/config/interface/service.config --cmd get_top --key key1 --top 1

Results:

>key = key1, top 1 <br>
> value = <br>
> item { <br>
>&ensp;key: "key1" <br>
>  &ensp;value_info { <br>
>  &ensp;&ensp;  value: "v2" <br>
>  &ensp;&ensp;  version: 3 <br>
>  &ensp;} <br>
>}

#### Get Key Range ####
Obtain the values of the keys in the ranges [`key1`, `key2`]. Do not use this function in your practice code

      kv_service_tools --config config_file --cmd get_key_range_with_version --min_key key1 --max_key key2

|  parameters   |  descriptions |
|  ----  | ----  |
| config  | the path of the client config which points to the db entrance |
| cmd  | get_key_range_with_version |
| min_key  | the minimum key  |
| max_key | the maximum key |

Example:

      bazel-bin/service/tools/kv/api_tools/kv_service_tools --config service/tools/config/interface/service.config --cmd get_key_range_with_version --min_key key1 --max_key key3

Results:

>min key = key1 max key = key2 <br>
> getrange value = <br>
> item { <br>
> &ensp; key: "key1" <br>
> &ensp; value_info { <br>
> &ensp;&ensp;   value: "v0" <br>
> &ensp;&ensp;   version: 1 <br>
> &ensp; } <br>
> } <br>
> item { <br>
> &ensp; key: "key2" <br>
> &ensp; value_info { <br>
> &ensp;&ensp;   value: "v1" <br>
> &ensp;&ensp;   version: 1 <br>
> &ensp; } <br>
>}


### Non-Version-Based Function ###
#### Set #####
Set `value` to the key `key`.

      kv_service_tools --config config_file --cmd set --key key --value value

|  parameters   |  descriptions |
|  ----  | ----  |
| config  | the path of the client config which points to the db entrance |
| cmd  | set |
| key  | the key you want to set |
| value | the new value |

Example:

      bazel-bin/service/tools/kv/api_tools/kv_service_tools --config service/tools/config/interface/service.config --cmd set --key key1 --value value1

Results:
> set key = key1, value = v1, done, ret = 0

#### Get ####
Obtain the value of `key`.

      kv_service_tools --config config_file --cmd get --key key

|  parameters   |  descriptions |
|  ----  | ----  |
| config  | the path of the client config which points to the db entrance |
| cmd  | get |
| key  | the key you want to obtain |

Example:

      bazel-bin/service/tools/kv/api_tools/kv_service_tools --config service/tools/config/interface/service.config --cmd get --key key1

Results:
> get key = key1, value = "v2"


#### Get Key Range ####
Obtain the values of the keys in the ranges [`key1`, `key2`]. Do not use this function in your practice code

      kv_service_tools --config config_path --cmd get_key_range --min_key key1 --max_key key2

|  parameters   |  descriptions |
|  ----  | ----  |
| config  | the path of the client config which points to the db entrance |
| cmd  | get_key_range |
| min_key  | the minimum key  |
| max_key | the maximum key |

Example:

      bazel-bin/service/tools/kv/api_tools/kv_service_tools --config service/tools/config/interface/service.config --cmd get_key_range --min_key key1 --max_key key3

Results:
> getrange min key = key1, max key = key3 <br>
> value = [v3,v2,v1]


## Deployment Script

We also provide access to a [deployment script](https://github.com/resilientdb/resilientdb/tree/master/scripts/deploy) that allows deployment on distinct machines.

## Deploy via Docker

1. **Install Docker**  
   Before getting started, make sure you have Docker installed on your system. If you don't have Docker already, you can download and install it from the official [Docker website](https://www.docker.com/products/docker-desktop/).

2. **Pull the Latest ResilientDB Image**  
   Choose the appropriate [ResilientDB image](https://hub.docker.com/repository/docker/expolab/resdb/general) for your machine's architecture:

   - For amd architecture, run:
     ```shell
     docker pull expolab/resdb:amd64
     ```

   - For Apple Silicon (M1/M2) architecture, run:
     ```shell
     docker pull expolab/resdb:arm64
     ```

3. **Run a Container with the Pulled Image**  
   Launch a Docker container using the ResilientDB image you just pulled:

   - For amd architecture, run:
     ```shell
     docker run -d --name myserver expolab/resdb:amd64
     ```

   - For Apple Silicon (M1/M2) architecture, run:
     ```shell
     docker run -d --name myserver expolab/resdb:arm64
     ```

4. **Test with Set and Get Commands**
   Exec into the running server:
   ```shell
   docker exec -it myserver bash
   ```

5. **NOTE: If you encounter a Connection Refused error**

   Run the following command within the container:
   ```shell
   ./service/tools/kv/server_tools/start_kv_service.sh
   ```

   Verify the functionality of the service by performing set and get operations provided above [functions](README.md#functions).


## Custom Ports ##
When starting the service locally, current services are running on 10000 port-base with 5 services where the server config is located [here](https://github.com/apache/incubator-resilientdb/blob/master/service/tools/config/server/server.config)

If you want to change the setting,  you need to generate the certificates.

Go the the workspace where the resilientdb repo is localted.

Change the setting parameters here and run the script:
  ```shell
  ./service/tools/kv/server_tools/generate_config.sh
  ```

Then re-run the start script:
  ```shell
  ./service/tools/kv/server_tools/start_kv_service.sh
  ```



## Smart Contract ##
If you want to use smart contracts, please go to:
https://blog.resilientdb.com/2025/02/14/GettingStartedSmartContract.html
