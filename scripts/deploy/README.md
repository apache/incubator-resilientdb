<!--
  - Licensed to the Apache Software Foundation (ASF) under one
  - or more contributor license agreements.  See the NOTICE file
  - distributed with this work for additional information
  - regarding copyright ownership.  The ASF licenses this file
  - to you under the Apache License, Version 2.0 (the
  - "License"); you may not use this file except in compliance
  - with the License.  You may obtain a copy of the License at
  -
  -   http://www.apache.org/licenses/LICENSE-2.0
  -
  - Unless required by applicable law or agreed to in writing,
  - software distributed under the License is distributed on an
  - "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
  - KIND, either express or implied.  See the License for the
  - specific language governing permissions and limitations
  - under the License.
  -->

This directory includes deployment scripts that help to deploy ResilientDB on multiple machines. At present, these scripts only support deploying KV service and KV Performance server.

# Usage

## Deploy KV Service

Add the IP addresses of the machines where you wish to deploy ResilientDB replicas and client proxy in the file [config/kv_server.conf](config/kv_server.conf). 
Create the ssh key file in the config "config/key.conf" and put your ssh key there (See the [key_example.conf](config/key_example.conf) as an example). 
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

	./performance/pbft_performance.sh config/kv_performance_server.conf

Results will be saved locally and be shown on the screen as well.

## Test Performance  Locally
Before running, place the private IP addresses of your machines in the file ``config/kv_performance_server_local.conf``.
You can simply add "127.0.0.1" to the files to specify the number of nodes and clients. 

Run the script:

	./performance_local/pbft_performance.sh config/kv_performance_server_local.conf


## Using non-ubuntu account ##
The default path for the workspace to deploy the system is /home/ubuntu
If you want to change the path, you can update ``script/env.sh``
