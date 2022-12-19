# ResilientDB: A High-throughput yielding Permissioned Blockchain Fabric.

 ResilientDB aims at *Making Permissioned Blockchain Systems Fast Again*. ResilientDB makes *system-centric* design decisions by adopting a *multi-thread architecture* that encompasses *deep-pipelines*. Further, we *separate* the ordering of client transactions from their execution, which allows us to perform *out-of-order processing of messages*.


## System Design:

High Level Design: 

[ResDBServer](https://docs.google.com/presentation/d/1i5sKocV4LQrngwNVLTTLRtshVIKICt3_tqMH4e5QgYQ/edit#slide=id.p)

[ConsensusService-PBFT](https://docs.google.com/presentation/d/1HjXVlCGbjkSzs6d7o4bT_wT-cllSCx1RkvVUskTaZJA/edit#slide=id.p)

[Full Design Doc](https://docs.google.com/document/d/1YA-vIMhSUnq6necRPY3t3thh4Zc2OuP9_GUwwuzSo-w/edit#)

## User Development guide
https://docs.google.com/presentation/d/1YIX6dG6cuc5EhXdytrJXoxMhGLP2BxLCmnVMhiC4WBc/edit#slide=id.p

---

## Steps to Run KVServer

Install dependences.

    sh INSTALL


Start local KVServers:

    sh example/start_kv_server.sh
- This script will start 4 local kv servers and 1 local client proxy. The client proxy is the proxy transferring the messages between servers and the user client.

Build KVServer Toos:

    bazel build example/kv_server_tools
    
Run tools to get value by key(for example, get the value with key "test"):

    bazel-bin/example/kv_server_tools example/kv_client_config.config get test
    
You will see this if success:

    client get value = xxx

Run tools to set value by key(for example, set the value with key "test" and value "test_value"):

    bazel-bin/example/kv_server_tools example/kv_client_config.config set test test_value
    
You will see this if success:

    client set ret = 0

## Steps to Run KVServer on Multiple Machines

We illustrate the steps with an example of running PBFT KVservers on 4 replicas and 2 clients

First, copy the IP addresses of the machines to ./oracle_script/iplist.txt

    172.31.14.56
    172.31.5.62
    172.31.8.207
    172.31.12.105
    172.31.3.13
    172.31.7.148

set the number of client on ./oracle_script/generate_config.sh #L12

    CLIENT_NUM=2

Build the tools for generating config

    bazel build //tools:certificate_tools

Generate svr_list.txt, cli_list.txt and certificates and then copy them into ./oracle_scripts/pbft/rep_4 (./oracle_scripts/{protocol}/rep_{replica_num}).

    cd oracle_script
    ./generate_config.sh
    cp ./iplist.txt ./pbft/rep_4/iplist.txt
    cp ./svr_list.txt ./pbft/rep_4/svr_list.txt
    cp ./cli_list.txt ./pbft/rep_4/cli_list.txt
    cp -r ./cert ./pbft/rep_4/

Generate configuration and executable file

    cd pbft/rep_4
    sh ./run_svr.sh
    sh ./run_cli.sh

In ./oracle_script/pbft/rep_4/killall.sh, substistue the ssh private key file with the private key file for your machines. 

For example, Dakai uses dakai_dev.pem for his machines. Then Dakai replace '.ssh/ssh-2022-03-24.key' with '.ssh/dakai_dev.pem', which has been copied to his host machine in advance.

Deploy the configuration and executable file to the machines, and run the KVServers.

    ./killall.sh

Open another terminal to make the clients start working.

   ubuntu@hostmachine:~/nexres$ ./bazel-bin/example/kv_server_tools oracle_script/pbft/rep_4/client.config get key

Open another terminal to monitor server log.

    ssh -i ～/.ssh/dakai_dev.pem ubuntu@172.31.14.56
    tail -f server1.log

Restart all KVServers after recording the performance numbers to prevent memory overflow in clients.

    ./killall.sh
    
    
