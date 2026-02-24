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

import { QueryExecutionMetrics } from '../types';
import env from '../config/environment';

/**
 * ResLens Client
 * 
 * Integrates with ResLens Middleware HTTP API (when available).
 * Falls back to local in-memory storage when API is not configured.
 * 
 * ResLens Middleware provides:
 * - HTTP endpoints for querying metrics
 * - Real-time stats from Pyroscope, Prometheus exporters, and LevelDB hooks
 * - Live Mode: Real-time polling of system metrics
 * 
 * Reference: https://beacon.resilientdb.com/docs/reslens
 */
export class ResLensClient {
  private metricsStorage: Map<string, QueryExecutionMetrics>;
  private recentQueries: QueryExecutionMetrics[];
  private apiUrl?: string;
  private apiKey?: string;
  private useApi: boolean;

  constructor() {
    this.metricsStorage = new Map();
    this.recentQueries = [];
    this.apiUrl = env.RESLENS_API_URL;
    this.apiKey = env.RESLENS_API_KEY;
    this.useApi = !!this.apiUrl;
  }

  /**
   * Capture query execution metrics
   * 
   * Sends metrics to ResLens Middleware API if configured, otherwise stores locally.
   */
  async captureMetrics(
    queryId: string,
    query: string,
    executionTime: number,
    success: boolean,
    additionalData?: Partial<QueryExecutionMetrics>
  ): Promise<QueryExecutionMetrics> {
    const metrics: QueryExecutionMetrics = {
      queryId,
      query,
      executionTime,
      timestamp: new Date(),
      success,
      ...additionalData,
    };

    // Always store locally for fallback
    this.metricsStorage.set(queryId, metrics);
    this.recentQueries.unshift(metrics);
    
    // Keep only last 100 queries
    if (this.recentQueries.length > 100) {
      this.recentQueries = this.recentQueries.slice(0, 100);
    }

    // Send to ResLens API if configured
    if (this.useApi && this.apiUrl) {
      try {
        const headers: Record<string, string> = {
          'Content-Type': 'application/json',
        };
        if (this.apiKey) {
          headers['Authorization'] = `Bearer ${this.apiKey}`;
        }

        const response = await fetch(`${this.apiUrl}/api/metrics`, {
          method: 'POST',
          headers,
          body: JSON.stringify(metrics),
        });

        if (!response.ok) {
          console.warn(`ResLens API returned ${response.status}, using local storage`);
        }
      } catch (error) {
        console.warn('Failed to send metrics to ResLens API, using local storage:', 
          error instanceof Error ? error.message : String(error));
      }
    }

    return metrics;
  }

  /**
   * Retrieve query execution metrics by query ID
   * Queries ResLens API if configured, otherwise uses local storage.
   */
  async getMetrics(queryId: string): Promise<QueryExecutionMetrics | null> {
    // Try API first if configured
    if (this.useApi && this.apiUrl) {
      try {
        const headers: Record<string, string> = {};
        if (this.apiKey) {
          headers['Authorization'] = `Bearer ${this.apiKey}`;
        }

        const response = await fetch(`${this.apiUrl}/api/metrics/query/${queryId}`, {
          method: 'GET',
          headers,
        });

        if (response.ok) {
          const metrics = await response.json();
          // Convert timestamp string to Date if needed
          if (metrics.timestamp && typeof metrics.timestamp === 'string') {
            metrics.timestamp = new Date(metrics.timestamp);
          }
          return metrics as QueryExecutionMetrics;
        } else if (response.status === 404) {
          return null;
        }
      } catch (error) {
        console.warn('Failed to fetch metrics from ResLens API, using local storage:',
          error instanceof Error ? error.message : String(error));
      }
    }

    // Fallback to local storage
    return this.metricsStorage.get(queryId) || null;
  }

  /**
   * Get performance statistics for a query pattern
   * Queries ResLens API if configured, otherwise uses local storage.
   */
  async getQueryStats(queryPattern: string): Promise<QueryExecutionMetrics[]> {
    // Try API first if configured
    if (this.useApi && this.apiUrl) {
      try {
        const headers: Record<string, string> = {};
        if (this.apiKey) {
          headers['Authorization'] = `Bearer ${this.apiKey}`;
        }

        const url = new URL(`${this.apiUrl}/api/stats/query`);
        url.searchParams.set('pattern', queryPattern);

        const response = await fetch(url.toString(), {
          method: 'GET',
          headers,
        });

        if (response.ok) {
          const metrics = await response.json();
          // Convert timestamp strings to Date if needed
          if (Array.isArray(metrics)) {
            return metrics.map((m: any) => ({
              ...m,
              timestamp: m.timestamp ? new Date(m.timestamp) : new Date(),
            })) as QueryExecutionMetrics[];
          }
          return [];
        }
      } catch (error) {
        console.warn('Failed to fetch query stats from ResLens API, using local storage:',
          error instanceof Error ? error.message : String(error));
      }
    }

    // Fallback to local storage
    const allMetrics = Array.from(this.metricsStorage.values());
    return allMetrics.filter((m) => m.query.includes(queryPattern));
  }

  /**
   * Get recent query executions
   * Queries ResLens API if configured, otherwise uses local storage.
   */
  async getRecentQueries(limit: number = 10): Promise<QueryExecutionMetrics[]> {
    // Try API first if configured
    if (this.useApi && this.apiUrl) {
      try {
        const headers: Record<string, string> = {};
        if (this.apiKey) {
          headers['Authorization'] = `Bearer ${this.apiKey}`;
        }

        const url = new URL(`${this.apiUrl}/api/metrics/recent`);
        url.searchParams.set('limit', limit.toString());

        const response = await fetch(url.toString(), {
          method: 'GET',
          headers,
        });

        if (response.ok) {
          const metrics = await response.json();
          // Convert timestamp strings to Date if needed
          if (Array.isArray(metrics)) {
            return metrics.map((m: any) => ({
              ...m,
              timestamp: m.timestamp ? new Date(m.timestamp) : new Date(),
            })) as QueryExecutionMetrics[];
          }
          return [];
        }
      } catch (error) {
        console.warn('Failed to fetch recent queries from ResLens API, using local storage:',
          error instanceof Error ? error.message : String(error));
      }
    }

    // Fallback to local storage
    return this.recentQueries.slice(0, limit);
  }

  /**
   * Check if ResLens is available
   * Checks ResLens API health if configured, otherwise returns true (local storage available).
   */
  async healthCheck(): Promise<boolean> {
    // If API is configured, check its health
    if (this.useApi && this.apiUrl) {
      try {
        const headers: Record<string, string> = {};
        if (this.apiKey) {
          headers['Authorization'] = `Bearer ${this.apiKey}`;
        }

        const response = await fetch(`${this.apiUrl}/api/health`, {
          method: 'GET',
          headers,
        });

        return response.ok;
      } catch {
        return false;
      }
    }

    // Local storage is always available
    return true;
  }

  /**
   * Clear local metrics storage (useful for testing)
   */
  clearStorage(): void {
    this.metricsStorage.clear();
    this.recentQueries = [];
  }

  /**
   * Get all stored metrics (useful for debugging)
   */
  getAllMetrics(): QueryExecutionMetrics[] {
    return Array.from(this.metricsStorage.values());
  }
}

