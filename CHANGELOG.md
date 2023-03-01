# Change Log

Major Changes
Support of UTXO model and wallet integration: Detailed Documentation

### NexRes v1.4.0 ([2023-02-28](https://github.com/resilientdb/resilientdb/releases/tag/nexres-v.1.4.0))

**Implemented Enhancements:** 
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


