# ResilientDB: Global-Scale Sustainable Blockchain Fabric

### ResilientDB aims at *Making Permissioned Blockchain Systems Fast Again*. ResilientDB makes *system-centric* design decisions by adopting a *multi-thread architecture* that encompasses *deep-pipelines*. Further, we *separate* the ordering of client transactions from their execution, which allows us to perform *out-of-order processing of messages*.

### Quick Facts about of ResilientDB
1. **PBFT** [Castro and Liskov, 1998] protocol is used to achieve consensus among the replicas.
2. ResilientDB expects minimum **3f+1** replicas, where **f** is the maximum number of byzantine (or malicious) replicas.
3. ReslientDB designates one of its replicas as the **primary** (replicas with identifier **0**), which is also responsible for initiating the consensus.
4. ResilientDB exposes a wide range of interfaces, including Key-Value service, **Smart Contracts**, Python SDK/API.
5. To facilitate data storage and persistence, ResilientDB provides support for an **in-memory key-value store**. Further, it provides durability through  **LevelDB** and **RocksDB**.
6. With ResilientDB we also provide a seamless **GUI display**. This display generates a status log and also accesses **Grafana to plot the results**. 

---


## Online Documentation:

You can find the latest ResilientDB documentation, including a programming guide, on the **[project web blog](https://blog.resilientdb.com/archive.html?tag=NexRes)**. This README file only contains basic setup instructions.

## OS Requirements
Ubuntu 20.*

---

## Steps to ResilientDB

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

    bazel-bin/example/kv_server_ools example/kv_client_config.config get test
    
You will see the following result if successful:

    client get value = test_value

Run tools to get all values that have been set:

    bazel-bin/example/kv_server_tools example/kv_client_config.config getvalues

You will see the following result if successful:

    client getvalues value = [test_value]



## FAQ

If you fail to install bazel on your ubuntu server, follow these steps:

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
