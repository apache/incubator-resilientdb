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
