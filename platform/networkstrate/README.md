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

<div align=center><img src=https://docs.google.com/drawings/d/e/2PACX-1vSkTj4ZujX3NOS18gt_xzX6hqobVCDRnpbbVUsWeV7L1-s4xF2Pg8NsFKLGQl---LsE9TzMQUseOtPU/pub?w=990&h=700#NexresRPC>
</div>
NexresRPC Server is a modern high-performance asynchronous IO Remote Procedure Call(RPC) framework that supports any service running in Byzantine Environment. 
It provides the capability for any service to connect other services in or out of the same cluster. 

NexresRPC contains three components: Acceptor, Service, and ReplicaClient.

* Acceptor runs two types of connections: **Long Connection** that is built between internal services, and **Short Connection** that is connected from the user clients. 
    * Long Connection uses an Async Acceptor, which uses the [boost::asio](https://www.boost.org/doc/libs/1_79_0/doc/html/boost_asio.html) library as the core io operator, to manage the socket channels. 
    * Short Connection uses the standard tcp socket library to accept a client request. The client socket information will be passed to the service in case 
they need to send messages back to the client. The incoming messages will be pushed into the message input queue(MIQ). 

* Service runs multiple workers which keep watching the incoming messages from MIQ, popping and executing. Messages that need to be 
delivered to other services will be pushed into the message output queue(MOQ).

* ReplicaClient is also an asnychronous io client which uses the [boost::asio](https://www.boost.org/doc/libs/1_79_0/doc/html/boost_asio.html) library to send out the messages from MOQ as well. Async Replica Client handlers a few 
numbers of [boost::io_service](https://www.boost.org/doc/libs/1_66_0/doc/html/boost_asio/reference/io_service.html) so as to fully utilize the IO write. Messages from MOQ will be grouped as one batch to reduce the IO latency.
