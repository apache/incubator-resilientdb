# ResilientDB: A High-throughput yielding Permissioned Blockchain Fabric.

 ResilientDB aims at *Making Permissioned Blockchain Systems Fast Again*. ResilientDB makes *system-centric* design decisions by adopting a *multi-thread architecture* that encompasses *deep-pipelines*. Further, we *separate* the ordering of client transactions from their execution, which allows us to perform *out-of-order processing of messages*.

### Release Notes Version 3.0 of ResilientDB
1. Implementation of **GeoBFT** protcol inside resilientDB 
    - [link](http://www.vldb.org/pvldb/vol13/p868-gupta.pdf) to the publication
    - Config Required to adjust [`GBFT, GBFT_CLUSTER_SIZE, GBFT_CCM_THREAD_CNT`]
2. Work Queues and multi-threaded structure have be re-written
3. Minor Changes:
    - Statistics
    - Client Memory issue update
    - Execution Queues
    - Result Scripts


### Release Notes Version 2.0 of ResilientDB
1. ResilientDB supports a **Dockerized** implementation, which allows specifying the number of clients and replicas.
2. **PBFT** [Castro and Liskov, 1998] protocol is used to achieve consensus among the replicas.
3. ResilientDB expects minimum **3f+1** replicas, where **f** is the maximum number of byzantine (or malicious) replicas.
4. ReslientDB designates one of its replicas as the **primary** (replicas with identifier **0**), which is also responsible for initiating the consensus.
5. At present, each client only sends YCSB-style transactions for processing, to the primary.
6. Each client transaction has an associated **transaction manager**, which stores all the data related to the transaction.
7. Depending on the type of replica (primary or non-primary), we associate different a number of threads and queues with each replica.
8. ResilientDB allows easy implementation of **Smart Contracts**. At present, we provide a comprehensive implementation of **Banking Smart Contracts**.
9. To facilitate data storage and persistence, ResilientDB provides support for an **in-memory key-value store**. Further, users can take advantage of **SQL query** execution through the fully-integrated APIs for **SQLite**.
10. With ResilientDB we also provide a seamless **GUI display**. This display generates a status log and also accesses **Grafana to plot the results**. Further details regarding the setup of GUI display are available in the **dashboard** folder.

---

### Steps to Run and Compile through Docker
First, install docker and docker-compose:

- [Install Docker-CE](https://docs.docker.com/install/)
- [Install Docker-Compose](https://docs.docker.com/compose/install/)


Use the Script ``resilientDB-docker``

    Usage:
     ./resilientDB-docker --clients=1 --replicas=4
     ./resilientDB-docker -d [default 4 replicas and 1 client]

## Result
-   The result will be printed on STDOUT and also ``res.out`` file. It contains the Throughputs and Latencies for the run and summary of each thread in replicas.
## warning:
-   Using docker, all replicas and clients will be running on one machine as containers, so a large number of replicas would degrade the performance of your system

---

## Steps to Run and Compile without Docker <br/>

#### We strongly recommend that first try the docker version, Here are the steps to run on a real environment:

* First Step is to untar the dependencies:

        cd deps && \ls | xargs -i tar -xvf {} && cd ..
* Create **obj** folder inside **resilientdb** folder, to store object files. And **results** to store the results.

        mkdir obj
        mkdir results
* Create a folder named **results** inside **resilientdb** to store the results.
        
* We provide a script **startResilientDB.sh** to compile and run the code. To run **ResilientDB** on a cluster such as AWS, Azure or Google Cloud, you need to specify the **Private IP Addresses** of each replica. 
* The code will be compiled on the machine that is running the **startResilientDB.sh** and send the binary files over the SSH to the **resilientdb** folder in all other  nodes. the directory which contains the **resilientdb** in nodes should be set as ``home_directory`` in following files as :
    1. scripts/scp_binaries.sh
    2. scripts/scp_results.sh
    3. scripts/simRun.py
* **change the ``CNODES`` and ``SNODES`` arrays in ``scripts/startResilientDB.sh`` and put IP Addresses.**
* Adjust the parameters in ``config.h`` such as number of replicas and clients
* Run script as: **./scripts/startResilientDB.sh \<number of servers\> \<number of clients\> \<batch size\>**

* All the results after running the script will be stored inside the **results** folder.


#### What is happening behind the scenes?

* The code is compiled using command: **make clean; make**
* On compilation, two new files are created: **runcl** and **rundb**.
* Each machine is going to act as a client needs to execute **runcl**.
* Each machine is going to act as a replica needs to execute **rundb**. 
* The script runs each binary as: **./rundb -nid\<numeric identifier\>**
* This numeric identifier starts from **0** (for the primary) and increases as **1,2,3...** for subsequent replicas and clients.



---


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
* ENABLE_CHAIN			Set it to true if blocks need to be stored in a ledger.
* TXN_PER_CHKPT			Frequency at which garbage collection is done.
* USE_CRYPTO			To switch on and off cryptographic signing of messages.
* CRYPTO_METHOD_RSA		To use RSA based digital signatures.
* CRYPTO_METHOD_ED25519		To use ED25519 based digital signatures.
* CRYPTO_METHOD_CMAC_AES	To use CMAC + AES combination for authentication.
* SYNTH_TABLE_SIZE		The range of keys for clients to select.
* EXT_DB MEMORY			To specify the type of memory storage (in-memory of SQLite)..
* BANKING_SMART_CONTRACT	To allow usage of smart contraacts instead of YCSB bechmarks.



</pre>

<br/>

---

* There are several other parameters in *config.h*, which are unusable (or not fully tested) in the current version.


