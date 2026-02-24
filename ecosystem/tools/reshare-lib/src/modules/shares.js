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
