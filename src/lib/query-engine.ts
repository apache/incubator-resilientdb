import chalk from "chalk";
import { MetadataMode } from "llamaindex";
import { TITLE_MAPPINGS } from "./constants";
import { documentIndexManager } from "./document-index-manager";

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

  // Get display name for a document path
  private getDocumentDisplayName(documentPath: string): string {
    const filename = documentPath.split("/").pop() || documentPath;
    const lowerFilename = filename.toLowerCase();
    return TITLE_MAPPINGS[lowerFilename] || filename.replace(".pdf", "");
  }

  // Main unified query processing method - using direct retrieval for all cases
  async queryDocuments(
    query: string,
    documentPaths: string[],
    options: StreamingQueryOptions = {}
  ): Promise<QueryResult> {
    try {
      // Validate that all documents have indices
      const hasAllIndices = documentIndexManager.hasAllIndices(documentPaths);
      if (!hasAllIndices) {
        throw new Error(
          "Some document indices are not prepared. Please prepare all documents first."
        );
      }

      console.log(chalk.yellow(`[QueryEngine] Processing query for ${documentPaths.length} documents`));
      console.log(chalk.yellow(`[QueryEngine] Documents: ${documentPaths.map(p => p.split("/").pop()).join(", ")}`));

      // Use direct retrieval for all cases - much faster and more reliable
      return await this.queryDirectRetrieval(query, documentPaths, options);

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

  // Direct retrieval method for better performance - based on the old implementation
  private async queryDirectRetrieval(
    query: string,
    documentPaths: string[],
    options: StreamingQueryOptions
  ): Promise<QueryResult> {
    console.log(chalk.blue(`[QueryEngine] Using direct retrieval for ${documentPaths.length} documents`));

    try {
      const {
        topK = this.defaultSimilarityTopK,
      } = options;

      // Get combined index for efficient retrieval
      const combinedIndex = await documentIndexManager.getCombinedIndex(documentPaths);
      if (!combinedIndex) {
        throw new Error("Failed to create combined index from documents");
      }

      // Configure retriever with topK scaled by document count
      const retriever = combinedIndex.asRetriever({
        similarityTopK: topK * documentPaths.length, // Scale with document count like the old implementation
      });

      // Retrieve relevant chunks
      const retrievedNodes = await retriever.retrieve({ query });
      console.log(chalk.green(`[QueryEngine] Retrieved ${retrievedNodes.length} chunks`));

      // Organize chunks by source document (like the old implementation)
      const chunksBySource: { [key: string]: ContextChunk[] } = {};
      let totalContextLength = 0;
      const processedChunks: ContextChunk[] = [];

      for (const node of retrievedNodes) {
        const sourceDoc = node.node.metadata?.source_document || "Unknown";
        const content = node.node.getContent(MetadataMode.NONE);

        // Check if adding this chunk would exceed context limit
        if (totalContextLength + content.length > this.maxContextLength) {
          console.log(
            chalk.yellow(`[QueryEngine] Context limit reached. Stopping at ${processedChunks.length} chunks.`)
          );
          break;
        }

        const chunk: ContextChunk = {
          content,
          source: sourceDoc,
          metadata: {
            score: node.score,
            retrievalMethod: "direct",
            ...node.node.metadata
          }
        };

        if (!chunksBySource[sourceDoc]) {
          chunksBySource[sourceDoc] = [];
        }
        chunksBySource[sourceDoc].push(chunk);
        processedChunks.push(chunk);
        totalContextLength += content.length;
      }

      // Create formatted context with source attribution (like the old implementation)
      const contextParts = Object.entries(chunksBySource).map(
        ([source, chunks]) => {
          const displayName = this.getDocumentDisplayName(source);
          const chunkContents = chunks.map((chunk) => chunk.content).join("\n\n");
          return `**From ${displayName}:**\n${chunkContents}`;
        }
      );

      const formattedContext = contextParts.join("\n\n---\n\n");

      // Create source information
      const sources: DocumentSource[] = documentPaths.map(path => ({
        path,
        name: path.split("/").pop() || path,
        displayTitle: this.getDocumentDisplayName(path)
      }));

      console.log(chalk.green(`[QueryEngine] Successfully processed ${processedChunks.length} chunks from ${Object.keys(chunksBySource).length} sources`));

      return {
        context: formattedContext,
        sources,
        chunks: processedChunks,
        totalChunks: retrievedNodes.length
      };

    } catch (error) {
      const errorMessage = error instanceof Error ? error.message : String(error);
      console.error(chalk.red(`[QueryEngine] Direct retrieval failed: ${errorMessage}`));
      throw new WorkflowAgentError(
        `Direct retrieval failed: ${errorMessage}`,
        documentPaths,
        error instanceof Error ? error : undefined
      );
    }
  }

  // Backward compatibility methods
  async queryMultipleDocuments(
    query: string,
    documentPaths: string[],
    options: StreamingQueryOptions = {}
  ): Promise<QueryResult> {
    return this.queryDocuments(query, documentPaths, options);
  }

  async querySingleDocument(
    query: string,
    documentPath: string,
    options: {
      topK?: number;
    } = {}
  ): Promise<QueryResult> {
    return this.queryDocuments(query, [documentPath], options);
  }

  // Get summary of available documents
  async getDocumentsSummary(documentPaths: string[]): Promise<{
    availableDocuments: number;
    totalDocuments: number;
    documentTitles: string[];
  }> {
    const hasAllIndices = documentIndexManager.hasAllIndices(documentPaths);
    const availableCount = hasAllIndices ? documentPaths.length : 0;

    return {
      availableDocuments: availableCount,
      totalDocuments: documentPaths.length,
      documentTitles: documentPaths.map(path => this.getDocumentDisplayName(path))
    };
  }

  // Get information about loaded agents (simplified for compatibility)
  getAgentInfo(): {
    totalAgents: number;
    documentPaths: string[];
  } {
    return {
      totalAgents: 0, // No agents in direct retrieval mode
      documentPaths: []
    };
  }

  // Clear all agents (no-op for compatibility)
  clearAllAgents(): void {
    console.log(chalk.yellow(`[QueryEngine] No agents to clear in direct retrieval mode`));
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

  // Extract unique sources from query result (from old implementation)
  getUniqueSources(chunks: ContextChunk[]): DocumentSource[] {
    const uniqueSources = new Set<string>();
    const sources: DocumentSource[] = [];

    chunks.forEach((chunk) => {
      if (!uniqueSources.has(chunk.source)) {
        uniqueSources.add(chunk.source);
        sources.push({
          path: chunk.source,
          name: chunk.source.split("/").pop() || chunk.source,
          displayTitle: this.getDocumentDisplayName(chunk.source),
        });
      }
    });

    return sources;
  }
}

// Export singleton instance
export const queryEngine = QueryEngine.getInstance();

