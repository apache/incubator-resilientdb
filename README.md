# ResilientDB: A High-throughput yielding Permissioned Blockchain Fabric.

 ResilientDB aims at *Making Permissioned Blockchain Systems Fast Again*. ResilientDB makes *system-centric* design decisions by adopting a *multi-thread architecture* that encompasses *deep-pipelines*. Further, we *separate* the ordering of client transactions from their execution, which allows us to perform *out-of-order processing of messages*.


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

Build a docker image if first time and run the image
	cd docker
	docker build . -f DockerfileForMac -t=nexres
	docker run -i -t --name nexres nexres "/bin/bash"
	exit
Start the image if the second time:
	get your image id by:
		docker ps -a
		docker start fcbe927b4242
		docker exec -it fcbe927b4242 /bin/bash

Change to user to ubuntu
su - ubuntu
git clone https://github.com/resilientdb/resilientdb.git
cd resilientdb/
sh INSTALL_MAC.sh
sh example/start_kv_server.sh
