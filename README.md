# Nexres: A High-throughput yielding Permissioned Blockchain Fabric.

 Nexres aims at *Making Permissioned Blockchain Systems Fast Again*. Nexres makes *system-centric* design decisions by adopting a *multi-thread architecture* that encompasses *deep-pipelines*. Further, we *separate* the ordering of client transactions from their execution, which allows us to perform *out-of-order processing of messages*.

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
    docker run -i -t --name nexres nexres "/bin/bash"
    exit
	
	
Once the docker images starts, get your image id:

	docker ps -a

Start the docker images and login (fcbe927b4242 is the image id I got from the last command):

	docker start fcbe927b4242
	docker exec -it fcbe927b4242 /bin/bash

Change to user to ubuntu and clone the repo. **(Please fork your own one)**

	su - ubuntu
	git clone https://github.com/resilientdb/resilientdb.git
	
Run the install script:

	cd resilientdb/
	sh INSTALL_MAC.sh

Start the example server and run the client commands:

	sh example/start_kv_server.sh
	bazel build example/kv_server_tools
	bazel-bin/example/kv_server_tools example/kv_client_config.config get test
