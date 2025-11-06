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
          name: 'execute_graphql_query',
          description: 'Execute a GraphQL query against ResilientDB',
          inputSchema: {
            type: 'object',
            properties: {
              query: { type: 'string', description: 'GraphQL query string' },
              variables: {
                type: 'object',
                description: 'Query variables',
              },
              operationName: {
                type: 'string',
                description: 'Operation name',
              },
            },
            required: ['query'],
          },
        },
        {
          name: 'introspect_schema',
          description: 'Get the GraphQL schema from ResilientDB',
          inputSchema: {
            type: 'object',
            properties: {},
          },
        },
        {
          name: 'get_query_metrics',
          description: 'Get execution metrics for a query from ResLens',
          inputSchema: {
            type: 'object',
            properties: {
              queryId: { type: 'string', description: 'Query ID' },
            },
            required: ['queryId'],
          },
        },
        {
          name: 'validate_query',
          description: 'Validate a GraphQL query syntax',
          inputSchema: {
            type: 'object',
            properties: {
              query: { type: 'string', description: 'GraphQL query to validate' },
            },
            required: ['query'],
          },
        },
        {
          name: 'estimate_query_efficiency',
          description: 'Estimate query performance and efficiency using analysis and ResLens metrics',
          inputSchema: {
            type: 'object',
            properties: {
              query: { type: 'string', description: 'GraphQL query to estimate' },
              includeSimilarQueries: { type: 'boolean', description: 'Include similar queries in estimation' },
              useLiveStats: { type: 'boolean', description: 'Use Live Mode stats if available' },
            },
            required: ['query'],
          },
        },
        {
          name: 'get_live_stats',
          description: 'Get real-time statistics from ResLens (Live Mode)',
          inputSchema: {
            type: 'object',
            properties: {
              queryPattern: { type: 'string', description: 'Optional query pattern to filter stats' },
            },
          },
        },
        {
          name: 'explain_query',
          description: 'Generate an AI-powered explanation of a GraphQL query using RAG',
          inputSchema: {
            type: 'object',
            properties: {
              query: { type: 'string', description: 'GraphQL query to explain' },
              detailed: { type: 'boolean', description: 'Generate detailed explanation' },
              includeLiveStats: { type: 'boolean', description: 'Include live ResLens statistics' },
            },
            required: ['query'],
          },
        },
        {
          name: 'optimize_query',
          description: 'Generate AI-powered optimization suggestions for a GraphQL query',
          inputSchema: {
            type: 'object',
            properties: {
              query: { type: 'string', description: 'GraphQL query to optimize' },
              includeSimilarQueries: { type: 'boolean', description: 'Include similar queries for comparison' },
            },
            required: ['query'],
          },
        },
      ],
    }));

    // Handle tool calls
    this.server.setRequestHandler(CallToolRequestSchema, async (request) => {
      const { name, arguments: args } = request.params;

      if (!args) {
        return {
          content: [
            {
              type: 'text',
              text: `Missing arguments for tool: ${name}`,
            },
          ],
          isError: true,
        };
      }

      try {
        switch (name) {
          case 'execute_graphql_query': {
            const query: GraphQLQuery = {
              query: args.query as string,
              variables: args.variables as Record<string, unknown> | undefined,
              operationName: args.operationName as string | undefined,
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
            const queryId = args.queryId as string;
            const metrics = await this.resLensClient.getMetrics(queryId);
            if (!metrics) {
              return {
                content: [
                  {
                    type: 'text',
                    text: `No metrics found for query ID: ${queryId}`,
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
            const query = args.query as string;
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
            const query = args.query as string;
            const includeSimilarQueries = args.includeSimilarQueries as boolean | undefined ?? true;
            const useLiveStats = args.useLiveStats as boolean | undefined ?? false;
            
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
            const queryPattern = args.queryPattern as string | undefined;
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
            const query = args.query as string;
            const detailed = args.detailed as boolean | undefined ?? false;
            const includeLiveStats = args.includeLiveStats as boolean | undefined ?? false;
            
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
            const query = args.query as string;
            const includeSimilarQueries = args.includeSimilarQueries as boolean | undefined ?? true;
            
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
                  text: `Unknown tool: ${name}`,
                },
              ],
              isError: true,
            };
        }
      } catch (error) {
        return {
          content: [
            {
              type: 'text',
              text: `Error executing tool ${name}: ${error instanceof Error ? error.message : String(error)}`,
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
          description: 'Current GraphQL schema from ResilientDB',
          mimeType: 'application/json',
        },
        {
          uri: 'reslens://metrics',
          name: 'ResLens Metrics',
          description: 'Query execution metrics from ResLens',
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

        return {
          contents: [],
          isError: true,
        };
      } catch (error) {
        return {
          contents: [],
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

