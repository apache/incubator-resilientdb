/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

const { createGraphQLClient } = require('./graphql-client');

async function executeSmartContract(params) {
  const client = createGraphQLClient(params.apiUrl);

  const query = `
    mutation ExecuteSmartContract(
      $config: String!,
      $sender: String!,
      $contract: String!,
      $functionName: String!,
      $arguments: String!
    ) {
      executeContract(
        config: $config,
        sender: $sender,
        contract: $contract,
        functionName: $functionName,
        arguments: $arguments
      )
    }
  `;

  const variables = {
    config: params.config,
    sender: params.sender,
    contract: params.contractAddress,
    functionName: params.functionName,
    arguments: params.arguments.join(','),
  };

  try {
    const response = await client.request(query, variables);
    return response.executeContract;
  } catch (error) {
    throw new Error(`Error executing smart contract: ${error.message}`);
  }
}

module.exports = { executeSmartContract };
