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

start the server:

	./service/tools/contract/service_tools/start_contract_service.sh

Now build the contract tool to help you access the server:

	bazel build service/tools/contract/api_tools/contract_tools

Using the contract tools to create an account first:

	bazel-bin/service/tools/contract/api_tools/contract_tools create -c service/tools/config/interface/service.config

Deploy a contract:

	bazel-bin/service/tools/contract/api_tools/contract_tools deploy -c service/tools/config/interface/service.config -p service/tools/contract/api_tools/example_contract/token.json -n token.sol:Token -a 1000 -m 0x67c6697351ff4aec29cdbaabf2fbe3467cc254f8

Transfer:

	bazel-bin/service/tools/contract/api_tools/contract_tools execute -c service/tools/config/interface/service.config -m 0x67c6697351ff4aec29cdbaabf2fbe3467cc254f8 -s 0xfc08e5bfebdcf7bb4cf5aafc29be03c1d53898f1 -f "transfer(address,uint256)" -a 0x1be8e78d765a2e63339fc99a66320db73158a35a,100
