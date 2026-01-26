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

import { ResLensClient } from '../reslens/client';
import { QueryExecutionMetrics } from '../types';
import env from '../config/environment';

/**
 * Live Stats Service
 * 
 * Polls ResLens API for real-time query execution metrics.
 * Caches statistics and provides them to LLM services.
 * 
 * Features:
 * - Real-time polling of ResLens metrics
 * - Caching for performance
 * - Query pattern matching
 * - Live statistics aggregation
 * 
 * Reference: https://beacon.resilientdb.com/docs/reslens
 */
export interface LiveStats {
  /** Current system metrics */
  systemMetrics?: {
    cpu?: number; // Percentage
    memory?: number; // MB
    network?: number; // KB
  };
  /** Recent query performance */
  recentQueries?: QueryExecutionMetrics[];
  /** Average execution time */
  avgExecutionTime?: number;
  /** Query throughput (queries per second) */
  throughput?: number;
  /** Cache hit ratio */
  cacheHitRatio?: number;
  /** Last update timestamp */
  lastUpdated: Date;
}

export interface LiveStatsOptions {
  /** Enable live polling */
  enabled?: boolean;
  /** Polling interval in milliseconds */
  pollInterval?: number;
  /** Maximum number of recent queries to track */
  maxRecentQueries?: number;
  /** Query pattern to filter metrics */
  queryPattern?: string;
}

export class LiveStatsService {
  private resLensClient: ResLensClient;
  private pollInterval?: NodeJS.Timeout;
  private cachedStats: LiveStats;
  private options: Required<LiveStatsOptions>;
  private isPolling: boolean;

  constructor(options: LiveStatsOptions = {}) {
    this.resLensClient = new ResLensClient();
    this.cachedStats = {
      lastUpdated: new Date(),
    };
    this.isPolling = false;
    
    this.options = {
      enabled: options.enabled ?? (env.RESLENS_LIVE_MODE || false),
      pollInterval: options.pollInterval ?? (env.RESLENS_POLL_INTERVAL || 5000),
      maxRecentQueries: options.maxRecentQueries ?? 50,
      queryPattern: options.queryPattern ?? '',
    };
  }

  /**
   * Start polling for live statistics
   */
  async startPolling(): Promise<void> {
    if (this.isPolling) {
      console.log('Live Stats Service: Already polling');
      return;
    }

    if (!this.options.enabled) {
      console.log('Live Stats Service: Live Mode disabled');
      return;
    }

    // Check ResLens availability
    const isAvailable = await this.resLensClient.healthCheck();
    if (!isAvailable) {
      console.warn('Live Stats Service: ResLens not available, polling disabled');
      return;
    }

    console.log(`Live Stats Service: Starting polling (interval: ${this.options.pollInterval}ms)`);
    this.isPolling = true;

    // Initial poll
    await this.poll();

    // Set up periodic polling
    this.pollInterval = setInterval(async () => {
      await this.poll();
    }, this.options.pollInterval);
  }

  /**
   * Stop polling for live statistics
   */
  stopPolling(): void {
    if (this.pollInterval) {
      clearInterval(this.pollInterval);
      this.pollInterval = undefined;
    }
    this.isPolling = false;
    console.log('Live Stats Service: Polling stopped');
  }

  /**
   * Poll ResLens for latest statistics
   */
  private async poll(): Promise<void> {
    try {
      // Get recent queries
      const recentQueries = await this.resLensClient.getRecentQueries(
        this.options.maxRecentQueries
      );

      // Filter by pattern if specified
      let filteredQueries = recentQueries;
      if (this.options.queryPattern) {
        filteredQueries = recentQueries.filter((q) =>
          q.query.includes(this.options.queryPattern)
        );
      }

      // Calculate statistics
      const avgExecutionTime =
        filteredQueries.length > 0
          ? filteredQueries.reduce((sum, q) => sum + q.executionTime, 0) /
            filteredQueries.length
          : undefined;

      // Calculate throughput (queries per second) based on recent queries
      const throughput = this.calculateThroughput(filteredQueries);

      // Calculate cache hit ratio
      const cacheHitRatio = this.calculateCacheHitRatio(filteredQueries);

      // Get system metrics from recent queries (if available)
      const systemMetrics = this.extractSystemMetrics(filteredQueries);

      // Update cached stats
      this.cachedStats = {
        systemMetrics,
        recentQueries: filteredQueries.slice(0, this.options.maxRecentQueries),
        avgExecutionTime: avgExecutionTime ? Math.round(avgExecutionTime) : undefined,
        throughput,
        cacheHitRatio,
        lastUpdated: new Date(),
      };
    } catch (error) {
      console.warn('Live Stats Service: Polling error:', 
        error instanceof Error ? error.message : String(error));
    }
  }

  /**
   * Get current live statistics
   */
  getLiveStats(): LiveStats {
    return this.cachedStats;
  }

  /**
   * Get statistics for a specific query pattern
   */
  async getQueryStats(queryPattern: string): Promise<QueryExecutionMetrics[]> {
    return await this.resLensClient.getQueryStats(queryPattern);
  }

  /**
   * Get metrics for a specific query ID
   */
  async getQueryMetrics(queryId: string): Promise<QueryExecutionMetrics | null> {
    return await this.resLensClient.getMetrics(queryId);
  }

  /**
   * Get recent queries matching a pattern
   */
  async getRecentQueries(
    pattern?: string,
    limit: number = 10
  ): Promise<QueryExecutionMetrics[]> {
    const queries = await this.resLensClient.getRecentQueries(limit);
    
    if (pattern) {
      return queries.filter((q) => q.query.includes(pattern));
    }
    
    return queries;
  }

  /**
   * Calculate throughput (queries per second) from recent queries
   */
  private calculateThroughput(queries: QueryExecutionMetrics[]): number | undefined {
    if (queries.length < 2) {
      return undefined;
    }

    // Sort by timestamp
    const sorted = queries
      .filter((q) => q.timestamp)
      .sort((a, b) => a.timestamp.getTime() - b.timestamp.getTime());

    if (sorted.length < 2) {
      return undefined;
    }

    const first = sorted[0].timestamp;
    const last = sorted[sorted.length - 1].timestamp;
    const timeSpan = (last.getTime() - first.getTime()) / 1000; // seconds

    if (timeSpan <= 0) {
      return undefined;
    }

    return Math.round((sorted.length / timeSpan) * 10) / 10; // Round to 1 decimal
  }

  /**
   * Calculate cache hit ratio from recent queries
   */
  private calculateCacheHitRatio(queries: QueryExecutionMetrics[]): number | undefined {
    if (queries.length === 0) {
      return undefined;
    }

    const cacheHits = queries.filter((q) => q.cacheHit === true).length;
    return Math.round((cacheHits / queries.length) * 100 * 10) / 10; // Percentage, 1 decimal
  }

  /**
   * Extract system metrics from query execution metrics
   */
  private extractSystemMetrics(
    queries: QueryExecutionMetrics[]
  ): LiveStats['systemMetrics'] {
    if (queries.length === 0) {
      return undefined;
    }

    // Get average resource usage from queries that have it
    const queriesWithResources = queries.filter((q) => q.resourceUsage);

    if (queriesWithResources.length === 0) {
      return undefined;
    }

    const avgCpu =
      queriesWithResources.reduce(
        (sum, q) => sum + (q.resourceUsage?.cpu || 0),
        0
      ) / queriesWithResources.length;

    const avgMemory =
      queriesWithResources.reduce(
        (sum, q) => sum + (q.resourceUsage?.memory || 0),
        0
      ) / queriesWithResources.length;

    const avgNetwork =
      queriesWithResources.reduce(
        (sum, q) => sum + (q.resourceUsage?.network || 0),
        0
      ) / queriesWithResources.length;

    return {
      cpu: Math.round(avgCpu * 10) / 10,
      memory: Math.round(avgMemory * 10) / 10,
      network: Math.round(avgNetwork * 10) / 10,
    };
  }

  /**
   * Check if live stats service is available and polling
   */
  isAvailable(): boolean {
    return this.isPolling && this.options.enabled;
  }

  /**
   * Check ResLens availability
   */
  async checkAvailability(): Promise<boolean> {
    return await this.resLensClient.healthCheck();
  }

  /**
   * Update polling options
   */
  updateOptions(options: Partial<LiveStatsOptions>): void {
    const wasPolling = this.isPolling;
    
    if (wasPolling) {
      this.stopPolling();
    }

    this.options = {
      ...this.options,
      ...options,
    };

    if (wasPolling && this.options.enabled) {
      this.startPolling().catch((error) => {
        console.error('Failed to restart polling:', error);
      });
    }
  }
}

