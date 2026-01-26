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
 * Query Analyzer
 * 
 * Analyzes GraphQL query structure to identify:
 * - Query complexity
 * - Potential optimization opportunities
 * - Resource usage patterns
 * - Common anti-patterns
 */

export interface QueryAnalysis {
  /** Query string */
  query: string;
  /** Complexity level */
  complexity: 'low' | 'medium' | 'high';
  /** Number of fields requested */
  fieldCount: number;
  /** Depth of nested queries */
  depth: number;
  /** Identified optimization opportunities */
  opportunities: OptimizationOpportunity[];
  /** Estimated resource usage */
  estimatedResourceUsage?: {
    executionTime: number;
    memoryUsage: string;
  };
}

export interface OptimizationOpportunity {
  /** Type of optimization */
  type: 'field_selection' | 'nested_depth' | 'filtering' | 'pagination' | 'caching';
  /** Description of the opportunity */
  description: string;
  /** Severity level */
  severity: 'low' | 'medium' | 'high';
  /** Suggested improvement */
  suggestion?: string;
}

/**
 * Query Analyzer
 * 
 * Analyzes GraphQL queries to identify optimization opportunities
 */
export class QueryAnalyzer {
  /**
   * Analyze a GraphQL query
   */
  analyze(query: string): QueryAnalysis {
    const parsed = this.parseQuery(query);
    
    const fieldCount = this.countFields(parsed);
    const depth = this.calculateDepth(parsed);
    const complexity = this.assessComplexity(fieldCount, depth);
    
    const opportunities = this.identifyOpportunities(parsed, fieldCount, depth);
    
    return {
      query,
      complexity,
      fieldCount,
      depth,
      opportunities,
      estimatedResourceUsage: this.estimateResourceUsage(complexity, fieldCount, depth),
    };
  }

  /**
   * Parse query into a simple structure
   */
  private parseQuery(query: string): {
    operation: 'query' | 'mutation' | 'subscription' | 'unknown';
    fields: string[];
    nested: boolean;
  } {
    const trimmed = query.trim();
    
    // Determine operation type
    let operation: 'query' | 'mutation' | 'subscription' | 'unknown' = 'unknown';
    if (trimmed.startsWith('query') || trimmed.startsWith('{')) {
      operation = 'query';
    } else if (trimmed.startsWith('mutation')) {
      operation = 'mutation';
    } else if (trimmed.startsWith('subscription')) {
      operation = 'subscription';
    }

    // Extract fields (simple regex-based extraction)
    const fieldMatches = query.match(/\w+\s*\{/g) || [];
    const fields = fieldMatches.map(m => m.replace(/\s*\{/, '').trim());

    // Check for nested structures
    const nested = (query.match(/\{/g) || []).length > 2;

    return {
      operation,
      fields,
      nested,
    };
  }

  /**
   * Count fields in query
   */
  private countFields(parsed: { fields: string[] }): number {
    return parsed.fields.length;
  }

  /**
   * Calculate nesting depth
   */
  private calculateDepth(parsed: { nested: boolean; fields: string[] }): number {
    if (!parsed.nested) return 1;
    
    // Simple depth estimation based on braces
    const query = parsed.fields.join(' ');
    const braceMatches = query.match(/\{/g) || [];
    return Math.min(braceMatches.length, 5); // Cap at 5 for practical purposes
  }

  /**
   * Assess query complexity
   */
  private assessComplexity(
    fieldCount: number,
    depth: number
  ): 'low' | 'medium' | 'high' {
    const score = fieldCount * 0.5 + depth * 2;

    if (score < 5) return 'low';
    if (score < 15) return 'medium';
    return 'high';
  }

  /**
   * Identify optimization opportunities
   */
  private identifyOpportunities(
    parsed: { operation: string; fields: string[]; nested: boolean },
    fieldCount: number,
    depth: number
  ): OptimizationOpportunity[] {
    const opportunities: OptimizationOpportunity[] = [];

    // Check for excessive field selection
    if (fieldCount > 10) {
      opportunities.push({
        type: 'field_selection',
        description: `Query selects ${fieldCount} fields. Consider selecting only needed fields.`,
        severity: fieldCount > 20 ? 'high' : 'medium',
        suggestion: 'Use field selection to fetch only required data',
      });
    }

    // Check for deep nesting
    if (depth > 3) {
      opportunities.push({
        type: 'nested_depth',
        description: `Query has depth of ${depth}. Deep nesting can impact performance.`,
        severity: depth > 4 ? 'high' : 'medium',
        suggestion: 'Consider flattening the query structure or using separate queries',
      });
    }

    // Check for missing filters
    if (parsed.operation === 'query' && !this.hasFilters(parsed.fields)) {
      opportunities.push({
        type: 'filtering',
        description: 'Query lacks filtering. Consider adding filters to reduce result size.',
        severity: 'low',
        suggestion: 'Add WHERE clauses or filters to limit results',
      });
    }

    // Check for missing pagination
    if (fieldCount > 5 && !this.hasPagination(parsed.fields)) {
      opportunities.push({
        type: 'pagination',
        description: 'Large query without pagination. May return excessive data.',
        severity: 'medium',
        suggestion: 'Implement pagination with limit/offset or cursor-based pagination',
      });
    }

    // Check for caching opportunities
    if (parsed.operation === 'query' && !this.hasCacheDirectives(parsed.fields)) {
      opportunities.push({
        type: 'caching',
        description: 'Query may benefit from caching.',
        severity: 'low',
        suggestion: 'Consider adding cache directives for frequently accessed data',
      });
    }

    return opportunities;
  }

  /**
   * Check if query has filters
   */
  private hasFilters(fields: string[]): boolean {
    const queryString = fields.join(' ');
    return /\([^)]*\)/.test(queryString) || 
           /where|filter|id:|limit|offset/.test(queryString.toLowerCase());
  }

  /**
   * Check if query has pagination
   */
  private hasPagination(fields: string[]): boolean {
    const queryString = fields.join(' ');
    return /limit|offset|first|last|after|before/.test(queryString.toLowerCase());
  }

  /**
   * Check for cache directives
   */
  private hasCacheDirectives(fields: string[]): boolean {
    const queryString = fields.join(' ');
    return /@cache|@cached/.test(queryString);
  }

  /**
   * Estimate resource usage
   */
  private estimateResourceUsage(
    complexity: 'low' | 'medium' | 'high',
    fieldCount: number,
    depth: number
  ): {
    executionTime: number;
    memoryUsage: string;
  } {
    // Rough estimates based on complexity
    let executionTime = 50; // Base time in ms
    executionTime += fieldCount * 5;
    executionTime += depth * 20;

    if (complexity === 'medium') executionTime *= 1.5;
    if (complexity === 'high') executionTime *= 3;

    const memoryUsage = complexity === 'low' ? '< 10MB' :
                        complexity === 'medium' ? '10-50MB' : '> 50MB';

    return {
      executionTime: Math.round(executionTime),
      memoryUsage,
    };
  }

  /**
   * Compare two queries for similarity
   */
  compareQueries(query1: string, query2: string): number {
    const analysis1 = this.analyze(query1);
    const analysis2 = this.analyze(query2);

    // Simple similarity score based on structure
    let similarity = 0;

    // Field overlap
    const fields1 = new Set(analysis1.query.match(/\w+(?=\s*\{|\s*\(|\s*:)/g) || []);
    const fields2 = new Set(analysis2.query.match(/\w+(?=\s*\{|\s*\(|\s*:)/g) || []);

    const intersection = new Set([...fields1].filter(f => fields2.has(f)));
    const union = new Set([...fields1, ...fields2]);

    similarity += (intersection.size / union.size) * 0.5;

    // Complexity similarity
    if (analysis1.complexity === analysis2.complexity) {
      similarity += 0.3;
    }

    // Depth similarity
    const depthDiff = Math.abs(analysis1.depth - analysis2.depth);
    similarity += (1 - depthDiff / 5) * 0.2;

    return Math.min(similarity, 1.0);
  }
}

