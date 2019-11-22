# ResilientDB: A High-throughput yielding Permissioned Blockchain Fabric.

### ResilientDB aims at *Making Permissioned Blockchain Systems Fast Again*. ResilientDB makes *system-centric* design decisions by adopting a *multi-thread architecture* that encompasses *deep-pipelines*. Further, we *separate* ordering of client transactions from their execution, which allows us to perform *out-of-order processing of messages*.

### Quick Facts about Version 1.0 of ResilientDB
1. ResilientDB supports a **Dockerized** implementation, which allows to specify the number of clients and replicas.
2. **PBFT** [Castro and Liskov, 1998] protocol is used to achieve consensus among the replicas.
3. ResilientDB expects minimum **3f+1** replicas, where **f** is the maximum number of byzantine (or malicious) replicas.
4. ReslientDB designates one of its replicas as the **primary** (replicas with identifier **0**), which is also responsible for initiating the consensus.
5. At present, each client only sends YCSB-style transactions for processing, to the primary.
7. Each client transaction has an associated **transaction manager**, which stores all the data related to the transaction.
6. Depending on the type of replica (primary or non-primary), we associate different number of threads and queues with each replica.

-------------- <br/> 

### Steps to Run and Compile


-------------- <br/>

### Relevant Parameters of "config.h"
<pre>
NODE_CNT	Total number of replicas, minimum 4, that is, f=1.  
</pre>

