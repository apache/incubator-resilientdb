Deployment scripts help you deploy the each Nexres Applications.
Current we support deploy KV server and KV Performance server.

# Usage

## Deploy KV Server

Put the server ip and the SSH key in [config/kv_server.conf](https://github.com/msadoghi/nexres/blob/master/deploy/config/kv_server.conf). 
Please use their private IPs.
If you change the binary path, also modify the path.


run:

    ./script/deploy.sh ./config/kv_server.conf
    
Once it is done, kv_server has been running inside the nodes pointed from the IPs. 
The script will also create a output folder(config_out) containing the generated server.config and client.config.

### Test your kv server

build and run the kv client tools:

    bazel build //example:kv_server_tools
    cd ..
    ./bazel-bin/example/kv_server_tools deploy/config_out/client.config get test 1234

Or:
	
    bazel run //example:kv_server_tools -- $PWD/config_out/client.config get test

## Deploy KV Performance Server

Put the server ip in [deploy_performance_server.conf](https://github.com/msadoghi/nexres/blob/master/deploy/kv_server/deploy_performance_server.conf). If your local machine could not access to the nodes using their private IPs, put the public IPs in the public_iplist section.
Otherwise, leave the public_iplist empty.

run:

    ./script/deploy.sh ./config/kv_performance_server.conf
    
Once it is done, kv_server has been running inside the nodes pointed from the IPs. 
The script will also create a output folder inside kv_server/ and generate the server.config and client.config.

### Run the performance

    bazel run //example:kv_server_tools -- $PWD/config_out/client.config set test 1234

This tools will trigger the eval on the server. The parameter for the set function does not matter.

### Get the performance

It will take a few seconds for the replica to generate data and send it out to the primary.
Keep tracking the performance log on one of the replica, like on server1:

    tail -f kv_server_performance.log | grep txn
    
Then you will see some performance logging during the 5 second period. e.g.

    server call:465157 server process:465156 socket recv:0 client call:465156 client req:46519 broad_cast:139557 send broad_cast:139563 per send broad_cast:49783 propose:46518 prepare:5815 commit:5813 pending execute:46504 execute:46504 execute done:46991 seq gap:70 total request:4699100 txn:939820 seq fail:0 time:210

