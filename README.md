# Usage

## Build and Deploy ResilientDB

Next, we show how to quickly build ResilientDB and deploy 4 replicas and 1 client proxy on your local machine. The proxy acts as an interface for all the clients. It batches client requests and forwards these batches to the replica designated as the leader. The 4 replicas participate in the PBFT consensus to order and execute these batches. Post execution, they return the response to the leader.

Install dependencies:

    ./INSTALL.sh


Run ResilientDB (Providing a Key-Value Service):

    ./service/tools/kv/server_tools/start_kv_service.sh
    
- This script starts 4 replicas and 1 client. Each replica instantiates a key-value store.

Build Interactive Tools:

    bazel build service/tools/kv/api_tools/kv_service_tools
    

## Test Performance

Add the IP addresses of the machines where you wish to deploy ResilientDB replicas and client proxy in the file [scripts/deploy/config/kv_performance_server.conf](scripts/deploy/config/kv_performance_server.conf). 

We recommend using private IP addresses of each machine.

* If you do not require any SSH key to log in to a machine, then you would need to update the scripts.
* In these scripts, we assume that the ``root`` is ``ubuntu`` and the current working directory is located at ``/home/ubuntu/``. If this is not the case for your machines, you would need to update the scripts.
* If you have changed the path for the binary, then you would need to update the path stated in the scripts.

Our benchmark is based on the Key-Value service.

Before running, place the private IP addresses of your machines in the file ``scripts/deploy/config/kv_performance_server.conf``.
Also, please create the ssh key file in the config "config/key.conf" and put your ssh key there (See the [scripts/deploy/config/key_example.conf](scripts/deploy/config/key_example.conf) as an example). 

Enter the path for performance test:
   cd scripts/deploy

Run the script to copy tpcc database file:

    ./script/copy_tpcc_db_file.sh

Run the script for HotStuff-1:
   
	./performance/hs1_performance.sh config/kv_performance_server.conf

Run the script for HotStuff:
   
	./performance/hs_performance.sh config/kv_performance_server.conf

Run the script for HotStuff-2:
   
	./performance/hs2_performance.sh config/kv_performance_server.conf

Run the script for HotStuff-1 with Slotting (To achieve the best performance of HotStuff-1 with Slotting, please uncomment Line 10 in [platform/networkstrate/async_replica_client.cpp](platform/networkstrate/async_replica_client.cpp)):
   
	./performance/slot_hs1_performance.sh config/kv_performance_server.conf

Results will be saved locally and be shown on the screen as well after running for 60 seconds.

You can test different parameters by editing the config files in "[scripts/deploy/config/](scripts/deploy/config/)". For example, [hs1.config](scripts/deploy/config/hs1.config) is the config file of HotStuff-1.


Below are the parameters that are related to our experiments:

"clientBatchNum": Batch Size,
"non_responsive_num": Number of slow leaders,
"fork_tail_num": Number of faulty leader that conduct tail-forking attack,
"rollback_num": Number of faulty leader that conduct rollback attack,
"tpcc_enabled": Use TPC-C benchmark,
"network_delay_num": Turn on Network Delay Test,
"mean_network_delay": The avearge value (us) of injected message delay.
