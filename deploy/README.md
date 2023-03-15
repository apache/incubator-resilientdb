This directory includes deployment scripts that help to deploy ResilientDB on multiple machines. At present, these scripts only support deploying KV service and KV Performance server.

# Usage

## Deploy KV Service

Add the IP addresses and the SSH key of the machines where you wish to deploy ResilientDB replicas and client proxy in the file [config/kv_server.conf](https://github.com/msadoghi/nexres/blob/master/deploy/config/kv_server.conf). 
We recomment using private IP addresses of each machine.

* If you do not require any SSH key to log in to a machine, then you would need to update the scripts.
* In these scripts, we assume that the ``root`` is ``ubuntu`` and the current working directory is located at ``/home/ubuntu/``. If this is not the case for your machines, you would need to update the scripts.
* If you have changed the path for the binary, then you would need to update the path stated in the scripts.


To deploy the KV service, run the following command from the ``deploy`` directory:

    ./script/deploy.sh ./config/kv_server.conf
    
If the script outputs ``Servers are running``, it implies that you have successfully deployed ReslientDB KV Service on desired machines.  

Note: this script creates a directory ``config_out``, which includes keys and certificates for all the replicas and the proxy. Further, it includes the configuration for replicas ``server.config`` and proxy ``client.config``.

### Testing deployed KV Service

To do so, we need to build and run the KV client tools:

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

