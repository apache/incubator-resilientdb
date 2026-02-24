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
*
*/

import ResShareToolkitClient from '../src/index.js';

async function run() {
  const client = new ResShareToolkitClient({
    baseUrl: 'https://fc70-136-109-248-65.ngrok-free.app' // Replace with ResilientDB ResShare backend URL
  });

  await client.auth.signup({ username: 'alice', password: 'Pass@123' });
  await client.auth.login({ username: 'alice', password: 'Pass@123' });

  const folder = await client.files.createFolder('docs');
  console.log('Create folder:', folder.status);

  const shares = await client.shares.list();
  console.log('Shared items:', shares.share_list || {});

  await client.auth.logout();
}

run().catch(error => {
  console.error('Example failed:', error);
});
