<!-- #
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#  http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.
#  -->

# Change Log

### Apache ResilientDB v1.11.0 ([2025-4-27](https://github.com/resilientdb/resilientdb/releases/tag/v1.11.0))
* Support Combined Runtime for SmartContact and Key-value Service. ([Junchao Chen](https://github.com/cjcchen))


### Apache ResilientDB v1.10.1 ([2024-4-16](https://github.com/resilientdb/resilientdb/releases/tag/v1.10.1))
* Remove the binary keys from the source code. ([Junchao Chen](https://github.com/cjcchen))
* Fix few bugs on Docker and performance tools. ([Gopal Nambiar](gopalnambiar2@gmail.com), [Junchao Chen](https://github.com/cjcchen))

### Apache ResilientDB v1.10.0 ([2024-4-16](https://github.com/resilientdb/resilientdb/releases/tag/v1.10.0-rc01))

Add the prototype of PoE. ([Junchao Chen](https://github.com/cjcchen))

* Implement the base version of the Proof-of-Execution (PoE) Consensus Protocol [EDBT 2011].

Add ResView Data Collection and APIs ([Saipranav-Kotamreddy](https://github.com/Saipranav-Kotamreddy))

* Consensus data such as PBFT messages and states is now collected and stored
* Added APIs to query consensus data and progress of replicas
* Added APIs to trigger faultiness and test view change

### NexRes v1.9.0 ([2023-11-29](https://github.com/resilientdb/resilientdb/releases/tag/nexres-v1.9.0))

Support Multi-version Key-Value Interface. ([Junchao Chen](https://github.com/cjcchen))

* Get and Set need to provide a version number to fetch the correct version of the data (if exists) or write to the correct version of data (if not overwritten already), respectively.
* Provide interfaces to obtain historical data with a specific version or a range of versions.


### NexRes v1.8.0 ([2023-08-21](https://github.com/resilientdb/resilientdb/releases/tag/nexres-v1.8.0))

**Implemented Enhancements:** The view-change recovery protocol was extensively expanded to support the following Byzantine failures through primary/leader replacement and replica recovery. ([Dakai Kang](https://github.com/DakaiKang))

* Byzantine primary becomes non-responsive, stopping proposing any new Pre-Prepare messages.
* Byzantine primary equivocates, proposing two different client requests to two subsets of replicas.
* Byzantine replicas try to keep some non-faulty replicas in the dark.
* Byzantine new primary becomes non-responsive, refusing to broadcast New-View messages.

### NexRes v1.7.0 ([2023-08-04](https://github.com/resilientdb/resilientdb/releases/tag/nexres-v1.7.0))

**Implemented Enhancements:** For each replica local recovery from durable storage upon system restart was added.([Junchao Chen](https://github.com/cjcchen))


### NexRes v1.6.0 ([2023-05-30](https://github.com/resilientdb/resilientdb/releases/tag/nexres-v1.6.0))

**Implemented Enhancements:** Refactoring and enhancement of the codebase to highlight the entire software stack of ResilientDB consisting of the following layers ([Junchao Chen](https://github.com/cjcchen))

* SDK Layer (C++, Python, Go, Solidity)
* Interface Layer (key-value, smart contract, UTXO)
* ResilientDB Database Connectivity (RDBC) API
* Platform Layer (consensus, chain, network, notary)
* Transaction Layer (execution runtime and in-memory chain state)
* Storage Layer (chain and chain state durability)


### NexRes v1.5.0 ([2023-04-04](https://github.com/resilientdb/resilientdb/releases/tag/nexres-v1.5.0))

**Implemented Enhancements:** 
* A complete refactoring of the codebase such that (1) the core engine code is now moved to the platform folder, including the formwork architectures and protocols implementations; (2) the API/interface-related code is moved to the service folder, including UTXO, smart contract, and key-value interface;  (3) the python SDK code is moved to a different repository [here](https://github.com/resilientdb/sdk) ([Junchao Chen](https://github.com/cjcchen)).

### NexRes v1.4.0 ([2023-02-28](https://github.com/resilientdb/resilientdb/releases/tag/nexres-v.1.4.0))
Major Changes
* Support of UTXO model and wallet integration: [Detailed Documentation](https://blog.resilientdb.com/2023/02/12/GettingStartedOnUtxo.html) ([Junchao Chen](https://github.com/cjcchen))


### NexRes v1.3.0 ([2023-02-22](https://github.com/resilientdb/resilientdb/releases/tag/nexres-v1.3.0))

**Implemented Enhancements:** 
* Added Python SDK that supports UTXO-like transactions such as asset creation, transfer, and multi-party validations. ([Arindaam Roy](https://github.com/royari), [Glenn Chen](https://github.com/glenn-chen), and [Julieta Duarte](https://github.com/juduarte00))
* Added REST CROW endpoints to interface with the on-chain KV service. ([Glenn Chen](https://github.com/glenn-chen) and [Julieta Duarte](https://github.com/juduarte00))
* Extended KV service range queries. ([Glenn Chen](https://github.com/glenn-chen) and [Julieta Duarte](https://github.com/juduarte00))

**Fixed Bugs:**
* Fixed KV service queries on all values retrieving empty placeholder values ([Glenn Chen](https://github.com/glenn-chen))

### NexRes v1.2.0 ([2023-01-29](https://github.com/resilientdb/resilientdb/releases/tag/nexres-v1.2.0))

**Implemented Enhancements:** 
* Support smart contract compiled from Solidity. ([Junchao Chen](https://github.com/cjcchen)) 
* Use eEVM as a back-end service to execute the contract functions. ([Junchao Chen](https://github.com/cjcchen))

### NexRes v1.1.0 ([2023-01-03](https://github.com/resilientdb/resilientdb/releases/tag/nexres-v1.1.0))

**Implemented Enhancements:** 

* Added Geo-Scale Byzantine Fault-tolerant consensus protocol, referred to as GeoBFT. It is designed for excellent scalability by using a topological-aware grouping of replicas in local clusters, giving rise to parallelization of consensus at the local level and by minimizing communication between clusters. ([WayneWang](https://github.com/WayneJa))

ResilientDB: Global Scale Resilient Blockchain Fabric, VLDB 2020

### NexRes v1.0.1 ([2022-10-13](https://github.com/resilientdb/resilientdb/releases/tag/nexres-v1.0.1))

**Implemented Enhancements:** 

* Add node manager backend for resilientdb.com to launch NexRes locally. ([Vishnu](https://github.com/sheshavpd) and [Junchao Chen](https://github.com/cjcchen))

### NexRes v1.0.0 ([2022-09-30](https://github.com/resilientdb/resilientdb/releases/tag/nexres-v1.0.0))

**Implemented Enhancements:** 

* Release the next generation of RelisientDB referred to as NexRes. ([Junchao Chen](https://github.com/cjcchen))
* NexRes supports PBFT as the default core protocol and provides a KV-Server as a service. ([Junchao Chen](https://github.com/cjcchen))
* NexRes also supports LevelDB and RocksDB as the durable storage layer. ([Junchao Chen](https://github.com/cjcchen))
* Add SDK endpoints. ([Junchao Chen](https://github.com/cjcchen))
* Add implementation of Dashboard. ([Jianxio](https://github.com/jyu25utk))
* Add the recovery protocol and checkpoint on PBFT. ([Junchao Chen](https://github.com/cjcchen) and [Vishnu](https://github.com/sheshavpd))


### NexRes v1.0.0-alpha ([2022-09-22](https://github.com/resilientdb/resilientdb/releases/tag/nexres-alpha))

**Implemented Enhancements:** 
* Release the next generation of RelisientDB, a complete rewrite and re-architecting, referred to as NexRes. ([Junchao Chen](https://github.com/cjcchen))
* NexRes supports PBFT as the default core protocol and provides a KV-Server as a service. ([Junchao Chen](https://github.com/cjcchen))
* NexRes also supports LevelDB and RocksDB as the durable storage layer. ([Glenn Chen](https://github.com/glenn-chen), [Julieta Duarte](https://github.com/juduarte00), and [Junchao Chen](https://github.com/cjcchen))


### v3.0 ([2021-09-30](https://github.com/resilientdb/resilientdb/releases/tag/v3.0))

**Implemented Enhancements:** 
* GeoBFT Protocol Added ([Sajjad Rahnama](https://github.com/sajjadrahnama))
* Work Queue structure changed ([Sajjad Rahnama](https://github.com/sajjadrahnama))
* Refactoring, and added Statistics and Scripts ([Sajjad Rahnama](https://github.com/sajjadrahnama))


### v2.0 ([2020-02-29](https://github.com/resilientdb/resilientdb/releases/tag/v2.0))

**Implemented Enhancements:** 
* A GUI display for ResilientDB to ease user interaction and analysis. [#9](https://github.com/resilientdb/resilientdb/issues/9) ([Sajjad Rahnama](https://github.com/sajjadrahnama) and [RohanSogani](https://github.com/RohanSogani))
* Added support for Smart Contracts with a Banking use case. [#7](https://github.com/resilientdb/resilientdb/issues/7) ([Sajjad Rahnama](https://github.com/sajjadrahnama))
* Monitoring results for visualizing graphs using influxdb. [#8](https://github.com/resilientdb/resilientdb/issues/8) ([Sajjad Rahnama](https://github.com/sajjadrahnama), [Dhruv Krishnan](https://github.com/DhruvKrish) and [Priya Holani](https://github.com/Holani))
* Added support for SQLite and provided a new representation for in-memory storage. [#6](https://github.com/resilientdb/resilientdb/issues/6) ([Sajjad Rahnama](https://github.com/sajjadrahnama))

**Fixed Bugs:**
* Bug in the Chain Initialization [#5](https://github.com/resilientdb/resilientdb/issues/5) ([Sajjad Rahnama](https://github.com/sajjadrahnama))


### v1.1 ([2019-12-05](https://github.com/resilientdb/resilientdb/releases/tag/v1.1)) 

**Implemented Enhancements:**
* Defined class to represent ledger. [6b0fdc5](https://github.com/resilientdb/resilientdb/commit/56f500fe5e4749c45f57dc8e62d12bc7a218ce69) ([Suyash Gupta](https://github.com/gupta-suyash))

**Fixed Bugs:**
* Docker script fails to run for multiple client [#1](https://github.com/resilientdb/resilientdb/issues/1) ([RohanSogani](https://github.com/RohanSogani))	

### v1.0 ([2019-11-24](https://github.com/resilientdb/resilientdb/releases/tag/v1.0)) 

Launched ResilientDB ([Suyash Gupta](https://github.com/gupta-suyash) and [Sajjad Rahnama](https://github.com/sajjadrahnama))


