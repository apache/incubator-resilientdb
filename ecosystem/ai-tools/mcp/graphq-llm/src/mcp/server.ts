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

// MCP SDK imports - adjust based on actual package structure
// Note: The actual import paths may vary depending on the MCP SDK version
// @ts-ignore - MCP SDK types may need adjustment
import { Server } from '@modelcontextprotocol/sdk/server/index.js';
// @ts-ignore
import { StdioServerTransport } from '@modelcontextprotocol/sdk/server/stdio.js';
// @ts-ignore
import {
  CallToolRequestSchema,
  ListToolsRequestSchema,
  ListResourcesRequestSchema,
  ReadResourceRequestSchema,
} from '@modelcontextprotocol/sdk/types.js';
import { ResilientDBClient } from '../resilientdb/client';
import { ResLensClient } from '../reslens/client';
import { EfficiencyEstimator } from '../services/efficiency-estimator';
import { LiveStatsService } from '../services/live-stats-service';
import { ExplanationService } from '../llm/explanation-service';
import { OptimizationService } from '../llm/optimization-service';
import { GraphQLQuery } from '../types';

/**
 * MCP Server for GraphQ-LLM
 * Provides secure two-way communication between ResilientDB and AI tools
 */
export class MCPServer {
  private server: Server;
  private resilientDBClient: ResilientDBClient;
  private resLensClient: ResLensClient;
  private efficiencyEstimator: EfficiencyEstimator;
  private liveStatsService: LiveStatsService;
  private explanationService: ExplanationService;
  private optimizationService: OptimizationService;

  constructor() {
    this.server = new Server(
      {
        name: 'graphq-llm-mcp-server',
        version: '0.1.0',
      },
      {
        capabilities: {
          tools: {},
          resources: {},
        },
      }
    );

    this.resilientDBClient = new ResilientDBClient();
    this.resLensClient = new ResLensClient();
    this.efficiencyEstimator = new EfficiencyEstimator();
    this.liveStatsService = new LiveStatsService();
    this.explanationService = new ExplanationService();
    this.optimizationService = new OptimizationService();

    this.setupHandlers();
  }

  private setupHandlers(): void {
    // List available tools
    this.server.setRequestHandler(ListToolsRequestSchema, async () => ({
      tools: [
        {
          name: 'check_connection',
          description: `Check the health and connectivity of the ResilientDB GraphQL endpoint.
          
Example usage:
{
  // No parameters required
}

Returns connection status, endpoint URL, response time, and any errors.`,
          inputSchema: {
            type: 'object',
            properties: {},
          },
        },
        {
          name: 'execute_graphql_query',
          description: `Execute a GraphQL query against ResilientDB.
          
Example usage:
{
  "query": "{ getTransaction(id: \"123\") { asset } }",
  "variables": {},
  "operationName": "GetTransaction"
}

Returns the query result as JSON.`,
          inputSchema: {
            type: 'object',
            properties: {
              query: { 
                type: 'string', 
                description: 'GraphQL query string (required)' 
              },
              variables: {
                type: 'object',
                description: 'Query variables (optional)',
              },
              operationName: {
                type: 'string',
                description: 'Operation name (optional)',
              },
            },
            required: ['query'],
          },
        },
        {
          name: 'introspect_schema',
          description: `Get the complete GraphQL schema from ResilientDB, including all types, queries, and mutations.
          
Example usage:
{
  // No parameters required
}

Returns the full schema structure including query types, mutation types, and all available fields.`,
          inputSchema: {
            type: 'object',
            properties: {},
          },
        },
        {
          name: 'get_query_metrics',
          description: `Get execution metrics for a specific query from ResLens.
          
Example usage:
{
  "queryId": "query-123-abc"
}

Returns metrics including execution time, success status, timestamp, and any additional performance data.`,
          inputSchema: {
            type: 'object',
            properties: {
              queryId: { 
                type: 'string', 
                description: 'Query ID to retrieve metrics for (required)' 
              },
            },
            required: ['queryId'],
          },
        },
        {
          name: 'validate_query',
          description: `Validate a GraphQL query syntax before execution.
          
Example usage:
{
  "query": "{ getTransaction(id: \"123\") { asset } }"
}

Returns validation result with any syntax errors found.`,
          inputSchema: {
            type: 'object',
            properties: {
              query: { 
                type: 'string', 
                description: 'GraphQL query to validate (required)' 
              },
            },
            required: ['query'],
          },
        },
        {
          name: 'estimate_query_efficiency',
          description: `Estimate query performance and efficiency using structural analysis and ResLens metrics.
          
Example usage:
{
  "query": "{ getTransaction(id: \"123\") { asset } }",
  "includeSimilarQueries": true,
  "useLiveStats": false
}

Returns efficiency score (0-100), predicted execution time, resource usage, and optimization recommendations.`,
          inputSchema: {
            type: 'object',
            properties: {
              query: { 
                type: 'string', 
                description: 'GraphQL query to estimate (required)' 
              },
              includeSimilarQueries: { 
                type: 'boolean', 
                description: 'Include similar queries in estimation (default: true)' 
              },
              useLiveStats: { 
                type: 'boolean', 
                description: 'Use Live Mode stats if available (default: false)' 
              },
            },
            required: ['query'],
          },
        },
        {
          name: 'get_live_stats',
          description: `Get real-time statistics from ResLens (Live Mode).
          
Example usage:
{
  "queryPattern": "getTransaction"
}

Returns current throughput, cache hit ratio, recent queries, and system metrics. Requires ResLens Live Mode to be enabled.`,
          inputSchema: {
            type: 'object',
            properties: {
              queryPattern: { 
                type: 'string', 
                description: 'Optional query pattern to filter stats (optional)' 
              },
            },
          },
        },
        {
          name: 'explain_query',
          description: `Generate an AI-powered explanation of a GraphQL query using RAG (Retrieval-Augmented Generation).
          
Example usage:
{
  "query": "{ getTransaction(id: \"123\") { asset } }",
  "detailed": true,
  "includeLiveStats": false
}

Returns a natural language explanation of what the query does, how it works, and its purpose. Uses RAG to retrieve relevant documentation context.`,
          inputSchema: {
            type: 'object',
            properties: {
              query: { 
                type: 'string', 
                description: 'GraphQL query to explain (required)' 
              },
              detailed: { 
                type: 'boolean', 
                description: 'Generate detailed explanation (default: false)' 
              },
              includeLiveStats: { 
                type: 'boolean', 
                description: 'Include live ResLens statistics (default: false)' 
              },
            },
            required: ['query'],
          },
        },
        {
          name: 'optimize_query',
          description: `Generate AI-powered optimization suggestions for a GraphQL query.
          
Example usage:
{
  "query": "{ getTransaction(id: \"123\") { asset } }",
  "includeSimilarQueries": true
}

Returns actionable optimization suggestions, performance comparisons with similar queries, and specific recommendations for improvement.`,
          inputSchema: {
            type: 'object',
            properties: {
              query: { 
                type: 'string', 
                description: 'GraphQL query to optimize (required)' 
              },
              includeSimilarQueries: { 
                type: 'boolean', 
                description: 'Include similar queries for comparison (default: true)' 
              },
            },
            required: ['query'],
          },
        },
      ],
    }));

    // Handle tool calls
    this.server.setRequestHandler(CallToolRequestSchema, async (request) => {
      const { name, arguments: args } = request.params;

      // Tools that don't require arguments
      const noArgTools = ['check_connection', 'introspect_schema'];
      
      if (!args && !noArgTools.includes(name)) {
        return {
          content: [
            {
              type: 'text',
              text: `Error: Missing arguments for tool '${name}'.

This tool requires arguments but none were provided.
Please check the tool's input schema and provide the required parameters.

Tool: ${name}
Suggestion: Review the tool description for required parameters.`,
            },
          ],
          isError: true,
        };
      }

      try {
        // Type assertion: args is guaranteed to exist for tools that require it
        const toolArgs = args as Record<string, unknown> | undefined;
        switch (name) {
          case 'check_connection': {
            const health = await this.resilientDBClient.checkConnection();
            return {
              content: [
                {
                  type: 'text',
                  text: JSON.stringify(health, null, 2),
                },
              ],
            };
          }

          case 'execute_graphql_query': {
            if (!toolArgs) {
              return {
                content: [{ type: 'text', text: 'Error: Missing arguments' }],
              };
            }
            const queryString = toolArgs.query as string;
            if (!queryString || typeof queryString !== 'string') {
              return {
                content: [
                  {
                    type: 'text',
                    text: `Error: Missing or invalid 'query' parameter.

This tool requires a valid GraphQL query string.
Please provide a properly formatted GraphQL query.

Tool: execute_graphql_query
Required parameter: query (string)
Example: { "query": "{ getTransaction(id: \"123\") { asset } }" }
Suggestion: Check your query syntax and ensure it's a valid GraphQL query.`,
                  },
                ],
                isError: true,
              };
            }
            const query: GraphQLQuery = {
              query: queryString,
              variables: toolArgs?.variables as Record<string, unknown> | undefined,
              operationName: toolArgs?.operationName as string | undefined,
            };
            const result = await this.resilientDBClient.executeQuery(query);
            return {
              content: [
                {
                  type: 'text',
                  text: JSON.stringify(result, null, 2),
                },
              ],
            };
          }

          case 'introspect_schema': {
            const schema = await this.resilientDBClient.introspectSchema();
            return {
              content: [
                {
                  type: 'text',
                  text: JSON.stringify(schema, null, 2),
                },
              ],
            };
          }

          case 'get_query_metrics': {
            if (!toolArgs) {
              return {
                content: [{ type: 'text', text: 'Error: Missing arguments' }],
              };
            }
            const queryId = toolArgs.queryId as string;
            if (!queryId) {
              return {
                content: [
                  {
                    type: 'text',
                    text: `Error: Missing required parameter 'queryId'.

This tool requires a query ID to retrieve metrics.
Please provide a valid query ID that was returned from a previous query execution.

Tool: get_query_metrics
Required parameter: queryId (string)
Suggestion: Use a query ID from a previous execute_graphql_query call.`,
                  },
                ],
                isError: true,
              };
            }
            const metrics = await this.resLensClient.getMetrics(queryId);
            if (!metrics) {
              return {
                content: [
                  {
                    type: 'text',
                    text: `No metrics found for query ID: ${queryId}

This query ID does not exist in ResLens metrics storage.
Possible reasons:
- The query was never executed
- The query ID is incorrect
- ResLens is not capturing metrics for this query
- ResLens API is not configured or unavailable

Query ID: ${queryId}
Suggestion: Verify the query ID or check if ResLens is properly configured.`,
                  },
                ],
                isError: true,
              };
            }
            return {
              content: [
                {
                  type: 'text',
                  text: JSON.stringify(metrics, null, 2),
                },
              ],
            };
          }

          case 'validate_query': {
            if (!toolArgs) {
              return {
                content: [{ type: 'text', text: 'Error: Missing arguments' }],
              };
            }
            const query = toolArgs.query as string;
            const validation = await this.resilientDBClient.validateQuery(query);
            return {
              content: [
                {
                  type: 'text',
                  text: JSON.stringify(validation, null, 2),
                },
              ],
            };
          }

          case 'estimate_query_efficiency': {
            if (!toolArgs) {
              return {
                content: [{ type: 'text', text: 'Error: Missing arguments' }],
              };
            }
            const query = toolArgs.query as string;
            const includeSimilarQueries = toolArgs.includeSimilarQueries as boolean | undefined ?? true;
            const useLiveStats = toolArgs.useLiveStats as boolean | undefined ?? false;
            
            const estimate = await this.efficiencyEstimator.estimateEfficiency(query, {
              includeSimilarQueries,
              useLiveStats,
            });
            
            return {
              content: [
                {
                  type: 'text',
                  text: JSON.stringify(estimate, null, 2),
                },
              ],
            };
          }

          case 'get_live_stats': {
            const queryPattern = toolArgs?.queryPattern as string | undefined;
            const stats = this.liveStatsService.getLiveStats();
            
            // If query pattern provided, filter stats
            if (queryPattern) {
              const filteredQueries = await this.liveStatsService.getRecentQueries(queryPattern);
              return {
                content: [
                  {
                    type: 'text',
                    text: JSON.stringify({
                      ...stats,
                      recentQueries: filteredQueries,
                    }, null, 2),
                  },
                ],
              };
            }
            
            return {
              content: [
                {
                  type: 'text',
                  text: JSON.stringify(stats, null, 2),
                },
              ],
            };
          }

          case 'explain_query': {
            if (!toolArgs) {
              return {
                content: [{ type: 'text', text: 'Error: Missing arguments' }],
              };
            }
            const query = toolArgs.query as string;
            const detailed = toolArgs.detailed as boolean | undefined ?? false;
            const includeLiveStats = toolArgs.includeLiveStats as boolean | undefined ?? false;
            
            const explanation = await this.explanationService.explain(
              query,
              {
                detailed,
                includeLiveStats,
              }
            );
            
            return {
              content: [
                {
                  type: 'text',
                  text: JSON.stringify(explanation, null, 2),
                },
              ],
            };
          }

          case 'optimize_query': {
            if (!toolArgs) {
              return {
                content: [{ type: 'text', text: 'Error: Missing arguments' }],
              };
            }
            const query = toolArgs.query as string;
            const includeSimilarQueries = toolArgs.includeSimilarQueries as boolean | undefined ?? true;
            
            const optimization = await this.optimizationService.optimize(
              query,
              {
                includeSimilarQueries,
              }
            );
            
            return {
              content: [
                {
                  type: 'text',
                  text: JSON.stringify(optimization, null, 2),
                },
              ],
            };
          }

          default:
            return {
              content: [
                {
                  type: 'text',
                  text: `Error: Unknown tool '${name}'.

This tool is not recognized or not available.
Please check the list of available tools using the list_tools command.

Tool requested: ${name}
Suggestion: Verify the tool name spelling or check available tools.`,
                },
              ],
              isError: true,
            };
        }
      } catch (error) {
        const errorMessage = error instanceof Error ? error.message : String(error);
        const errorStack = error instanceof Error ? error.stack : undefined;
        
        // Build detailed error message
        let detailedError = `Error executing tool '${name}': ${errorMessage}`;
        
        // Add context about the arguments
        if (args) {
          detailedError += `\n\nContext:\nTool: ${name}`;
          detailedError += `\nArguments provided: ${JSON.stringify(args, null, 2)}`;
        }
        
        // Add suggestions based on error type
        if (errorMessage.includes('connection') || errorMessage.includes('ECONNREFUSED')) {
          detailedError += `\n\nSuggestion: Check if ResilientDB is running and accessible. Use 'check_connection' tool to verify connectivity.`;
        } else if (errorMessage.includes('syntax') || errorMessage.includes('parse')) {
          detailedError += `\n\nSuggestion: Validate your query syntax using the 'validate_query' tool before execution.`;
        } else if (errorMessage.includes('authentication') || errorMessage.includes('401') || errorMessage.includes('403')) {
          detailedError += `\n\nSuggestion: Check your API key and authentication credentials.`;
        } else {
          detailedError += `\n\nSuggestion: Review the tool's requirements and ensure all parameters are correctly formatted.`;
        }
        
        // Add stack trace in development mode (optional)
        if (errorStack && process.env.NODE_ENV === 'development') {
          detailedError += `\n\nStack trace:\n${errorStack}`;
        }
        
        return {
          content: [
            {
              type: 'text',
              text: detailedError,
            },
          ],
          isError: true,
        };
      }
    });

    // List available resources
    this.server.setRequestHandler(ListResourcesRequestSchema, async () => ({
      resources: [
        {
          uri: 'resilientdb://schema',
          name: 'ResilientDB GraphQL Schema',
          description: 'Current GraphQL schema from ResilientDB, including all types, queries, and mutations',
          mimeType: 'application/json',
        },
        {
          uri: 'reslens://metrics',
          name: 'ResLens Metrics',
          description: 'Recent query execution metrics from ResLens (last 10 queries)',
          mimeType: 'application/json',
        },
        {
          uri: 'reslens://live-stats',
          name: 'ResLens Live Statistics',
          description: 'Real-time statistics from ResLens Live Mode (throughput, cache hit ratio, system metrics)',
          mimeType: 'application/json',
        },
        {
          uri: 'resilientdb://connection-status',
          name: 'ResilientDB Connection Status',
          description: 'Current connection health status, endpoint URL, and response time',
          mimeType: 'application/json',
        },
      ],
    }));

    // Handle resource reads
    this.server.setRequestHandler(ReadResourceRequestSchema, async (request) => {
      const { uri } = request.params;

      try {
        if (uri === 'resilientdb://schema') {
          const schema = await this.resilientDBClient.introspectSchema();
          return {
            contents: [
              {
                uri,
                mimeType: 'application/json',
                text: JSON.stringify(schema, null, 2),
              },
            ],
          };
        }

        if (uri === 'reslens://metrics') {
          const recentQueries = await this.resLensClient.getRecentQueries(10);
          return {
            contents: [
              {
                uri,
                mimeType: 'application/json',
                text: JSON.stringify(recentQueries, null, 2),
              },
            ],
          };
        }

        if (uri === 'reslens://live-stats') {
          const stats = this.liveStatsService.getLiveStats();
          return {
            contents: [
              {
                uri,
                mimeType: 'application/json',
                text: JSON.stringify(stats, null, 2),
              },
            ],
          };
        }

        if (uri === 'resilientdb://connection-status') {
          const health = await this.resilientDBClient.checkConnection();
          return {
            contents: [
              {
                uri,
                mimeType: 'application/json',
                text: JSON.stringify(health, null, 2),
              },
            ],
          };
        }

        return {
          contents: [],
          isError: true,
        };
      } catch (error) {
        return {
          contents: [
            {
              uri,
              mimeType: 'text/plain',
              text: `Error reading resource '${uri}': ${error instanceof Error ? error.message : String(error)}`,
            },
          ],
          isError: true,
        };
      }
    });
  }

  /**
   * Start the MCP server
   */
  async start(): Promise<void> {
    // Start Live Stats Service polling if enabled
    await this.liveStatsService.startPolling();
    
    const transport = new StdioServerTransport();
    await this.server.connect(transport);
    console.log('MCP Server started and listening on stdio');
    console.log('Live Stats Service: ' + (this.liveStatsService.isAvailable() ? 'Enabled' : 'Disabled'));
  }

  /**
   * Stop the MCP server
   */
  async stop(): Promise<void> {
    // Stop Live Stats Service polling
    this.liveStatsService.stopPolling();
  }
}

