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
import AuthClient from './modules/auth.js';
import FilesClient from './modules/files.js';
import SharesClient from './modules/shares.js';

class ResShareToolkitClient {
  constructor(config) {
    if (!config || !config.baseUrl) {
      throw new Error('ResShareToolkitClient requires a baseUrl in config');
    }

    const baseUrl = String(config.baseUrl).replace(/\/+$/, '');
    const apiPrefix = this._normalizePrefix(config.apiPrefix);

    this.config = {
      baseUrl,
      apiPrefix,
      timeout: Number.isFinite(config.timeout) ? config.timeout : 30000,
      retries: Number.isFinite(config.retries) ? config.retries : 2,
      onAuthExpired: config.onAuthExpired || null,
      credentials: config.credentials || 'include',
      cookieJarEnabled: config.cookieJarEnabled
    };

    this.apiBase = `${this.config.baseUrl}${this.config.apiPrefix}`;

    this._fetch = config.fetch || globalThis.fetch;
    this._FormData = config.FormData || globalThis.FormData;

    if (!this._fetch) {
      throw new Error('ResShareToolkitClient requires fetch (provide config.fetch in Node)');
    }

    this._token = null;
    this._cookieJarEnabled = this.config.cookieJarEnabled;
    if (this._cookieJarEnabled === undefined) {
      this._cookieJarEnabled = typeof window === 'undefined';
    }
    this._cookieStore = new Map();

    this.auth = new AuthClient(this);
    this.files = new FilesClient(this);
    this.shares = new SharesClient(this);
  }

  setToken(token) {
    this._token = token || null;
  }

  clearToken() {
    this._token = null;
  }

  getToken() {
    return this._token;
  }

  async _requestJson(path, options = {}) {
    return this._request(path, { ...options, responseType: 'json' });
  }

  async _requestRaw(path, options = {}) {
    return this._request(path, { ...options, responseType: 'raw' });
  }

  _createFormData() {
    if (!this._FormData) {
      throw new Error('FormData is not available (provide config.FormData in Node)');
    }
    return new this._FormData();
  }

  _buildUrl(path) {
    if (!path) {
      return this.apiBase;
    }
    const normalizedPath = path.startsWith('/') ? path : `/${path}`;
    return `${this.apiBase}${normalizedPath}`;
  }

  _buildCookieHeader() {
    if (!this._cookieJarEnabled || !this._cookieStore.size) {
      return '';
    }
    return Array.from(this._cookieStore.entries())
      .map(([name, value]) => `${name}=${value}`)
      .join('; ');
  }

  _storeCookies(response) {
    if (!this._cookieJarEnabled || !response?.headers) {
      return;
    }

    let cookies = [];
    if (typeof response.headers.getSetCookie === 'function') {
      cookies = response.headers.getSetCookie();
    } else {
      const header = response.headers.get('set-cookie');
      if (header) {
        cookies = [header];
      }
    }

    cookies.forEach((cookie) => {
      const pair = cookie.split(';')[0];
      const separatorIndex = pair.indexOf('=');
      if (separatorIndex <= 0) {
        return;
      }
      const name = pair.slice(0, separatorIndex).trim();
      const value = pair.slice(separatorIndex + 1).trim();
      if (name) {
        this._cookieStore.set(name, value);
      }
    });
  }

  _normalizePrefix(prefix) {
    if (!prefix) {
      return '';
    }
    const trimmed = String(prefix).trim();
    if (!trimmed) {
      return '';
    }
    const withSlash = trimmed.startsWith('/') ? trimmed : `/${trimmed}`;
    return withSlash.replace(/\/+$/, '');
  }

  _normalizeResponse(data) {
    if (!data || typeof data !== 'object' || Array.isArray(data)) {
      return data;
    }

    if (data.status == null) {
      const status = data.result || data.message;
      if (status) {
        return { ...data, status };
      }
    }

    return data;
  }

  async _request(path, options = {}) {
    const url = this._buildUrl(path);
    const method = options.method || 'GET';
    const headers = { ...(options.headers || {}) };
    const maxRetries = Number.isFinite(options.retries) ? options.retries : this.config.retries;
    const responseType = options.responseType || 'json';

    let body = options.body;
    const isFormData = this._FormData && body instanceof this._FormData;
    const isString = typeof body === 'string';
    const isBlob = typeof Blob !== 'undefined' && body instanceof Blob;
    const isBinary = isBlob || body instanceof ArrayBuffer || body instanceof Uint8Array;

    if (body != null && !isFormData && !isString && !isBinary) {
      if (!headers['Content-Type']) {
        headers['Content-Type'] = 'application/json';
      }
      body = JSON.stringify(body);
    }

    if (this._token && !headers.Authorization) {
      headers.Authorization = `Bearer ${this._token}`;
    }

    const cookieHeader = this._buildCookieHeader();
    if (cookieHeader) {
      if (headers.Cookie) {
        headers.Cookie = `${headers.Cookie}; ${cookieHeader}`;
      } else {
        headers.Cookie = cookieHeader;
      }
    }

    const fetchOptions = {
      method,
      headers,
      body,
      credentials: options.credentials || this.config.credentials
    };

    let lastError = null;
    let refreshed = false;

    for (let attempt = 0; attempt <= maxRetries; attempt += 1) {
      try {
        const controller = new AbortController();
        const timeoutId = setTimeout(() => controller.abort(), this.config.timeout);

        const response = await this._fetch(url, {
          ...fetchOptions,
          signal: controller.signal
        });

        clearTimeout(timeoutId);
        this._storeCookies(response);

        if (response.ok) {
          if (responseType === 'raw') {
            return response;
          }

          const contentType = response.headers.get('content-type') || '';
          if (contentType.includes('application/json')) {
            const data = await response.json();
            return this._normalizeResponse(data);
          }

          return null;
        }

        let errorBody = null;
        try {
          errorBody = await response.json();
        } catch (error) {
          errorBody = { message: response.statusText };
        }

        if (response.status === 401 && this.config.onAuthExpired && !refreshed) {
          refreshed = true;
          const newToken = await this.config.onAuthExpired();
          if (newToken) {
            this.setToken(newToken);
            attempt -= 1;
            continue;
          }
        }

        throw new ApiError(response.status, this._normalizeResponse(errorBody));
      } catch (error) {
        lastError = error;

        if (error instanceof ApiError) {
          if ([400, 401, 403, 404].includes(error.status)) {
            throw error;
          }
        }

        if (attempt === maxRetries) {
          break;
        }

        await this._sleep(Math.pow(2, attempt) * 250);
      }
    }

    throw lastError || new Error('Request failed after retries');
  }

  _sleep(ms) {
    return new Promise(resolve => setTimeout(resolve, ms));
  }
}

class ApiError extends Error {
  constructor(status, body, message = null) {
    super(message || body?.message || body?.status || `API error: ${status}`);
    this.name = 'ApiError';
    this.status = status;
    this.body = body;
  }

  isAuthError() {
    return this.status === 401;
  }

  isValidationError() {
    return this.status === 400;
  }

  getUserMessage() {
    if (this.body?.errors && typeof this.body.errors === 'object') {
      return Object.entries(this.body.errors)
        .map(([field, msg]) => `${field}: ${msg}`)
        .join(', ');
    }
    return this.body?.message || this.message;
  }
}

export { ResShareToolkitClient, ApiError };
export default ResShareToolkitClient;
