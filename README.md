![](https://img.shields.io/badge/language-c++-orange.svg)
![](https://img.shields.io/badge/platform-Ubuntu20.0+-lightgrey.svg)
![GitHub](https://img.shields.io/github/license/resilientdb/resilientdb)



# ResilientDB: Global-Scale Sustainable Blockchain Fabric

**[ResilientDB](https://resilientdb.com/)** is a **High Throughput Yielding Permissioned Blockchain Fabric** founded by **[ExpoLab](https://expolab.org/)** at **[UC Davis](https://www.ucdavis.edu/)** in 2018. ResilientDB advocates a **system-centric** design by adopting a **multi-threaded architecture** that encompasses **deep pipelines**. Further, ResilientDB **separates** the ordering of client transactions from their execution, which allows it to **process messages out-of-order**.

### Quick Facts on ResilientDB
1. ResilientDB orders client transactions through a highly optimized implementation of the  **[PBFT](https://pmg.csail.mit.edu/papers/osdi99.pdf)** [Castro and Liskov, 1998] protocol, which helps to achieve consensus among its replicas. ResilientDB also supports deploying other state-of-the-art consensus protocols *[release are planned]* such as **[GeoBFT](http://www.vldb.org/pvldb/vol13/p868-gupta.pdf)** [**[blog](https://blog.resilientdb.com/2023/03/07/GeoBFT.html), [released](https://github.com/resilientdb/resilientdb/releases/tag/nexres-v1.1.0)**], **[PoE](https://openproceedings.org/2021/conf/edbt/p111.pdf)**, **[RCC](https://arxiv.org/abs/1911.00837)**, **[RingBFT](https://openproceedings.org/2022/conf/edbt/paper-73.pdf)**, **[PoC](https://arxiv.org/abs/2302.02325)**, **[SpotLess](https://arxiv.org/abs/2302.02118)**, **[HotStuff](https://arxiv.org/abs/1803.05069)**, and **[DAG](https://arxiv.org/pdf/2105.11827.pdf)**.
2. ResilientDB requires deploying at least **3f+1** replicas, where **f (f > 0)** is the maximum number of arbitrary (or malicious) replicas.
3. ResilientDB supports primary-backup architecture, which designates one of the replicas as the **primary** (replica with identifier **0**). The primary replica initiates consensus on a client transaction, while backups agree to follow a non-malicious primary.
4. ResilientDB exposes a wide range of interfaces such as a **Key-Value** store, **Smart Contracts**, **UTXO**, and **Python SDK**. Following are some of the decentralized applications (DApps) built on top of ResilientDB: **[NFT Marketplace](https://nft.resilientdb.com/)** and **[Debitable](https://debitable.resilientdb.com/)**.
5. To persist blockchain, chain state, and metadata, ResilientDB provides durability through  **LevelDB** and **RocksDB**.
6. ResilientDB provides access to a seamless **GUI display** for deployment and maintenance, and supports  **Grafana** for plotting monitoring data. 
7. **[Historial Facts]** The ResilientDB project was founded by **[Mohammad Sadoghi](https://expolab.org/)** along with his students ([Suyash Gupta](https://gupta-suyash.github.io/index.html) as the lead Architect, [Sajjad Rahnama](https://sajjadrahnama.com/), [Jelle Hellings](https://www.jhellings.nl/)) at **[UC Davis](https://www.ucdavis.edu/)** in 2018 and was open-sourced in late 2019. On September 30, 2021, we released ResilientDB v-3.0. In 2022, ResilientDB was completely re-written and re-architected ([Junchao Chen](https://github.com/cjcchen) as the lead Architect along with the entire [NexRes Team](https://resilientdb.com/)), paving the way for a new sustainable foundation, referred to as NexRes (Next Generation ResilientDB). Thus, on September 30, 2022, NexRes-v1.0.0 was born, marking a new beginning for **[ResilientDB](https://resilientdb.com/)**.

---


## Online Documentation:

The latest ResilientDB documentation, including a programming guide, is available on our **[blog repository](https://blog.resilientdb.com/archive.html?tag=NexRes)**. This README file provides basic setup instructions.

#### Table of Contents
1. Software Stack Architecture 
   - SDK, Interface/API, Platform, Execution, and Chain Layers 
   - Detailed API Documentation: **[Core](https://api.resilientdb.com/)** and **[SDK](https://sdk.resilientdb.com/)**
2. **SDK Layer:** **[Python SDK](https://blog.resilientdb.com/2023/02/01/UsingPythonSDK.html)**
3. **Interface Layer:** **[Key-Value](https://blog.resilientdb.com/2022/09/28/GettingStartedNexRes.html)**, **[Solidity Smart Contract](https://blog.resilientdb.com/2023/01/15/GettingStartedSmartContract.html)**, **[Unspent Transaction Output (UTXO) Model](https://blog.resilientdb.com/2023/02/12/UtxoOnNexres.html)**, ResilientDB Database Connectivity (RDBC) API
4. **Platform Layer:** **[Consensus Manager Architecture (ordering, recovery, network, chain management)](https://blog.resilientdb.com/2022/09/27/What_Is_NexRes.html)**
   - Recovery & Checkpoint Design 
5. **Execution Layer:** Transaction Manager Design (Runtime) 
6. **Chain Layer:** Chain State & Storage Manager Design (**[durability](https://blog.resilientdb.com/2023/02/15/NexResDurabilityLayer.html)**) 
7. **[Installing & Deploying ResilientDB](https://blog.resilientdb.com/2022/09/28/GettingStartedNexRes.html)**
   - Build Your First Application: **[KV Service](https://blog.resilientdb.com/2022/09/28/StartYourApplication.html)**, **[UTXO](https://blog.resilientdb.com/2023/02/12/GettingStartedOnUtxo.html)**
   - Dashboard: **[Monitoring](https://blog.resilientdb.com/2022/12/06/NexResGrafanaDashboardInstallation.html)**, **[Deployment](https://blog.resilientdb.com/2022/12/06/DeployGrafanaDashboardOnOracleCloud.html)**, **[Data Pipeline](https://blog.resilientdb.com/2022/12/12/NexResGrafanaDashboardPipeline.html)**
   - System Parameters & Configuration   
   - Continuous Integration & Testing 

![Nexres](./img/nexres.png)

## OS Requirements
Ubuntu 20.*

---

## Build and Deploy ResilientDB

Install dependencies:

    ./INSTALL.sh


Run ResilientDB (Providing a Key-Value Service):

    ./service/tools/kv/server_tools/start_kv_service.sh
    
- This script will start 4 replica and 1 client. Each replica instantiates a key-value store.

Build Interactive Tools:

    bazel build service/tools/kv/api_tools/kv_service_tools

Run tools to set a value by a key (for example, set the value with key "test" and value "test_value"):

    bazel-bin/service/tools/kv/api_tools/kv_service_tools service/tools/config/interface/service.config set test test_value
    
You will see the following result if successful:

    client set ret = 0

Run tools to get value by a key (for example, get the value with key "test"):

    bazel-bin/service/tools/kv/api_tools/kv_service_tools service/tools/config/interface/service.config get test 
    
You will see the following result if successful:

    client get value = test_value

Run tools to get all values that have been set:

    bazel-bin/service/tools/kv/api_tools/kv_service_tools service/tools/config/interface/service.config getvalues

You will see the following result if successful:

    client getvalues value = [test_value]


