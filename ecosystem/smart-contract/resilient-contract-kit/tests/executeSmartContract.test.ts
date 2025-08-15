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

const { executeSmartContract } = require('../src/index');

jest.mock('../src/graphql-client', () => {
  return {
    createGraphQLClient: jest.fn().mockReturnValue({
      request: jest.fn().mockResolvedValue({ executeContract: 'Success' }),
    }),
  };
});

describe('executeSmartContract', () => {
  it('should execute a smart contract and return success', async () => {
    const result = await executeSmartContract({
      apiUrl: 'http://localhost:8400/graphql',
      config: 'config-data',
      sender: '0xSenderAddress',
      contractAddress: '0xContractAddress',
      functionName: 'transfer',
      arguments: ['arg1', 'arg2'],
    });

    expect(result).toBe('Success');
  });
});
