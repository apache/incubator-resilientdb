import { GraphQLClient } from 'graphql-request';
import env from '../config/environment';
import { GraphQLQuery, SchemaContext } from '../types';

/**
 * ResilientDB GraphQL API Client
 * Handles connections to ResilientDB's GraphQL endpoint
 */
export class ResilientDBClient {
  private client: GraphQLClient;

  constructor() {
    this.client = new GraphQLClient(env.RESILIENTDB_GRAPHQL_URL, {
      headers: env.RESILIENTDB_API_KEY
        ? {
            Authorization: `Bearer ${env.RESILIENTDB_API_KEY}`,
          }
        : {},
    });
  }

  /**
   * Execute a GraphQL query against ResilientDB
   */
  async executeQuery<T = unknown>(query: GraphQLQuery): Promise<T> {
    try {
      const headers: Record<string, string> = {};
      if (query.operationName) {
        headers['X-Operation-Name'] = query.operationName;
      }
      
      const response = await this.client.request<T>(
        query.query,
        query.variables || {},
        headers
      );
      return response;
    } catch (error) {
      throw new Error(
        `Failed to execute GraphQL query: ${error instanceof Error ? error.message : String(error)}`
      );
    }
  }

  /**
   * Introspect the GraphQL schema from ResilientDB
   */
  async introspectSchema(): Promise<SchemaContext> {
    const introspectionQuery = `
      query IntrospectionQuery {
        __schema {
          queryType {
            name
            fields {
              name
              description
              type {
                name
                kind
              }
              args {
                name
                type {
                  name
                  kind
                }
                description
                defaultValue
              }
            }
          }
          mutationType {
            name
            fields {
              name
              description
              type {
                name
                kind
              }
              args {
                name
                type {
                  name
                  kind
                }
                description
                defaultValue
              }
            }
          }
          types {
            name
            kind
            description
            fields {
              name
              type {
                name
                kind
              }
              description
            }
          }
        }
      }
    `;

    try {
      const response = await this.client.request<{
        __schema: {
          queryType?: {
            fields?: Array<{
              name: string;
              description?: string;
              type?: { name?: string; kind?: string };
              args?: Array<{
                name: string;
                type?: { name?: string; kind?: string };
                description?: string;
                defaultValue?: unknown;
              }>;
            }>;
          };
          mutationType?: {
            fields?: Array<{
              name: string;
              description?: string;
              type?: { name?: string; kind?: string };
              args?: Array<{
                name: string;
                type?: { name?: string; kind?: string };
                description?: string;
                defaultValue?: unknown;
              }>;
            }>;
          };
          types?: Array<{
            name?: string;
            kind?: string;
            description?: string;
            fields?: Array<{
              name?: string;
              type?: { name?: string; kind?: string };
              description?: string;
            }>;
          }>;
        };
      }>(introspectionQuery);

      // Transform introspection response to SchemaContext
      const schema = response.__schema;
      return {
        schema: JSON.stringify(schema),
        types: (schema.types || []).map(t => ({
          name: t.name || '',
          kind: t.kind || '',
          description: t.description,
          fields: (t.fields || []).map(f => ({
            name: f.name || '',
            type: f.type?.name || f.type?.kind || '',
            description: f.description,
          })),
        })),
        queries: (schema.queryType?.fields || []).map(f => ({
          name: f.name,
          type: f.type?.name || f.type?.kind || '',
          description: f.description,
          args: f.args?.map(a => ({
            name: a.name,
            type: a.type?.name || a.type?.kind || '',
            description: a.description,
            defaultValue: a.defaultValue,
          })),
        })),
        mutations: (schema.mutationType?.fields || []).map(f => ({
          name: f.name,
          type: f.type?.name || f.type?.kind || '',
          description: f.description,
          args: f.args?.map(a => ({
            name: a.name,
            type: a.type?.name || a.type?.kind || '',
            description: a.description,
            defaultValue: a.defaultValue,
          })),
        })),
      };
    } catch (error) {
      throw new Error(
        `Failed to introspect schema: ${error instanceof Error ? error.message : String(error)}`
      );
    }
  }

  /**
   * Validate a GraphQL query syntax
   */
  async validateQuery(query: string): Promise<{ valid: boolean; errors?: string[] }> {
    // For now, basic validation - can be enhanced with graphql-js parser
    try {
      // This is a placeholder - in production, use graphql-js validate function
      await this.client.request(query, {});
      return { valid: true };
    } catch (error) {
      return {
        valid: false,
        errors: [error instanceof Error ? error.message : String(error)],
      };
    }
  }

  /**
   * Get the GraphQL endpoint URL
   */
  getEndpoint(): string {
    return env.RESILIENTDB_GRAPHQL_URL;
  }

  /**
   * Check connection health to ResilientDB
   * Returns connection status and basic info
   */
  async checkConnection(): Promise<{
    connected: boolean;
    endpoint: string;
    error?: string;
    responseTime?: number;
  }> {
    const startTime = Date.now();
    try {
      // Simple introspection query to test connection
      const testQuery = `{ __typename }`;
      await this.client.request(testQuery);
      const responseTime = Date.now() - startTime;
      
      return {
        connected: true,
        endpoint: env.RESILIENTDB_GRAPHQL_URL,
        responseTime,
      };
    } catch (error) {
      return {
        connected: false,
        endpoint: env.RESILIENTDB_GRAPHQL_URL,
        error: error instanceof Error ? error.message : String(error),
      };
    }
  }
}

