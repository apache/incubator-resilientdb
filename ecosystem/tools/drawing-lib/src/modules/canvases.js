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
 * Canvases module for Collaborative Drawing SDK
 * 
 * Handles canvas management, drawing operations, and collaboration features.
 */

class CanvasesClient {
  constructor(client) {
    this.client = client;
  }

  /**
   * Create a new collaborative canvas
   * 
   * @param {Object} canvasData - Canvas configuration
   * @param {string} canvasData.name - Canvas name (1-256 chars, required)
   * @param {string} canvasData.type - Canvas type: "public", "private", or "secure" (required)
   * @param {string} [canvasData.description] - Canvas description (max 500 chars)
   * @returns {Promise<{canvas: object}>} Created canvas object
   */
  async create({ name, type, description }) {
    return await this.client._request('/canvases', {
      method: 'POST',
      body: JSON.stringify({ name, type, description })
    });
  }

  /**
   * List canvases accessible to authenticated user
   * 
   * @param {Object} [options={}] - List options
   * @param {boolean} [options.includeArchived] - Include archived canvases
   * @param {string} [options.sortBy] - Sort field: "createdAt", "updatedAt", "name"
   * @param {string} [options.order] - Sort order: "asc" or "desc"
   * @param {number} [options.page] - Page number for pagination
   * @param {number} [options.per_page] - Items per page
   * @param {string} [options.type] - Filter by canvas type
   * @returns {Promise<{canvases: array, pagination: object}>} Canvases list with pagination
   */
  async list(options = {}) {
    const params = new URLSearchParams();
    if (options.includeArchived) params.append('includeArchived', 'true');
    if (options.sortBy) params.append('sortBy', options.sortBy);
    if (options.order) params.append('order', options.order);
    if (options.page) params.append('page', options.page.toString());
    if (options.per_page) params.append('per_page', options.per_page.toString());
    if (options.type) params.append('type', options.type);

    const queryString = params.toString();
    const path = queryString ? `/canvases?${queryString}` : '/canvases';

    return await this.client._request(path, {
      method: 'GET'
    });
  }

  /**
   * Get details for a specific canvas
   * 
   * @param {string} canvasId - Canvas ID
   * @returns {Promise<{canvas: object}>} Canvas details
   */
  async get(canvasId) {
    return await this.client._request(`/canvases/${canvasId}`, {
      method: 'GET'
    });
  }

  /**
   * Update canvas settings
   * 
   * @param {string} canvasId - Canvas ID
   * @param {Object} updates - Fields to update
   * @param {string} [updates.name] - New canvas name
   * @param {string} [updates.description] - New description
   * @param {boolean} [updates.archived] - Archive status
   * @returns {Promise<{canvas: object}>} Updated canvas object
   */
  async update(canvasId, updates) {
    return await this.client._request(`/canvases/${canvasId}`, {
      method: 'PATCH',
      body: JSON.stringify(updates)
    });
  }

  /**
   * Delete a canvas (owner only)
   * 
   * @param {string} canvasId - Canvas ID
   * @returns {Promise<{status: string}>} Deletion status
   */
  async delete(canvasId) {
    return await this.client._request(`/canvases/${canvasId}`, {
      method: 'DELETE'
    });
  }

  /**
   * Share canvas with multiple users
   * 
   * @param {string} canvasId - Canvas ID
   * @param {Array<{username: string, role: string}>} users - Users to share with
   * @returns {Promise<{status: string, results: array}>} Share results
   */
  async share(canvasId, users) {
    return await this.client._request(`/canvases/${canvasId}/share`, {
      method: 'POST',
      body: JSON.stringify({ users })
    });
  }

  /**
   * Get list of canvas members with their roles
   * 
   * @param {string} canvasId - Canvas ID
   * @returns {Promise<{members: array}>} Canvas members
   */
  async getMembers(canvasId) {
    return await this.client._request(`/canvases/${canvasId}/members`, {
      method: 'GET'
    });
  }

  /**
   * Get drawing strokes for a canvas
   * 
   * @param {string} canvasId - Canvas ID
   * @param {Object} [options={}] - Query options
   * @param {number} [options.since] - Get strokes after this timestamp
   * @param {number} [options.until] - Get strokes before this timestamp
   * @returns {Promise<{strokes: array}>} Canvas strokes
   */
  async getStrokes(canvasId, options = {}) {
    const params = new URLSearchParams();
    if (options.since) params.append('since', options.since.toString());
    if (options.until) params.append('until', options.until.toString());

    const queryString = params.toString();
    const path = queryString ? `/canvases/${canvasId}/strokes?${queryString}` : `/canvases/${canvasId}/strokes`;

    return await this.client._request(path, {
      method: 'GET'
    });
  }

  /**
   * Submit a new drawing stroke to a canvas
   * 
   * @param {string} canvasId - Canvas ID
   * @param {Object} stroke - Stroke data
   * @param {Array<{x: number, y: number}>} stroke.pathData - Array of points
   * @param {string} stroke.color - Stroke color (hex format, e.g., "#000000")
   * @param {number} stroke.lineWidth - Line width in pixels
   * @param {string} [stroke.user] - Username (optional, server can infer)
   * @param {string} [stroke.tool] - Drawing tool name
   * @param {string} [stroke.signature] - Signature for secure canvases
   * @param {string} [stroke.signerPubKey] - Signer public key for secure canvases
   * @returns {Promise<{status: string, stroke: object}>} Submission result
   */
  async addStroke(canvasId, stroke) {
    return await this.client._request(`/canvases/${canvasId}/strokes`, {
      method: 'POST',
      body: JSON.stringify(stroke)
    });
  }

  /**
   * Undo the last stroke submitted by current user
   * 
   * @param {string} canvasId - Canvas ID
   * @returns {Promise<{status: string, undone: object}>} Undo result
   */
  async undo(canvasId) {
    return await this.client._request(`/canvases/${canvasId}/history/undo`, {
      method: 'POST'
    });
  }

  /**
   * Redo a previously undone stroke
   * 
   * @param {string} canvasId - Canvas ID
   * @returns {Promise<{status: string, redone: object}>} Redo result
   */
  async redo(canvasId) {
    return await this.client._request(`/canvases/${canvasId}/history/redo`, {
      method: 'POST'
    });
  }

  /**
   * Clear all strokes from canvas
   * 
   * @param {string} canvasId - Canvas ID
   * @returns {Promise<{status: string, clearedAt: number}>} Clear result
   */
  async clear(canvasId) {
    return await this.client._request(`/canvases/${canvasId}/strokes`, {
      method: 'DELETE'
    });
  }

  /**
   * Get undo/redo status for current user in canvas
   * 
   * @param {string} canvasId - Canvas ID
   * @returns {Promise<{canUndo: boolean, canRedo: boolean}>} Undo/redo availability
   */
  async getUndoRedoStatus(canvasId) {
    return await this.client._request(`/canvases/${canvasId}/history/status`, {
      method: 'GET'
    });
  }

  /**
   * Reset undo/redo stacks for current user
   * 
   * @param {string} canvasId - Canvas ID
   * @returns {Promise<{status: string}>} Reset status
   */
  async resetStacks(canvasId) {
    return await this.client._request(`/canvases/${canvasId}/history/reset`, {
      method: 'POST'
    });
  }

  /**
   * Transfer canvas ownership to another user
   * 
   * @param {string} canvasId - Canvas ID
   * @param {string} newOwnerId - New owner's user ID
   * @returns {Promise<{status: string}>} Transfer status
   */
  async transferOwnership(canvasId, newOwnerId) {
    return await this.client._request(`/canvases/${canvasId}/transfer`, {
      method: 'POST',
      body: JSON.stringify({ newOwnerId })
    });
  }

  /**
   * Leave a canvas (removes membership)
   * 
   * @param {string} canvasId - Canvas ID
   * @returns {Promise<{status: string}>} Leave status
   */
  async leave(canvasId) {
    return await this.client._request(`/canvases/${canvasId}/leave`, {
      method: 'POST'
    });
  }

  /**
   * Invite a user to join a canvas
   * 
   * @param {string} canvasId - Canvas ID
   * @param {string} username - Username to invite
   * @param {string} [role='editor'] - Role to assign: "owner", "editor", or "viewer"
   * @returns {Promise<{status: string, inviteId: string}>} Invitation result
   */
  async invite(canvasId, username, role = 'editor') {
    return await this.client._request(`/canvases/${canvasId}/invite`, {
      method: 'POST',
      body: JSON.stringify({ username, role })
    });
  }

  /**
   * Search for canvases by name (autocomplete)
   * 
   * @param {string} query - Search query
   * @returns {Promise<{canvases: array}>} Matching canvases
   */
  async suggest(query) {
    return await this.client._request(`/canvases/suggest?q=${encodeURIComponent(query)}`, {
      method: 'GET'
    });
  }
}

export default CanvasesClient;
