#
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#  http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.
#

## Transaction Specs
[BigchainDB transactions](https://github.com/bigchaindb/BEPs/tree/master/13)

The first thing to understand about Resdb is how we structure our data. Traditional SQL databases structure data in tables. NoSQL databases use other formats to structure data such as JSON and key-values, as well as tables. At Resdb, we structure data as assets. We believe anything can be represented as an asset. An asset can characterize any physical or digital object that you can think of like a car, a data set or an intellectual property right.

These assets can be registered on Resdb in two ways. 
1. By users in CREATE transactions.
2. Transferred (or updated) to other users in TRANSFER transactions. 

Traditionally, people design applications focusing on business processes (e.g. apps for booking & processing client orders, apps for tracking delivery of products etc). At Resdb, we don’t focus on processes rather on assets (e.g. a client order can be an asset that is then tracked across its entire lifecycle). This switch in perspective from a process-centric towards an asset-centric view influences much of how we build applications.

## BigchainBD License
- [Apache License 2.0](https://fossa.com/blog/open-source-licenses-101-apache-license-2-0/)

## Making client requests Asyncronous
[Comparison between different ways to send send request](https://julien.danjou.info/python-and-fast-http-clients/)

[asyncio - an introduction](https://www.datacamp.com/tutorial/asyncio-introduction)

[more asyncio](https://realpython.com/async-io-python/)

[aiohttp](https://github.com/aio-libs/aiohttp)


## Remaining tasks
1. test the sdk
2. Implement verify transaction on resdb client
3. A solution for storing secrets
4. Build the APIs for NFT marketplce

~ 3 weeks
