<!--
  ~ Licensed to the Apache Software Foundation (ASF) under one
  ~ or more contributor license agreements.  See the NOTICE file
  ~ distributed with this work for additional information
  ~ regarding copyright ownership.  The ASF licenses this file
  ~ to you under the Apache License, Version 2.0 (the
  ~ "License"); you may not use this file except in compliance
  ~ with the License.  You may obtain a copy of the License at
  ~
  ~   http://www.apache.org/licenses/LICENSE-2.0
  ~
  ~ Unless required by applicable law or agreed to in writing,
  ~ software distributed under the License is distributed on an
  ~ "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
  ~ KIND, either express or implied.  See the License for the
  ~ specific language governing permissions and limitations
  ~ under the License.
-->

# ResilientDB Profiling and Monitoring Setup

This repository provides a comprehensive guide to set up profiling and monitoring for ResilientDB using Prometheus, Grafana, and related tools. The setup includes CPU and memory metrics visualization, GraphQL integration, and ResLens configuration.

---

## Prerequisites

Before running the ResLens application, you need to start kv service on the ResDB backend and the sdk.

### ResilientDB
Git clone the ResLens backend repository, a fork of ResilientDB and follow the instructions to set it up:
```bash
git clone https://github.com/harish876/incubator-resilientdb
```
Setup KV Service:
```bash

./service/tools/kv/server_tools/start_kv_service_monitoring.sh
```

### SDK
Git clone the GraphQL Repository and follow the instructions on the README to set it up:

Install GraphQL:
```bash
git clone https://github.com/ResilientApp/ResilientDB-GraphQL

```
Setup SDK:
```bash
bazel build service/http_server:crow_service_main

bazel-bin/service/http_server/crow_service_main service/tools/config/interface/client.config service/http_server/server_config.config
```

### Middleware
Git clone the ResLens Middleware Repository:

Install ResLens Middleware:
```bash
git clone https://github.com/harish876/MemLens-middleware
```

Setup ResLens Middleware:
```bash
npm instal

npm run start
```

###  Monitoring tools
Follow this to setup prometheus, grafana and a few other monitoring tools used in this application.
1. [Prometheus Installation](https://medium.com/@abdullah.eid.2604/prometheus-installation-on-linux-ubuntu-c4497e5154f6)
2. [Node Exporter Installation](https://medium.com/@abdullah.eid.2604/node-exporter-installation-on-linux-ubuntu-8203d033f69c)
3. [Process Exporter Installation](https://developer.couchbase.com/tutorial-process-exporter-setup)
4. [Pyroscope](https://dl.pyroscope.io/release/pyroscope-0.37.0-source.tar.gz)

### Detailed Guide for Configurations
TODO
