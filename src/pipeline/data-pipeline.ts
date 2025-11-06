import { ResilientDBClient } from '../resilientdb/client';
import { ResLensClient } from '../reslens/client';
import { LiveStatsService } from '../services/live-stats-service';
import { PipelineData, GraphQLQuery, QueryExecutionMetrics, SchemaContext } from '../types';

/**
 * Data Pipeline
 * Connects ResilientDB, ResLens, and AI module data flows
 */
export class DataPipeline {
  private resilientDBClient: ResilientDBClient;
  private resLensClient: ResLensClient;
  private liveStatsService: LiveStatsService;
  private schemaCache?: SchemaContext;
  private schemaCacheExpiry?: Date;

  constructor() {
    this.resilientDBClient = new ResilientDBClient();
    this.resLensClient = new ResLensClient();
    this.liveStatsService = new LiveStatsService();
  }

  /**
   * Process a GraphQL query through the pipeline
   */
  async processQuery(
    query: GraphQLQuery,
    captureMetrics: boolean = true
  ): Promise<PipelineData> {
    const startTime = Date.now();
    const queryId = this.generateQueryId();

    try {
      // Step 1: Get schema context (cached for performance)
      const schemaContext = await this.getSchemaContext();

      // Step 2: Execute query
      const result = await this.resilientDBClient.executeQuery(query);

      // Step 3: Calculate execution time
      const executionTime = Date.now() - startTime;

      // Step 4: Capture metrics if enabled
      let metrics: QueryExecutionMetrics | undefined;
      if (captureMetrics) {
        metrics = await this.resLensClient.captureMetrics(
          queryId,
          query.query,
          executionTime,
          true,
          {
            resultSize: this.estimateResultSize(result),
          }
        );
      }

      // Step 5: Return pipeline data
      return {
        query,
        metrics,
        schemaContext,
        timestamp: new Date(),
      };
    } catch (error) {
      const executionTime = Date.now() - startTime;

      // Capture error metrics
      if (captureMetrics) {
        await this.resLensClient.captureMetrics(
          queryId,
          query.query,
          executionTime,
          false,
          {
            error: error instanceof Error ? error.message : String(error),
          }
        );
      }

      throw error;
    }
  }

  /**
   * Get schema context with caching
   */
  async getSchemaContext(forceRefresh: boolean = false): Promise<SchemaContext> {
    // Cache schema for 5 minutes
    const cacheTTL = 5 * 60 * 1000; // 5 minutes in ms

    if (
      !forceRefresh &&
      this.schemaCache &&
      this.schemaCacheExpiry &&
      this.schemaCacheExpiry > new Date()
    ) {
      return this.schemaCache;
    }

    this.schemaCache = await this.resilientDBClient.introspectSchema();
    this.schemaCacheExpiry = new Date(Date.now() + cacheTTL);

    return this.schemaCache;
  }

  /**
   * Get query metrics for analysis
   */
  async getQueryMetrics(queryId: string): Promise<QueryExecutionMetrics | null> {
    return await this.resLensClient.getMetrics(queryId);
  }

  /**
   * Get query statistics for pattern analysis
   */
  async getQueryStatistics(queryPattern: string): Promise<QueryExecutionMetrics[]> {
    return await this.resLensClient.getQueryStats(queryPattern);
  }

  /**
   * Generate a unique query ID
   */
  private generateQueryId(): string {
    return `query_${Date.now()}_${Math.random().toString(36).substring(2, 9)}`;
  }

  /**
   * Estimate result size (simple implementation)
   */
  private estimateResultSize(result: unknown): number {
    return JSON.stringify(result).length;
  }

  /**
   * Health check for all pipeline components
   */
  async healthCheck(): Promise<{
    resilientdb: boolean;
    reslens: boolean;
    liveStats: boolean;
  }> {
    const [reslensHealth, liveStatsHealth] = await Promise.allSettled([
      this.resLensClient.healthCheck(),
      this.liveStatsService.checkAvailability(),
    ]);

    // ResilientDB health check via schema introspection
    let resilientdbHealth = false;
    try {
      await this.resilientDBClient.introspectSchema();
      resilientdbHealth = true;
    } catch {
      resilientdbHealth = false;
    }

    return {
      resilientdb: resilientdbHealth,
      reslens:
        reslensHealth.status === 'fulfilled' ? reslensHealth.value : false,
      liveStats:
        liveStatsHealth.status === 'fulfilled' ? liveStatsHealth.value : false,
    };
  }

  /**
   * Get live statistics from ResLens
   */
  getLiveStats() {
    return this.liveStatsService.getLiveStats();
  }

  /**
   * Start Live Stats polling
   */
  async startLiveStats(): Promise<void> {
    await this.liveStatsService.startPolling();
  }

  /**
   * Stop Live Stats polling
   */
  stopLiveStats(): void {
    this.liveStatsService.stopPolling();
  }
}

