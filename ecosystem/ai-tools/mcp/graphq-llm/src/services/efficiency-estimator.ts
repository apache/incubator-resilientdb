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
import { QueryAnalyzer, QueryAnalysis } from './query-analyzer';
import env from '../config/environment';

/**
 * Efficiency Estimator Service
 * 
 * Estimates query performance and efficiency using:
 * - Query structure analysis (complexity, depth, field count)
 * - Historical ResLens metrics (similar queries)
 * - Live Mode stats (real-time system metrics)
 * 
 * Provides:
 * - Execution time estimation
 * - Resource usage estimation
 * - Efficiency score (0-100)
 * - Performance predictions
 */
export interface EfficiencyEstimate {
  /** Estimated execution time in milliseconds */
  estimatedExecutionTime: number;
  /** Estimated resource usage */
  estimatedResourceUsage: {
    cpu?: number; // Percentage
    memory?: number; // MB
    network?: number; // KB
  };
  /** Efficiency score (0-100, higher is better) */
  efficiencyScore: number;
  /** Confidence level of the estimate (0-1) */
  confidence: number;
  /** Comparison with similar queries */
  similarQueries?: Array<{
    query: string;
    executionTime: number;
    similarity: number;
  }>;
  /** Performance category */
  performanceCategory: 'excellent' | 'good' | 'moderate' | 'poor' | 'critical';
  /** Recommendations for improvement */
  recommendations?: string[];
}

export interface EstimationOptions {
  /** Include similar queries in estimation */
  includeSimilarQueries?: boolean;
  /** Use Live Mode stats if available */
  useLiveStats?: boolean;
  /** Minimum confidence threshold */
  minConfidence?: number;
}

export class EfficiencyEstimator {
  private resLensClient: ResLensClient;
  private queryAnalyzer: QueryAnalyzer;

  constructor() {
    this.resLensClient = new ResLensClient();
    this.queryAnalyzer = new QueryAnalyzer();
  }

  /**
   * Estimate efficiency for a GraphQL query
   * 
   * @param query The GraphQL query string
   * @param options Estimation options
   * @returns Efficiency estimate with predictions
   */
  async estimateEfficiency(
    query: string,
    options: EstimationOptions = {}
  ): Promise<EfficiencyEstimate> {
    const {
      includeSimilarQueries = true,
      useLiveStats = env.RESLENS_LIVE_MODE || false,
    } = options;

    // Step 1: Analyze query structure
    const analysis = this.queryAnalyzer.analyze(query);

    // Step 2: Get similar queries from ResLens (if enabled)
    let similarQueries: Array<{
      query: string;
      executionTime: number;
      similarity: number;
    }> = [];

    if (includeSimilarQueries) {
      try {
        // Extract query pattern (operation name or first field)
        const queryPattern = this.extractQueryPattern(query);
        const similarMetrics = await this.resLensClient.getQueryStats(queryPattern);

        // Calculate similarity and filter
        similarQueries = similarMetrics
          .map((metric) => ({
            query: metric.query,
            executionTime: metric.executionTime,
            similarity: this.calculateQuerySimilarity(query, metric.query),
          }))
          .filter((sq) => sq.similarity >= 0.3) // Minimum similarity threshold
          .sort((a, b) => b.similarity - a.similarity)
          .slice(0, 5); // Top 5 similar queries
      } catch (error) {
        console.warn('Failed to get similar queries from ResLens:', 
          error instanceof Error ? error.message : String(error));
      }
    }

    // Step 3: Estimate execution time
    const estimatedExecutionTime = this.estimateExecutionTime(analysis, similarQueries);

    // Step 4: Estimate resource usage
    const estimatedResourceUsage = this.estimateResourceUsage(analysis, estimatedExecutionTime);

    // Step 5: Calculate efficiency score
    const efficiencyScore = this.calculateEfficiencyScore(
      analysis,
      estimatedExecutionTime,
      estimatedResourceUsage
    );

    // Step 6: Calculate confidence
    const confidence = this.calculateConfidence(analysis, similarQueries, useLiveStats);

    // Step 7: Determine performance category
    const performanceCategory = this.determinePerformanceCategory(
      efficiencyScore,
      estimatedExecutionTime
    );

    // Step 8: Generate recommendations
    const recommendations = this.generateRecommendations(
      analysis,
      efficiencyScore,
      estimatedExecutionTime
    );

    return {
      estimatedExecutionTime,
      estimatedResourceUsage,
      efficiencyScore,
      confidence,
      similarQueries: similarQueries.length > 0 ? similarQueries : undefined,
      performanceCategory,
      recommendations: recommendations.length > 0 ? recommendations : undefined,
    };
  }

  /**
   * Extract query pattern for matching similar queries
   */
  private extractQueryPattern(query: string): string {
    // Try to extract operation name
    const operationMatch = query.match(/(?:query|mutation|subscription)\s+(\w+)/i);
    if (operationMatch) {
      return operationMatch[1];
    }

    // Extract first field name
    const fieldMatch = query.match(/\{?\s*(\w+)/);
    if (fieldMatch) {
      return fieldMatch[1];
    }

    return query.substring(0, 50); // Fallback: first 50 chars
  }

  /**
   * Calculate similarity between two queries (simple heuristic)
   */
  private calculateQuerySimilarity(query1: string, query2: string): number {
    // Normalize queries (remove whitespace, lowercase)
    const norm1 = query1.toLowerCase().replace(/\s+/g, ' ').trim();
    const norm2 = query2.toLowerCase().replace(/\s+/g, ' ').trim();

    // Simple word overlap similarity
    const words1 = new Set(norm1.split(/\s+/));
    const words2 = new Set(norm2.split(/\s+/));

    const intersection = new Set([...words1].filter((w) => words2.has(w)));
    const union = new Set([...words1, ...words2]);

    return intersection.size / union.size;
  }

  /**
   * Estimate execution time based on query analysis and similar queries
   */
  private estimateExecutionTime(
    analysis: QueryAnalysis,
    similarQueries: Array<{ executionTime: number; similarity: number }>
  ): number {
    // Base estimation from query complexity
    let baseTime = 0;

    // Base time per field
    baseTime += analysis.fieldCount * 10; // 10ms per field

    // Depth penalty (nested queries are slower)
    baseTime += analysis.depth * 20; // 20ms per depth level

    // Complexity penalty (low/medium/high)
    if (analysis.complexity === 'high') {
      baseTime += 50;
    } else if (analysis.complexity === 'medium') {
      baseTime += 25;
    }

    // Adjust based on similar queries if available
    if (similarQueries.length > 0) {
      // Weighted average of similar queries
      const totalSimilarity = similarQueries.reduce((sum, sq) => sum + sq.similarity, 0);
      const weightedTime = similarQueries.reduce(
        (sum, sq) => sum + sq.executionTime * sq.similarity,
        0
      );

      if (totalSimilarity > 0) {
        const avgSimilarTime = weightedTime / totalSimilarity;
        // Blend: 70% similar query average, 30% base estimate
        return Math.round(avgSimilarTime * 0.7 + baseTime * 0.3);
      }
    }

    return Math.round(baseTime);
  }

  /**
   * Estimate resource usage based on query complexity and execution time
   */
  private estimateResourceUsage(
    analysis: QueryAnalysis,
    executionTime: number
  ): {
    cpu?: number;
    memory?: number;
    network?: number;
  } {
    // CPU: Higher for complex queries and longer execution
    const cpu = Math.min(100, (analysis.fieldCount * 2) + (executionTime / 10));

    // Memory: Higher for deep queries and many fields
    const memory = analysis.fieldCount * 5 + analysis.depth * 10;

    // Network: Higher for queries that fetch many fields
    const network = analysis.fieldCount * 2;

    return {
      cpu: Math.round(cpu),
      memory: Math.round(memory),
      network: Math.round(network),
    };
  }

  /**
   * Calculate efficiency score (0-100, higher is better)
   */
  private calculateEfficiencyScore(
    analysis: QueryAnalysis,
    executionTime: number,
    resourceUsage: { cpu?: number; memory?: number; network?: number }
  ): number {
    let score = 100;

    // Penalize long execution time
    if (executionTime > 1000) {
      score -= 30; // Very slow
    } else if (executionTime > 500) {
      score -= 20; // Slow
    } else if (executionTime > 200) {
      score -= 10; // Moderate
    }

    // Penalize high resource usage
    if (resourceUsage.cpu && resourceUsage.cpu > 80) {
      score -= 15;
    } else if (resourceUsage.cpu && resourceUsage.cpu > 50) {
      score -= 10;
    }

    if (resourceUsage.memory && resourceUsage.memory > 100) {
      score -= 15;
    } else if (resourceUsage.memory && resourceUsage.memory > 50) {
      score -= 10;
    }

    // Penalize query complexity
    if (analysis.fieldCount > 30) {
      score -= 20; // Too many fields
    } else if (analysis.fieldCount > 15) {
      score -= 10;
    }

    if (analysis.depth > 5) {
      score -= 20; // Too deep
    } else if (analysis.depth > 3) {
      score -= 10;
    }

    // Penalize high complexity level
    if (analysis.complexity === 'high') {
      score -= 15;
    } else if (analysis.complexity === 'medium') {
      score -= 10;
    }

    // Penalize optimization opportunities
    score -= Math.min(20, analysis.opportunities.length * 5);

    return Math.max(0, Math.min(100, score));
  }

  /**
   * Calculate confidence in the estimate (0-1)
   */
  private calculateConfidence(
    analysis: QueryAnalysis,
    similarQueries: Array<unknown>,
    useLiveStats: boolean
  ): number {
    let confidence = 0.5; // Base confidence

    // Higher confidence if we have similar queries
    if (similarQueries.length > 0) {
      confidence += 0.3;
    }

    // Higher confidence if query is simple (more predictable)
    if (analysis.fieldCount < 10 && analysis.depth < 3) {
      confidence += 0.1;
    }

    // Higher confidence if Live Mode is enabled (real-time data)
    if (useLiveStats) {
      confidence += 0.1;
    }

    return Math.min(1, confidence);
  }

  /**
   * Determine performance category
   */
  private determinePerformanceCategory(
    efficiencyScore: number,
    executionTime: number
  ): 'excellent' | 'good' | 'moderate' | 'poor' | 'critical' {
    if (efficiencyScore >= 80 && executionTime < 200) {
      return 'excellent';
    }
    if (efficiencyScore >= 60 && executionTime < 500) {
      return 'good';
    }
    if (efficiencyScore >= 40 && executionTime < 1000) {
      return 'moderate';
    }
    if (efficiencyScore >= 20) {
      return 'poor';
    }
    return 'critical';
  }

  /**
   * Generate recommendations for improvement
   */
  private generateRecommendations(
    analysis: QueryAnalysis,
    efficiencyScore: number,
    executionTime: number
  ): string[] {
    const recommendations: string[] = [];

    if (efficiencyScore < 60) {
      recommendations.push('Consider optimizing this query to improve performance');
    }

    if (executionTime > 1000) {
      recommendations.push('Query execution time is high - consider adding filters or pagination');
    }

    if (analysis.fieldCount > 20) {
      recommendations.push('Reduce the number of fields requested (over-fetching)');
    }

    if (analysis.depth > 5) {
      recommendations.push('Reduce query depth to avoid N+1 problems');
    }

    // Add specific optimization opportunities
    if (analysis.opportunities.length > 0) {
      recommendations.push(...analysis.opportunities.map(opp => opp.description).slice(0, 3));
    }

    return recommendations;
  }

  /**
   * Check service availability
   */
  async checkAvailability(): Promise<{
    resLens: boolean;
    queryAnalyzer: boolean;
  }> {
    const resLensAvailable = await this.resLensClient.healthCheck();
    return {
      resLens: resLensAvailable,
      queryAnalyzer: true, // Local utility, always available
    };
  }
}

