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

