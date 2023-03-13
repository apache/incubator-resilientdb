# ResilientDB: Global-Scale Sustainable Blockchain Fabric

**[ResilientDB](https://resilientdb.com/)** is a **High Throughput Yielding Permissioned Blockchain Fabric** founded by **[ExpoLab](https://expolab.org/)** at **[UC Davis](https://www.ucdavis.edu/)** in 2018. ResilientDB advocates a **system-centric** design by adopting a **multi-threaded architecture** that encompasses **deep pipelines**. Further, ResilientDB **separates** the ordering of client transactions from their execution, which allows it to **process messages out-of-order**.

### Quick Facts on ResilientDB
1. ResilientDB's core consensus protocol is based on a highly optimized **[PBFT](https://pmg.csail.mit.edu/papers/osdi99.pdf)** [Castro and Liskov, 1998] implementation to achieve agreement among replicas. The core consensus layer is further expanded with the state-of-the-art consensus protocols *[release are planned]* such as **[GeoBFT](http://www.vldb.org/pvldb/vol13/p868-gupta.pdf)** [**[blog](https://blog.resilientdb.com/2023/03/07/GeoBFT.html), [released](https://github.com/resilientdb/resilientdb/releases/tag/nexres-v1.1.0)**], **[PoE](https://openproceedings.org/2021/conf/edbt/p111.pdf)**, **[RCC](https://arxiv.org/abs/1911.00837)**, **[RingBFT](https://openproceedings.org/2022/conf/edbt/paper-73.pdf)**, **[PVP](https://arxiv.org/abs/2302.02325)**, **[PoC](https://arxiv.org/abs/2302.02118)**, **[HotStuff](https://arxiv.org/abs/1803.05069)**, and **[DAG](https://arxiv.org/pdf/2105.11827.pdf)**.
2. ResilientDB expects minimum of **3f+1** replicas, where **f** is the maximum number of arbitrary (or malicious) replicas.
3. ReslientDB designates one of its replicas as the **primary** (replicas with identifier **0**), which is also responsible for initiating the consensus.
4. ResilientDB exposes a wide range of API services such as a **Key-Value**, **Smart Contracts**, **UTXO**, and **Python SDK**. Examples DApp that are being built on ResilientDB are: **[NFT Marketplace](https://nft.resilientdb.com/)** and **[Debitable](https://debitable.resilientdb.com/)**.
5. To facilitate the persistence of the chain and chain state, ResilientDB provides durability through  **LevelDB** and **RocksDB**.
6. To support deployment and maintenance, ResilientDB provides access to a seamless **GUI display** along with access to **Grafana** for plotting monitoring data. 
7. **[Historial Facts]** The ResilientDB project was founded by **[Mohammad Sadoghi](https://expolab.org/)** along with his students ([Suyash Gupta](https://gupta-suyash.github.io/index.html) as the lead Architect, [Sajjad Rahnama](https://sajjadrahnama.com/), [Jelle Hellings](https://www.jhellings.nl/)) at **[UC Davis](https://www.ucdavis.edu/)** in 2018 and was open-sourced in late 2019. On September 30, 2021, we released ResilientDB v-3.0. In 2022, ResilientDB was completely re-written and re-architected ([Junchao Chen](https://github.com/cjcchen) as the lead Architect along with the entire [NexRes Team](https://resilientdb.com/)), paving the way for a new sustainable foundation, referred to as NexRes (Next Generation ResilientDB); thus, on September 30, 2022, NexRes-v1.0.0 was born, marking a new beginning for **[ResilientDB](https://resilientdb.com/)**.

---


## Online Documentation:

You may find the latest ResilientDB documentation, including a programming guide, on our **[blog repository](https://blog.resilientdb.com/archive.html?tag=NexRes)**. This README file provides basic setup instructions.

#### Table of Contents
1. Software Stack Architecture 
   - Platform, Service, and Tooling/API Layers [TBA]
   - **[Detailed API Documentation](https://api.resilientdb.com/)**
   - Overview of Class Diagram & Code Structure  [TBA]
3. **Platform Layer:** **[Consensus Manager Architecture (ordering, recovery, network, chain management, storage)](https://blog.resilientdb.com/2022/09/27/What_Is_NexRes.html)**
   - Recovery & Checkpoint Design [TBA]
   - **[Durability Design](https://blog.resilientdb.com/2023/02/15/NexResDurabilityLayer.html)**
4. **Service Layer:** Transaction Manager Design (runtime and chain state) [TBA]
5. **Tooling & API Layer:** **[Key-Value](https://blog.resilientdb.com/2022/09/28/GettingStartedNexRes.html)**, **[Solidity Smart Contract](https://blog.resilientdb.com/2023/01/15/GettingStartedSmartContract.html)**, **[Unspent Transaction Output (UTXO) Model](https://blog.resilientdb.com/2023/02/12/UtxoOnNexres.html)**, **[Python SDK](https://blog.resilientdb.com/2023/02/01/UsingPythonSDK.html)**
6. **[Installing & Deploying ResilientDB](https://blog.resilientdb.com/2022/09/28/GettingStartedNexRes.html)**
   - Build Your First Application: **[KV Service](https://blog.resilientdb.com/2022/09/28/StartYourApplication.html)**, **[UTXO](https://blog.resilientdb.com/2023/02/12/GettingStartedOnUtxo.html)**
   - Dashboard: **[Monitoring](https://blog.resilientdb.com/2022/12/06/NexResGrafanaDashboardInstallation.html)**, **[Deployment](https://blog.resilientdb.com/2022/12/06/DeployGrafanaDashboardOnOracleCloud.html)**, **[Data Pipeline](https://blog.resilientdb.com/2022/12/12/NexResGrafanaDashboardPipeline.html)**
   - System Parameters & Configuration  [TBA] 
   - Continuous Integration & Testing [TBA]

## OS Requirements
Ubuntu 20.*

---

## Build and Deploy ResilientDB

Install dependencies:

    ./INSTALL.sh


Run ResilientDB (Providing a Key-Value Service):

    ./example/start_kv_server.sh
    
- This script will start 4 replica and 1 client. Each replica instantiates a key-value store.

Build Interactive Tools:

    bazel build example/kv_server_tools

Run tools to set a value by a key (for example, set the value with key "test" and value "test_value"):

    bazel-bin/example/kv_server_tools example/kv_client_config.config set test test_value
    
You will see the following result if successful:

    client set ret = 0

Run tools to get value by a key (for example, get the value with key "test"):

    bazel-bin/example/kv_server_tools example/kv_client_config.config get test
    
You will see the following result if successful:

    client get value = test_value

Run tools to get all values that have been set:

    bazel-bin/example/kv_server_tools example/kv_client_config.config getvalues

You will see the following result if successful:

    client getvalues value = [test_value]

## Enable SDK

If you want to enable sdk verification, add a flag to the command

    ./example/start_kv_server.sh --define enable_sdk=True

And in example/kv_config.config

    require_txn_validation:true,

## FAQ

If installing bazel fails on a ubuntu server, follow these steps:

> mkdir bazel-src
>
> cd bazel-src
>
> wget https://github.com/bazelbuild/bazel/releases/download/5.4.0/bazel-5.4.0-dist.zip
>
> sudo apt-get install build-essential openjdk-11-jdk python zip unzip
>
> unzip bazel-5.4.0-dist.zip
>
> env EXTRA_BAZEL_ARGS="--tool_java_runtime_version=local_jdk" bash ./compile.sh
>
> sudo mv output/bazel /usr/local/bin
