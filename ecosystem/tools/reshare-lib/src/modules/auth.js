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
