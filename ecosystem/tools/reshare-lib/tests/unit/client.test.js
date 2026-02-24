import ResShareToolkitClient from '../../src/index.js';

class MockFormData {
  constructor() {
    this.fields = [];
  }

  append(name, value, filename) {
    this.fields.push({ name, value, filename });
  }
}

const jsonResponse = (data, status = 200) => ({
  ok: status >= 200 && status < 300,
  status,
  headers: {
    get: (name) => (name.toLowerCase() === 'content-type' ? 'application/json' : null)
  },
  json: async () => data
});

const jsonResponseWithCookies = (data, cookies, status = 200) => ({
  ok: status >= 200 && status < 300,
  status,
  headers: {
    get: (name) => (name.toLowerCase() === 'content-type' ? 'application/json' : null),
    getSetCookie: () => cookies
  },
  json: async () => data
});

describe('ResShareToolkitClient', () => {
  test('requires baseUrl', () => {
    expect(() => new ResShareToolkitClient({})).toThrow(/baseUrl/);
  });

  test('builds apiBase with prefix', () => {
    const client = new ResShareToolkitClient({
      baseUrl: 'http://localhost:5000/',
      apiPrefix: '/api/v1',
      fetch: async () => jsonResponse({}),
      FormData: MockFormData
    });

    expect(client.apiBase).toBe('http://localhost:5000/api/v1');
  });

  test('login sends JSON body and credentials', async () => {
    let captured = null;
    const fetch = async (url, options) => {
      captured = { url, options };
      return jsonResponse({ message: 'SUCCESS' });
    };

    const client = new ResShareToolkitClient({
      baseUrl: 'http://localhost:5000',
      fetch,
      FormData: MockFormData
    });

    await client.auth.login({ username: 'alice', password: 'secret' });

    expect(captured.url).toBe('http://localhost:5000/login');
    expect(captured.options.method).toBe('POST');
    expect(captured.options.credentials).toBe('include');
    expect(captured.options.headers['Content-Type']).toBe('application/json');
    expect(JSON.parse(captured.options.body)).toEqual({
      username: 'alice',
      password: 'secret'
    });
  });

  test('upload builds form data payload', async () => {
    let captured = null;
    const fetch = async (url, options) => {
      captured = { url, options };
      return jsonResponse({ message: 'SUCCESS' });
    };

    const client = new ResShareToolkitClient({
      baseUrl: 'http://localhost:5000',
      fetch,
      FormData: MockFormData
    });

    const file = { name: 'doc.txt' };
    await client.files.upload({ file, path: 'docs', skipProcessing: true });

    expect(captured.url).toBe('http://localhost:5000/upload');
    expect(captured.options.method).toBe('POST');
    expect(captured.options.body instanceof MockFormData).toBe(true);

    const fields = captured.options.body.fields;
    const fieldMap = Object.fromEntries(fields.map((field) => [field.name, field]));

    expect(fieldMap.path.value).toBe('docs');
    expect(fieldMap.skip_ai_processing.value).toBe('true');
    expect(fieldMap.file.filename).toBe('doc.txt');
  });

  test('remove share builds combined path', async () => {
    let captured = null;
    const fetch = async (url, options) => {
      captured = { url, options };
      return jsonResponse({ message: 'SUCCESS' });
    };

    const client = new ResShareToolkitClient({
      baseUrl: 'http://localhost:5000',
      fetch,
      FormData: MockFormData
    });

    await client.shares.remove({ fromUser: 'alice', path: 'docs' });

    expect(captured.url).toBe('http://localhost:5000/delete');
    expect(captured.options.method).toBe('DELETE');
    expect(JSON.parse(captured.options.body)).toEqual({
      node_path: 'alice/docs',
      delete_in_root: false
    });
  });

  test('persists session cookie between requests in Node', async () => {
    const calls = [];
    const fetch = async (url, options) => {
      calls.push({ url, options });
      if (calls.length === 1) {
        return jsonResponseWithCookies({ message: 'SUCCESS' }, ['session=abc123; Path=/; HttpOnly']);
      }
      return jsonResponse({ message: 'SUCCESS' });
    };

    const client = new ResShareToolkitClient({
      baseUrl: 'http://localhost:5000',
      fetch,
      FormData: MockFormData
    });

    await client.auth.login({ username: 'alice', password: 'secret' });
    await client.files.createFolder('docs');

    expect(calls[1].options.headers.Cookie).toBe('session=abc123');
  });
});
