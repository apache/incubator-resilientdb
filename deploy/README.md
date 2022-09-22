Deployment scripts help you deploy the each Nexres Applications.
Current we support deploy KV server and KV Performance server.

# Usage

## Deploy KV Server

Put the server ip in [deploy.conf](https://github.com/msadoghi/nexres/blob/master/deploy/kv_server/deploy.conf). If your local machine could not access to the nodes using their private IPs, put the public IPs in the public_iplist section.
Otherwise, leave the public_iplist empty.

run:

    ./script/deploy.sh kv_server/deploy.conf
    
Once it is done, kv_server has been running inside the nodes pointed from the IPs. 
The script will also create a output folder inside kv_server/ and generate the server.config and client.config.

### Test your kv server

build and run the kv client tools:

    bazel build //example:kv_server_tools
    cd ..
    ./bazel-bin/example/kv_server_tools deploy/kv_server/output/client.config get test 1234

## Deploy KV Performance Server

Put the server ip in [deploy_performance_server.conf](https://github.com/msadoghi/nexres/blob/master/deploy/kv_server/deploy_performance_server.conf). If your local machine could not access to the nodes using their private IPs, put the public IPs in the public_iplist section.
Otherwise, leave the public_iplist empty.

run:

    ./script/deploy.sh kv_server/deploy_performance_server.conf
    
Once it is done, kv_server has been running inside the nodes pointed from the IPs. 
The script will also create a output folder inside kv_server/ and generate the server.config and client.config.

### Run the performance

    bazel build //kv_client:kv_performance_client_main
    cd ..
    ./bazel-bin/kv_client/kv_performance_client_main deploy/kv_server/output/client.config

### Get the performance
 
ssh to one of the machine where the kv server located, e,g, the first IP in the iplist. 
Grep the monitor log from kv_server_performance.log

    grep txn kv_server_performance.log
    
Then you will see the TPS during the 5 second period. e.g.

    server call:465157 server process:465156 socket recv:0 client call:465156 client req:46519 broad_cast:139557 send broad_cast:139563 per send broad_cast:49783 propose:46518 prepare:5815 commit:5813 pending execute:46504 execute:46504 execute done:46991 seq gap:70 total request:4699100 txn:939820 seq fail:0 time:210

### Benchmark


The current codebase has the follow TPS formance:

|  TPS (txn/s) | nodes | Input Threads  |  Output Threads  |  Worker Threads | Transactions In Flight |
|  ----  | ---- | ---- | ---- | ---- | ---- |
| 939820 |  4   |   2  |  2   |  96  | 2048 |

#### Setting

Oracle VM.Standard2.8

- 20.04.1-Ubuntu x86_64 x86_64 x86_64 GNU/Linux
- Intel(R) Xeon(R) Platinum 8167M CPU @ 2.00GHz
- Network bandwidth (Gbps): 8.2
- Memory (GB): 120
- 8 OCPU (16 cores)

## Note
**If you could not access to the node using the private IP in client.config, please change it to its public ip.**
     
