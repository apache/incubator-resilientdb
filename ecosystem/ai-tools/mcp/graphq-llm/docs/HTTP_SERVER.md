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


# HTTP Server Endpoints

## Transactions
Note that key-value pairs committed to ResilientDB through the API store the JSON object as the value. To get the intended value of the pair, simply access the value of the JSON object.

### GET /v1/transactions
Get all values

### GET /v1/transactions/\<string>
Get value of specific id

### GET /v1/transactions/\<string>/\<string>
Get values based on key range

### POST /v1/transactions/commit
Sets a new value for a key. Note that key-value pairs committed to ResilientDB through the API store the JSON object as the value. To get the intended value of the pair, simply access the value of the JSON object.

Ex: `curl -X POST  -d '{"id":"samplekey","value":"samplevalue"}' localhost:18000/v1/transactions/commit`

## Blocks

### GET /v1/blocks
Retrieve all blocks

### GET /v1/blocks/\<int>
Retrieve all blocks, grouped in batch sizes of the int parameter

### GET /v1/blocks/\<int>/\<int>
Retrieve list of blocks within a range

## Miscellaneous
### GET /populatetable
Used for the Explorer webpage

## Todo
Update endpoints to use URL query parameters
