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

install bech32:

	pip install bech32

Create your own private key:

	bazel run //service/tools/utxo/wallet_tool/py:keys


Create your wallet address:
	
	bazel run //service/tools/utxo/wallet_tool/py:addr -- 3056301006072A8648CE3D020106052B8104000A034200049C8FBD86EA4E38FD607CD3AC49FEB75E364B0C694EFB2E6DDD33ABED0BB1017575A79CC53EC6A052F839B4876E96FF9E4B08ECF23EC9CD495B82ECF9D95303BD


Start service:

	./service/tools/utxo/service_tools/start_utxo_service.sh


Build the tools:
	
	bazel build service/tools//utxo/wallet_tool/cpp/utxo_client_tools


Transfer the coins:

	bazel-bin/service/tools/utxo/wallet_tool/cpp/utxo_client_tools -c service/tools/config/interface/service.config -m transfer -t bc1qd5ftrxa3vlsff5dl04nxg06ku6p4w6enk0cna9 -d bc1q09tk54hqfz5muzn9rgalfkdjfey8qpuhmzs5zn -x 0 -v 100 -p 303E020100301006072A8648CE3D020106052B8104000A0427302502010104202CB99BBB2AFEB7F48A574064091B34F24781C93AD8181A511C8DCFB2A111AD82 -b 3056301006072A8648CE3D020106052B8104000A03420004F838F3253A5224411D8951AA6EF2BB474EDD283EC088CD13D5404956C0A88079ECF539D9669A3D639A35BF9FD0F67ECBB3D332733C59B0272EB844405B6568D3

Get the transaction list:

	bazel-bin/service/tools/utxo/wallet_tool/cpp/utxo_client_tools -c service/tools/config/interface/service0.config -m list -e -1 -n 5

Get the wallet value:

	bazel-bin/service/tools/utxo/wallet_tool/cpp/utxo_client_tools -c service/tools/config/interface/service0.config -m wallet -t bc1qd5ftrxa3vlsff5dl04nxg06ku6p4w6enk0cna9
