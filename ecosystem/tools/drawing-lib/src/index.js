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

/**
 * Collaborative Drawing Toolkit - Main Client
 * 
 * Universal JavaScript toolkit for building collaborative drawing applications
 * with blockchain-backed storage.
 * 
 * @example
 * ```javascript
 * import DrawingClient from 'collaborative-drawing-toolkit';
 * 
 * const client = new DrawingClient({
 *   baseUrl: 'https://api.example.com',
 *   apiVersion: 'v1'
 * });
 * 
 * await client.auth.login({ username: 'alice', password: 'password' });
 * const canvas = await client.canvases.create({ name: 'My Canvas', type: 'public' });
 * await client.canvases.addStroke(canvas.id, {
 *   pathData: [{ x: 0, y: 0 }, { x: 100, y: 100 }],
 *   color: '#000000',
 *   lineWidth: 2
 * });
 * ```
 */

import AuthClient from './modules/auth.js';
import CanvasesClient from './modules/canvases.js';
import InvitesClient from './modules/invites.js';
import NotificationsClient from './modules/notifications.js';
import SocketClient from './modules/socket.js';

class DrawingClient {
  /**
   * Create a new collaborative drawing client instance
   * 
   * @param {Object} config - Configuration options
   * @param {string} config.baseUrl - Base URL of backend API server (required)
   * @param {string} [config.apiVersion='v1'] - API version to use
   * @param {number} [config.timeout=30000] - Request timeout in milliseconds
   * @param {number} [config.retries=3] - Number of retries for failed requests
   * @param {Function} [config.onTokenExpired] - Callback when token expires
   */
  constructor(config) {
    if (!config || !config.baseUrl) {
      throw new Error('DrawingClient requires a baseUrl in config');
    }

    this.config = {
      baseUrl: config.baseUrl.replace(/\/$/, ''), // Remove trailing slash
      apiVersion: config.apiVersion || 'v1',
      timeout: config.timeout || 30000,
      retries: config.retries || 3,
      onTokenExpired: config.onTokenExpired || null
    };

    // Build API base URL with version prefix
    this.apiBase = `${this.config.baseUrl}/api/${this.config.apiVersion}`;

    // Internal state
    this._token = null;
    this._refreshCallback = null;

    // Initialize API modules
    this.auth = new AuthClient(this);
    this.canvases = new CanvasesClient(this);
    this.invites = new InvitesClient(this);
    this.notifications = new NotificationsClient(this);
    this.socket = new SocketClient(this);

    // Backward compatibility: provide deprecated 'rooms' alias
    Object.defineProperty(this, 'rooms', {
      get() {
        console.warn('[DEPRECATED] client.rooms is deprecated. Please use client.canvases instead.');
        return this.canvases;
      }
    });
  }

  /**
   * Set the authentication token for API requests
   * 
   * @param {string} token - JWT access token
   */
  setToken(token) {
    this._token = token;
  }

  /**
   * Get the current authentication token
   * 
   * @returns {string|null} Current JWT token or null
   */
  getToken() {
    return this._token;
  }

  /**
   * Internal method to make HTTP requests with retry logic
   * 
   * @private
   * @param {string} path - API endpoint path
   * @param {Object} options - Fetch options
   * @returns {Promise<any>} Parsed JSON response
   * @throws {ApiError} When request fails
   */
  async _request(path, options = {}) {
    const url = `${this.apiBase}${path}`;
    const headers = {
      'Content-Type': 'application/json',
      ...(options.headers || {})
    };

    // Add authorization header if token is set
    if (this._token) {
      headers['Authorization'] = `Bearer ${this._token}`;
    }

    const fetchOptions = {
      ...options,
      headers,
      credentials: 'include' // Include cookies for refresh tokens
    };

    let lastError = null;
    for (let attempt = 0; attempt <= this.config.retries; attempt++) {
      try {
        const controller = new AbortController();
        const timeoutId = setTimeout(() => controller.abort(), this.config.timeout);

        const response = await fetch(url, {
          ...fetchOptions,
          signal: controller.signal
        });

        clearTimeout(timeoutId);

        // Handle successful responses
        if (response.ok) {
          const contentType = response.headers.get('content-type');
          if (contentType && contentType.includes('application/json')) {
            return await response.json();
          }
          return null;
        }

        // Handle error responses
        let errorBody = null;
        try {
          errorBody = await response.json();
        } catch (e) {
          errorBody = { message: response.statusText };
        }

        // Handle token expiration
        if (response.status === 401 && this.config.onTokenExpired) {
          try {
            const newToken = await this.config.onTokenExpired();
            if (newToken) {
              this.setToken(newToken);
              // Retry the request with new token
              continue;
            }
          } catch (refreshError) {
            throw new ApiError(response.status, errorBody, 'Token refresh failed');
          }
        }

        throw new ApiError(response.status, errorBody);
      } catch (error) {
        lastError = error;

        // Don't retry on certain errors
        if (error instanceof ApiError) {
          if (error.status === 400 || error.status === 401 || error.status === 403 || error.status === 404) {
            throw error;
          }
        }

        // Don't retry on last attempt
        if (attempt === this.config.retries) {
          break;
        }

        // Exponential backoff
        await this._sleep(Math.pow(2, attempt) * 1000);
      }
    }

    throw lastError || new Error('Request failed after retries');
  }

  /**
   * Sleep for specified milliseconds
   * 
   * @private
   * @param {number} ms - Milliseconds to sleep
   * @returns {Promise<void>}
   */
  _sleep(ms) {
    return new Promise(resolve => setTimeout(resolve, ms));
  }
}

/**
 * Custom error class for API errors
 */
class ApiError extends Error {
  constructor(status, body, message = null) {
    super(message || body?.message || `API error: ${status}`);
    this.name = 'ApiError';
    this.status = status;
    this.body = body;
  }

  /**
   * Check if error is an authentication error (401)
   */
  isAuthError() {
    return this.status === 401;
  }

  /**
   * Check if error is a validation error (400)
   */
  isValidationError() {
    return this.status === 400;
  }

  /**
   * Get validation errors map
   */
  getValidationErrors() {
    return this.body?.errors || {};
  }

  /**
   * Get user-friendly error message
   */
  getUserMessage() {
    if (this.body?.errors && typeof this.body.errors === 'object') {
      return Object.entries(this.body.errors)
        .map(([field, msg]) => `${field}: ${msg}`)
        .join(', ');
    }
    return this.body?.message || this.message;
  }
}

export default DrawingClient;
export { ApiError };
