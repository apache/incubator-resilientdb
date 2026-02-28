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
class AuthClient {
  constructor(client) {
    this.client = client;
  }

  async login({ username, password }) {
    return this.client._requestJson('/login', {
      method: 'POST',
      body: { username, password },
      retries: 0
    });
  }

  async signup({ username, password }) {
    return this.client._requestJson('/signup', {
      method: 'POST',
      body: { username, password },
      retries: 0
    });
  }

  async logout() {
    return this.client._requestJson('/logout', {
      method: 'POST'
    });
  }

  async deleteUser({ password }) {
    return this.client._requestJson('/delete-user', {
      method: 'DELETE',
      body: { password }
    });
  }

  async status() {
    return this.client._requestJson('/auth-status', {
      method: 'GET'
    });
  }
}

export default AuthClient;
