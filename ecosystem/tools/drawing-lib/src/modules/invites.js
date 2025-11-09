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
 * Invitations module for Collaborative Drawing SDK
 * 
 * Handles room invitation management.
 */

class InvitesClient {
  constructor(client) {
    this.client = client;
  }

  /**
   * List all pending invitations for current user
   * 
   * @returns {Promise<{invites: array}>} List of invitations
   */
  async list() {
    return await this.client._request('/collaborations/invitations', {
      method: 'GET'
    });
  }

  /**
   * Accept a room invitation
   * 
   * @param {string} inviteId - Invitation ID
   * @returns {Promise<{status: string, room: object}>} Acceptance result with room details
   */
  async accept(inviteId) {
    return await this.client._request(`/collaborations/invitations/${inviteId}/accept`, {
      method: 'POST'
    });
  }

  /**
   * Decline a room invitation
   * 
   * @param {string} inviteId - Invitation ID
   * @returns {Promise<{status: string}>} Decline status
   */
  async decline(inviteId) {
    return await this.client._request(`/collaborations/invitations/${inviteId}/decline`, {
      method: 'POST'
    });
  }
}

export default InvitesClient;
