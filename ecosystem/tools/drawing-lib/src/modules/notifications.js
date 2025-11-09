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
 * Notifications module for Collaborative Drawing SDK
 * 
 * Handles user notification management.
 */

class NotificationsClient {
  constructor(client) {
    this.client = client;
  }

  /**
   * List all notifications for current user
   * 
   * @returns {Promise<{notifications: array}>} List of notifications
   */
  async list() {
    return await this.client._request('/notifications', {
      method: 'GET'
    });
  }

  /**
   * Mark a notification as read
   * 
   * @param {string} notificationId - Notification ID
   * @returns {Promise<{status: string}>} Mark read status
   */
  async markRead(notificationId) {
    return await this.client._request(`/notifications/${notificationId}/mark-read`, {
      method: 'POST'
    });
  }

  /**
   * Delete a specific notification
   * 
   * @param {string} notificationId - Notification ID
   * @returns {Promise<{status: string}>} Deletion status
   */
  async delete(notificationId) {
    return await this.client._request(`/notifications/${notificationId}`, {
      method: 'DELETE'
    });
  }

  /**
   * Delete all notifications for current user
   * 
   * @returns {Promise<{status: string}>} Clear status
   */
  async clear() {
    return await this.client._request('/notifications', {
      method: 'DELETE'
    });
  }

  /**
   * Get notification preferences for current user
   * 
   * @returns {Promise<{preferences: object}>} Notification preferences
   */
  async getPreferences() {
    return await this.client._request('/notifications/preferences', {
      method: 'GET'
    });
  }

  /**
   * Update notification preferences
   * 
   * @param {Object} preferences - Notification preferences to update
   * @param {boolean} [preferences.roomInvites] - Receive room invitation notifications
   * @param {boolean} [preferences.mentions] - Receive mention notifications
   * @param {boolean} [preferences.roomActivity] - Receive room activity notifications
   * @returns {Promise<{status: string, preferences: object}>} Update result
   */
  async updatePreferences(preferences) {
    return await this.client._request('/notifications/preferences', {
      method: 'PATCH',
      body: JSON.stringify(preferences)
    });
  }
}

export default NotificationsClient;
