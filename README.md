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

### Steps to Run and Compile through Docker


-------------- <br/>

### Steps to Run and Compile without Docker <br/>

* We provide a script **startResilientDB.sh** to compile and run the code. To run **ResilientDB** on a cluster such as AWS, Azure or Google Cloud, you need to specify the **Private IP Addresses** of each replica. 
* So state the Private IP addresses in the script.
* Run script as: **./scripts/startResilientDB.sh <number of servers> <number of clients> <batch size>**
* Prior to running the code, create a folder named **results** inside **resilientdb**.
* All the results after running the script will be stored inside the **results** folder.






-------------- <br/>


### Relevant Parameters of "config.h"
<pre>
* NODE_CNT			Total number of replicas, minimum 4, that is, f=1.  
* THREAD_CNT			Total number of threads at primary (at least 5)
* REM_THREAD_CNT		Total number of input threads at a replica (set it to 3)
* SEND_THREAD_CNT		Total number of output threads at a replica (at least 1)
* CLIENT_NODE_CNT		Total number of clients (at least 1).  
* CLIENT_THREAD_CNT		Total number of threads at a client (at least 1)
* CLIENT_REM_THREAD_CNT		Total number of input threads at a client (set it to 1)
* SEND_THREAD_CNT		Total number of output threads at a client (set it to 1)
* MAX_TXN_IN_FLIGHT		Multiple of Batch Size
* DONE_TIMER			Amount of time to run the system.
* WARMUP_TIMER			Amount of time to warmup the system (No statistics collected).
* BATCH_THREADS			Number of threads at primary to batch client transactions.
* BATCH_SIZE			Number of transactions in a batch (at least 10)
* TXN_PER_CHKPT			Frequency at which garbage collection is done.
* USE_CRYPTO			To switch on and off cryptographic signing of messages.
* CRYPTO_METHOD_RSA		To use RSA based digital signatures.
* CRYPTO_METHOD_ED25519		To use ED25519 based digital signatures.
* CRYPTO_METHOD_CMAC_AES	To use CMAC + AES combination for authentication.
* SYNTH_TABLE_SIZE		The range of keys for clients to select.
</pre>

