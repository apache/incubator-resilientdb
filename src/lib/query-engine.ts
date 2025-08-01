import chalk from "chalk";
import { TITLE_MAPPINGS } from "./constants";
import { documentService } from "./document-service";

// Re-export interfaces from existing implementation for compatibility
export interface DocumentSource {
  path: string;
  name: string;
  displayTitle?: string;
}

export interface ContextChunk {
  content: string;
  source: string;
  metadata?: Record<string, any>;
}

export interface QueryResult {
  context: string;
  sources: DocumentSource[];
  chunks: ContextChunk[];
  totalChunks: number;
}

export interface StreamingQueryOptions {
  enableStreaming?: boolean;
  topK?: number;
  tool?: string;
  language?: string;
  scope?: string[];
}

export class WorkflowAgentError extends Error {
  constructor(
    message: string,
    public documentPaths: string[],
    public originalError?: Error
  ) {
    super(message);
    this.name = 'WorkflowAgentError';
  }
}

export class QueryEngine {
  private static instance: QueryEngine;
  private maxContextLength: number = 220000; // DeepSeek max context length
  private defaultSimilarityTopK: number = 10;
  private static readonly SEPARATOR_LENGTH: number = 10;

  private constructor() {
    // Initialize with configuration
  }

  static getInstance(): QueryEngine {
    if (!QueryEngine.instance) {
      QueryEngine.instance = new QueryEngine();
    }
    return QueryEngine.instance;
  }

  // Set maximum context length for responses
  setMaxContextLength(length: number): void {
    this.maxContextLength = length;
  }

  private getDocumentDisplayName(documentPath: string): string {
    const filename = documentPath.split("/").pop() || documentPath;
    const lowerFilename = filename.toLowerCase();
    return TITLE_MAPPINGS[lowerFilename] || filename.replace(".pdf", "");
  }

  async queryDocuments(
    query: string,
    documentPaths: string[],
    options: StreamingQueryOptions = {}
  ): Promise<QueryResult> {
    try {
      console.log(chalk.yellow(`[QueryEngine] Processing query for ${documentPaths.length} documents`));
      console.log(chalk.yellow(`[QueryEngine] Documents: ${documentPaths.map(p => p.split("/").pop()).join(", ")}`));

      return await this.query(query, documentPaths, options);

    } catch (error) {
      if (error instanceof WorkflowAgentError) {
        throw error;
      }
      const errorMessage = error instanceof Error ? error.message : String(error);
      console.error(chalk.red(`[QueryEngine] Query failed: ${errorMessage}`));
      throw new WorkflowAgentError(
        `Query execution failed: ${errorMessage}`,
        documentPaths,
        error instanceof Error ? error : undefined
      );
    }
  }

  private async query(
    query: string,
    documentPaths: string[],
    options: StreamingQueryOptions
  ): Promise<QueryResult> {
    console.log(chalk.blue(`[QueryEngine] Performing query for ${documentPaths.length} documents`));

    try {
      const { topK = this.defaultSimilarityTopK } = options;

      // Check if documents are indexed, if not index them
      const areIndexed = await documentService.areDocumentsIndexed(documentPaths);
      if (!areIndexed) {
        console.log(chalk.yellow(`[QueryEngine] Documents not indexed, indexing now...`));
        await documentService.indexDocuments(documentPaths);
      }

      const result = await documentService.queryDocuments(query, {
        topK,
        documentPaths
      });

      console.log(chalk.green(`[QueryEngine] Query completed with ${result.totalChunks} chunks`));
      return result;

    } catch (error) {
      const errorMessage = error instanceof Error ? error.message : String(error);
      console.error(chalk.red(`[QueryEngine] Query failed: ${errorMessage}`));
      throw new WorkflowAgentError(
        `Query failed: ${errorMessage}`,
        documentPaths,
        error instanceof Error ? error : undefined
      );
    }
  }

  async prepareDocuments(documentPaths: string[]): Promise<void> {
    console.log(chalk.blue(`[QueryEngine] Preparing ${documentPaths.length} documents for indexing`));
    await documentService.indexDocuments(documentPaths);
  }

  // Get summary of available documents
  async getDocumentsSummary(documentPaths: string[]): Promise<{
    availableDocuments: number;
    totalDocuments: number;
    documentTitles: string[];
  }> {
    const areIndexed = await documentService.areDocumentsIndexed(documentPaths);
    const availableCount = areIndexed ? documentPaths.length : 0;
    
    return {
      availableDocuments: availableCount,
      totalDocuments: documentPaths.length,
      documentTitles: documentPaths.map(path => this.getDocumentDisplayName(path))
    };
  }

  // Get information about available documents
  getDocumentInfo(): {
    totalDocuments: number;
    documentPaths: string[];
  } {
    return {
      totalDocuments: 0, // Would need to query vector store for actual count
      documentPaths: []
    };
  }

  // Truncate context to fit within limits while preserving source attribution (from old implementation)
  truncateContext(context: string, maxLength: number): string {
    if (context.length <= maxLength) {
      return context;
    }

    // Try to truncate at section boundaries (---)
    const sections = context.split("\n\n---\n\n");
    let truncated = "";

    for (const section of sections) {
      if (
        truncated.length +
        section.length +
        QueryEngine.SEPARATOR_LENGTH <=
        maxLength
      ) {
        truncated += (truncated ? "\n\n---\n\n" : "") + section;
      } else {
        break;
      }
    }

    if (truncated.length === 0) {
      // If no complete sections fit, truncate the first section
      truncated =
        sections[0].substring(0, maxLength - 20) + "...\n\n[Context truncated]";
    } else {
      truncated += "\n\n[Additional context truncated]";
    }

    return truncated;
  }
}

// Export singleton instance
export const queryEngine = QueryEngine.getInstance();

