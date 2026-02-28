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
class FilesClient {
  constructor(client) {
    this.client = client;
  }

  async createFolder(path) {
    return this.client._requestJson('/create-folder', {
      method: 'POST',
      body: { folder_path: path }
    });
  }

  async upload({ file, path = '', skipProcessing = true, filename }) {
    if (!file) {
      throw new Error('upload requires a file');
    }

    const formData = this.client._createFormData();
    const resolvedName = filename || file.name;

    if (resolvedName) {
      formData.append('file', file, resolvedName);
    } else {
      formData.append('file', file);
    }

    formData.append('path', path || '');
    formData.append('skip_ai_processing', String(Boolean(skipProcessing)));

    return this.client._requestJson('/upload', {
      method: 'POST',
      body: formData,
      retries: 0
    });
  }

  async deleteItem({ path, deleteInRoot = true }) {
    return this.client._requestJson('/delete', {
      method: 'DELETE',
      body: {
        node_path: path,
        delete_in_root: deleteInRoot
      }
    });
  }

  async download({ path, isShared = false }) {
    return this.client._requestRaw('/download', {
      method: 'POST',
      body: {
        path,
        is_shared: isShared
      }
    });
  }

  async downloadZip({ path, isShared = false }) {
    return this.client._requestRaw('/download-zip', {
      method: 'POST',
      body: {
        path,
        is_shared: isShared
      }
    });
  }
}

export default FilesClient;
