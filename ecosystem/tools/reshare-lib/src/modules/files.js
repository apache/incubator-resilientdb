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
