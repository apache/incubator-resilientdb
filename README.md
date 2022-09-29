# Nexres: A High-throughput yielding Permissioned Blockchain Fabric.

Nexres aims at *Making Permissioned Blockchain Systems Fast Again*. Nexres makes *system-centric* design decisions by adopting a *multi-thread architecture* that encompasses *deep-pipelines*. Further, we *separate* the ordering of client transactions from their execution, which allows us to perform *out-of-order processing of messages*.
 

### Release Notes Version alpha of Nexres
1. **PBFT** [Castro and Liskov, 1998] protocol is used to achieve consensus among the replicas.
2. Nexres expects minimum **3f+1** replicas, where **f** is the maximum number of byzantine (or malicious) replicas.
3. Nexres uses coroutine as its network infrastructure. 
4. The main implementation of Nexres including the network and pbft protocol are lock free by using boost::lockfreequeue.
5. Nexres designates one of its replicas as the **primary**, which is also responsible for initiating the consensus.
6. To facilitate data storage and persistence, Nexres provides support for an **in-memory key-value store**, Rocksdb and LevelDB and expose SDKs to store key-value pairs.
7. We also provide prometheus metrics to display the performance and latency.
8. We support docker environment with ubuntu for **MacOS M1** and source code for native ubuntu. Nexres can only run on **ubuntu 20.04**.



## Docs
[Nexres Introduction](https://docs.google.com/presentation/d/1QurZA4w_PTxAtb_5FpuXJhsQVAIA2t5kvx1X2ETIl7w/edit#slide=id.p)

---
## Steps to Run KVServer

Install dependences.

    sh INSTALL.sh


Start local KVServers:

    sh example/start_kv_server.sh
- This script will start 4 local kv servers and 1 local client proxy. The client proxy is the proxy transferring the messages between servers and the user client.

Build KVServer Tools:

    bazel build example/kv_server_tools
    
Run tools to get value by key(for example, get the value with key "test"):

    bazel-bin/example/kv_server_tools example/kv_client_config.config get test
    
You will see this if success:

    client get value = xxx

Run tools to set value by key(for example, set the value with key "test" and value "test_value"):

    bazel-bin/example/kv_server_tools example/kv_client_config.config set test test_value
    
You will see this if success:

    client set ret = 0

## Run on MacOS

Install docker: See more [here](https://docs.docker.com/desktop/mac/apple-silicon/)

&ensp;  &ensp;  Download the [binary](https://desktop.docker.com/mac/main/arm64/Docker.dmg?utm_source=docker&utm_medium=webreferral&utm_campaign=docs-driven-download-mac-arm64)

Open your terminal and install requirements:
    
    softwareupdate --install-rosetta

Build a docker image if first time and launch the docker image (you can change the name (nexres) as you want):

    cd docker
    docker build . -f DockerfileForMac -t=nexres
    docker run -it --name nexres nexres /bin/bash
    exit
	
	
Once the docker images starts, get your image id:

	docker ps -a

Start the docker images and login (fcbe927b4242 is the image id I got from the last command):

	docker start fcbe927b4242
	docker exec -it fcbe927b4242 /bin/bash

Change to user to ubuntu and clone the repo. **(Please fork your own one)**

	su - ubuntu
	git clone https://github.com/resilientdb/resilientdb.git
	git checkout -b nexres remotes/origin/nexres
	
Run the install script:

	cd resilientdb/
	sh INSTALL_MAC.sh

Start the example server and run the client commands:

	sh example/start_kv_server.sh
	bazel build example/kv_server_tools
	bazel-bin/example/kv_server_tools example/kv_client_config.config get test
	
	
	
---


### Key Parameters of "resdb_config.h" 
<pre>

  * max_process_txn                           Water mark that number of replicas inflight, default = 2048;
  * client_batch_num                          Batch size of the client transactions. Before sending to the primary replica, we will collect client_batch_num of transactions from all the users. default = 100;
  * is_enable_checkpoint                      Enable checkpoint which will create stable checkpoint periodically. 
  * viewchange_commit_timeout_ms              The commitment timeout which will trigger viewchange default = 60s. Only for is_enable_checkpoint is true.
  * worker_num                                The number of worker threads to address the requests, default = 96.
  * input_worker_num                          The number of coroutine workers to handler incomming messages from the network, default = 1.
  * output_worker_num                         The number of coroutine workers to handler outcomming messages to the network, default = 1.
  
  All the parameters will be migrated into ResConfigData.proto in the future.
</pre>

### Key Parameters of "ResConfigData.proto"
<pre>
  * self_region_id                            The region id of the replica, default is 0, all the replicas will run in the same region.
  * region                                    A list of regions containing all the information of the replicas in different regions.
  * enable_viewchange                         If true, will trigger viewchange check per "viewchange_commit_timeout_ms".
  </pre>
