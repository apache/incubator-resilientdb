# ResilientDB: A High-throughput yielding Permissioned Blockchain Fabric.

### ResilientDB aims at *Making Permissioned Blockchain Systems Fast Again*. ResilientDB makes *system-centric* design decisions by adopting a *multi-thread architecture* that encompasses *deep-pipelines*. Further, we *separate* ordering of client transactions from their execution, which allows us to perform *out-of-order processing of messages*.

### Quick Facts about Version 1.0 of ResilientDB
1. ResilientDB supports a Dockerized implementation, which allows to specify the number of clients and replicas.
2. PBFT [Castro and Liskov, 1998] protocol is used to achieve consensus among the replicas.
3. At present, clients only send YCSB transactions for processing.
4. ResilientDB expects minimum ##### 3f+1 ##### replicas, where f is the maximum number of byzantine (or malicious) replicas.

