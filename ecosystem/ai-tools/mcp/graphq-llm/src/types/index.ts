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
 * Core type definitions for GraphQ-LLM
 */

export interface GraphQLQuery {
  query: string;
  variables?: Record<string, unknown>;
  operationName?: string;
}

export interface QueryExecutionMetrics {
  queryId: string;
  query: string;
  executionTime: number;
  timestamp: Date;
  success: boolean;
  error?: string;
  resultSize?: number;
  cacheHit?: boolean;
  resourceUsage?: {
    cpu?: number;
    memory?: number;
    network?: number;
  };
}

export interface QuerySuggestion {
  query: string;
  explanation: string;
  confidence: number;
  optimization?: {
    reason: string;
    estimatedImprovement?: string;
  };
}

export interface QueryAnalysis {
  query: string;
  explanation: string;
  complexity?: 'low' | 'medium' | 'high';
  recommendations?: string[];
  estimatedEfficiency?: number;
}

export interface SchemaContext {
  schema: string;
  types: GraphQLType[];
  queries: GraphQLField[];
  mutations: GraphQLField[];
}

export interface GraphQLType {
  name: string;
  kind: string;
  description?: string;
  fields?: GraphQLField[];
}

export interface GraphQLField {
  name: string;
  type: string;
  description?: string;
  args?: GraphQLArgument[];
}

export interface GraphQLArgument {
  name: string;
  type: string;
  description?: string;
  defaultValue?: unknown;
}

export interface MCPContext {
  query?: GraphQLQuery;
  schema?: SchemaContext;
  metrics?: QueryExecutionMetrics;
  documentation?: string[];
}

export interface PipelineData {
  query: GraphQLQuery;
  metrics?: QueryExecutionMetrics;
  schemaContext?: SchemaContext;
  timestamp: Date;
}

