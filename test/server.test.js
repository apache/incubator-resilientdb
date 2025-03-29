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

import axios from 'axios';
import { expect } from 'chai';

describe('GraphQL Server Reachability Test', function () {
  this.timeout(5000); // Set timeout to 5 seconds for the test

  it('should return status 200 for the GraphQL server', async function () {
    try {
      const response = await axios.post('http://localhost:8400/graphql', {
        query: `
          {
            __schema {
              types {
                name
              }
            }
          }
        `
      });
      // Check if the server responds with status 200
      expect(response.status).to.equal(200);
    } catch (error) {
      // If there's an error, fail the test
      throw new Error(`GraphQL server is not reachable: ${error.message}`);
    }
  });
});
