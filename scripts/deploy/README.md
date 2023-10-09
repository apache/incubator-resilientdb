This directory includes deployment scripts that help to deploy ResilientDB on multiple machines. At present, these scripts only support deploying KV service and KV Performance server.

# Usage

## Deploy KV Service

Add the IP addresses and the SSH key of the machines where you wish to deploy ResilientDB replicas and client proxy in the file [config/kv_server.conf](https://github.com/msadoghi/nexres/blob/master/deploy/config/kv_server.conf). 
We recommend using private IP addresses of each machine.

* If you do not require any SSH key to log in to a machine, then you would need to update the scripts.
* In these scripts, we assume that the ``root`` is ``ubuntu`` and the current working directory is located at ``/home/ubuntu/``. If this is not the case for your machines, you would need to update the scripts.
* If you have changed the path for the binary, then you would need to update the path stated in the scripts.


To deploy the KV service, run the following command from the ``deploy`` directory:

    ./script/deploy.sh ./config/kv_server.conf
    
If the script outputs ``Servers are running``, it implies that you have successfully deployed ReslientDB KV Service on desired machines.  

Note: this script creates a directory ``config_out``, which includes keys and certificates for all the replicas and the proxy. Further, it includes the configuration for replicas ``server.config`` and proxy ``client.config``.

### Testing deployed KV Service

To do so, we need to build and run the KV client tools:

    cd ../..
    bazel build service/tools/kv/api_tools/kv_service_tools
    bazel-bin/service/tools/kv/api_tools/kv_service_tools scripts/deploy/config_out/client.config get test 1234

Or:
	
    bazel run //service/tools/kv/api_tools:kv_service_tools -- $PWD/config_out/client.config get test

## Test Performance 

Our benchmark is based on the Key-Value service.

Before running, place the private IP addresses of your machines in the file ``config/kv_performance_server.conf``.

Run the script:

	./performance/run_performance.sh config/kv_performance_server.conf

Results will be saved locally and be shown on the screen as well.

Run Other protocol:

  POE: ./performance/run_performance.sh config/poe_performance_server.conf
