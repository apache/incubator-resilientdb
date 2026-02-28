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
class SharesClient {
  constructor(client) {
    this.client = client;
  }

  async share({ target, path, node }) {
    if (!target) {
      throw new Error('share requires a target username');
    }

    const payload = { target };

    if (path) {
      payload.path = path;
    }

    if (node) {
      payload.node = typeof node === 'string' ? node : JSON.stringify(node);
    }

    return this.client._requestJson('/share', {
      method: 'POST',
      body: payload
    });
  }

  async list() {
    return this.client._requestJson('/shared', {
      method: 'GET'
    });
  }

  async remove({ combinedPath, fromUser, path }) {
    const nodePath = combinedPath || (fromUser && path ? `${fromUser}/${path}` : null);
    if (!nodePath) {
      throw new Error('remove requires combinedPath or fromUser and path');
    }

    return this.client._requestJson('/delete', {
      method: 'DELETE',
      body: {
        node_path: nodePath,
        delete_in_root: false
      }
    });
  }
}

export default SharesClient;
