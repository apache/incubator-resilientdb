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
 * Socket.IO client module for Collaborative Drawing SDK
 * 
 * Handles real-time collaboration via Socket.IO.
 */

import { io } from 'socket.io-client';

class SocketClient {
  constructor(client) {
    this.client = client;
    this.socket = null;
    this.connected = false;
    this.currentCanvases = new Set();
  }

  /**
   * Connect to Socket.IO server
   * 
   * @param {string} token - JWT access token for authentication
   * @param {Object} [options={}] - Socket.IO connection options
   * @returns {Promise<void>} Resolves when connected
   */
  connect(token, options = {}) {
    return new Promise((resolve, reject) => {
      if (this.connected) {
        resolve();
        return;
      }

      const socketUrl = this.client.config.baseUrl;

      this.socket = io(socketUrl, {
        auth: {
          token: token
        },
        transports: ['websocket', 'polling'],
        ...options
      });

      this.socket.on('connect', () => {
        this.connected = true;
        resolve();
      });

      this.socket.on('connect_error', (error) => {
        this.connected = false;
        reject(error);
      });

      this.socket.on('disconnect', () => {
        this.connected = false;
      });
    });
  }

  /**
   * Disconnect from Socket.IO server
   */
  disconnect() {
    if (this.socket) {
      this.socket.disconnect();
      this.socket = null;
      this.connected = false;
      this.currentCanvases.clear();
    }
  }

  /**
   * Join a canvas for real-time collaboration
   * 
   * @param {string} canvasId - Canvas ID to join
   */
  joinCanvas(canvasId) {
    if (!this.socket) {
      throw new Error('Socket not connected. Call connect() first.');
    }

    this.socket.emit('join_room', { roomId: canvasId });
    this.currentCanvases.add(canvasId);
  }

  /**
   * Leave a canvas
   * 
   * @param {string} canvasId - Canvas ID to leave
   */
  leaveCanvas(canvasId) {
    if (!this.socket) {
      return;
    }

    this.socket.emit('leave_room', { roomId: canvasId });
    this.currentCanvases.delete(canvasId);
  }

  /**
   * @deprecated Use joinCanvas() instead. This method will be removed in a future version.
   * Join a room for real-time collaboration
   * 
   * @param {string} roomId - Room ID to join
   */
  joinRoom(roomId) {
    console.warn('[DEPRECATED] socket.joinRoom() is deprecated. Please use socket.joinCanvas() instead.');
    return this.joinCanvas(roomId);
  }

  /**
   * @deprecated Use leaveCanvas() instead. This method will be removed in a future version.
   * Leave a room
   * 
   * @param {string} roomId - Room ID to leave
   */
  leaveRoom(roomId) {
    console.warn('[DEPRECATED] socket.leaveRoom() is deprecated. Please use socket.leaveCanvas() instead.');
    return this.leaveCanvas(roomId);
  }

  /**
   * Listen for a Socket.IO event
   * 
   * @param {string} event - Event name
   * @param {Function} callback - Event handler
   * 
   * @example
   * client.socket.on('new_line', (stroke) => {
   *   console.log('New stroke received:', stroke);
   * });
   */
  on(event, callback) {
    if (!this.socket) {
      throw new Error('Socket not connected. Call connect() first.');
    }

    this.socket.on(event, callback);
  }

  /**
   * Remove event listener
   * 
   * @param {string} event - Event name
   * @param {Function} [callback] - Specific handler to remove (optional)
   */
  off(event, callback) {
    if (!this.socket) {
      return;
    }

    if (callback) {
      this.socket.off(event, callback);
    } else {
      this.socket.off(event);
    }
  }

  /**
   * Emit an event to the server
   * 
   * @param {string} event - Event name
   * @param {any} data - Event data
   */
  emit(event, data) {
    if (!this.socket) {
      throw new Error('Socket not connected. Call connect() first.');
    }

    this.socket.emit(event, data);
  }

  /**
   * Check if socket is currently connected
   * 
   * @returns {boolean} Connection status
   */
  isConnected() {
    return this.connected && this.socket?.connected;
  }

  /**
   * Get list of currently joined rooms
   * 
   * @returns {Set<string>} Set of room IDs
   */
  getJoinedRooms() {
    return new Set(this.currentRooms);
  }
}

export default SocketClient;
