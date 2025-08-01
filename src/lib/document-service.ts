import chalk from "chalk";
import { Document, LlamaParseReader, VectorStoreIndex } from "llamaindex";
import { join } from "path";
import { config } from "../config/environment";
import { TITLE_MAPPINGS } from "./constants";
import { parsedDocumentStorage } from "./parsed-document-storage";
import { vectorStoreService } from "./vector-store";

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

export interface QueryOptions {
  topK?: number;
  documentPaths?: string[];
}

export class DocumentServiceError extends Error {
  constructor(
    message: string,
    public documentPaths: string[],
    public originalError?: Error
  ) {
    super(message);
    this.name = 'DocumentServiceError';
  }
}

/**
 * Simplified document service following LlamaIndex TypeScript best practices
 * Uses PGVectorStore as single source of truth, eliminating complex caching layers
 */
export class DocumentService {
  private static instance: DocumentService;
  private defaultTopK: number = 10;

  private constructor() {}

  static getInstance(): DocumentService {
    if (!DocumentService.instance) {
      DocumentService.instance = new DocumentService();
    }
    return DocumentService.instance;
  }

  /**
   * Parse and index documents directly to PGVectorStore
   * Following best practice: single index with metadata for multi-document support
   */
  async indexDocuments(documentPaths: string[]): Promise<VectorStoreIndex> {
    try {
      console.log(chalk.blue(`[DocumentService] Indexing ${documentPaths.length} documents`));
      
      // Parse all documents with source metadata
      const allDocuments = await this.parseDocumentsWithMetadata(documentPaths);
      
      if (!allDocuments || allDocuments.length === 0) {
        throw new Error("No documents could be parsed");
      }

      // Create single index with all documents using PGVectorStore
      const storageContext = await vectorStoreService.getStorageContext();
      const index = await VectorStoreIndex.fromDocuments(allDocuments, { 
        storageContext
      });

      console.log(chalk.green(`[DocumentService] Successfully indexed ${allDocuments.length} document chunks`));
      return index;

    } catch (error) {
      const errorMessage = error instanceof Error ? error.message : String(error);
      console.error(chalk.red(`[DocumentService] Indexing failed: ${errorMessage}`));
      throw new DocumentServiceError(
        `Document indexing failed: ${errorMessage}`,
        documentPaths,
        error instanceof Error ? error : undefined
      );
    }
  }

  /**
   * Query documents using simplified approach with optional document filtering
   * Uses PGVectorStore's native metadata filtering for efficiency
   */
  async queryDocuments(query: string, options: QueryOptions = {}): Promise<QueryResult> {
    try {
      const { topK = this.defaultTopK, documentPaths } = options;

      console.log(chalk.blue(`[DocumentService] Querying with topK=${topK}${documentPaths ? `, filtered to ${documentPaths.length} documents` : ''}`));

      // Load existing index from vector store
      const vectorStore = await vectorStoreService.getVectorStore();
      const index = await VectorStoreIndex.fromVectorStore(vectorStore);

      // Create retriever with optional document filtering
      const retrieverOptions: any = { similarityTopK: topK };
      
      if (documentPaths && documentPaths.length > 0) {
        // Create metadata filters for document filtering
        retrieverOptions.filters = {
          filters: [{
            key: "source_document",
            value: documentPaths,
            operator: "in"
          }]
        };
      }

      const retriever = index.asRetriever(retrieverOptions);
      const nodes = await retriever.retrieve({ query });

      console.log(chalk.green(`[DocumentService] Retrieved ${nodes.length} relevant chunks`));

      return this.formatQueryResult(nodes, documentPaths || []);

    } catch (error) {
      const errorMessage = error instanceof Error ? error.message : String(error);
      console.error(chalk.red(`[DocumentService] Query failed: ${errorMessage}`));
      throw new DocumentServiceError(
        `Document query failed: ${errorMessage}`,
        options.documentPaths || [],
        error instanceof Error ? error : undefined
      );
    }
  }

  /**
   * Check if documents are already indexed by querying vector store
   */
  async areDocumentsIndexed(documentPaths: string[]): Promise<boolean> {
    try {
      const vectorStore = await vectorStoreService.getVectorStore();
      const index = await VectorStoreIndex.fromVectorStore(vectorStore);
      
      // Try a simple query to see if we get results
      const retriever = index.asRetriever({
        similarityTopK: 1,
        filters: {
          filters: [{
            key: "source_document",
            value: documentPaths,
            operator: "in"
          }]
        }
      });

      const nodes = await retriever.retrieve({ query: "test" });
      return nodes.length > 0;

    } catch (error) {
      console.warn(chalk.yellow(`[DocumentService] Could not check if documents are indexed: ${error}`));
      return false;
    }
  }

  /**
   * Get available documents from vector store metadata
   */
  async getAvailableDocuments(): Promise<DocumentSource[]> {
    try {
      // This would require a custom query to get unique source_document values
      // For now, return empty array - this could be enhanced with a direct DB query
      console.warn(chalk.yellow(`[DocumentService] getAvailableDocuments not yet implemented`));
      return [];
    } catch (error) {
      console.error(chalk.red(`[DocumentService] Failed to get available documents: ${error}`));
      return [];
    }
  }

  /**
   * Get cache statistics for monitoring
   */
  async getCacheStats(): Promise<{
    documentCount: number;
    totalChunks: number;
    oldestDocument: string | null;
    newestDocument: string | null;
  }> {
    try {
      return await parsedDocumentStorage.getStorageStats();
    } catch (error) {
      console.error(chalk.red(`[DocumentService] Failed to get cache stats: ${error}`));
      return {
        documentCount: 0,
        totalChunks: 0,
        oldestDocument: null,
        newestDocument: null
      };
    }
  }

  /**
   * Clear document cache (useful for testing or cleanup)
   */
  async clearCache(): Promise<void> {
    try {
      await parsedDocumentStorage.clearAll();
      console.log(chalk.green("[DocumentService] Document cache cleared"));
    } catch (error) {
      console.error(chalk.red(`[DocumentService] Failed to clear cache: ${error}`));
      throw error;
    }
  }

  /**
   * Remove specific document from cache
   */
  async removeCachedDocument(documentPath: string): Promise<void> {
    try {
      await parsedDocumentStorage.removeDocument(documentPath);
      console.log(chalk.green(`[DocumentService] Removed cached document: ${documentPath}`));
    } catch (error) {
      console.error(chalk.red(`[DocumentService] Failed to remove cached document ${documentPath}: ${error}`));
      throw error;
    }
  }

  /**
   * Parse documents and add source metadata for multi-document support
   * Uses cached parsed chunks when available to avoid re-parsing
   */
  private async parseDocumentsWithMetadata(documentPaths: string[]): Promise<Document[]> {
    const allDocuments: Document[] = [];
    let cacheHits = 0;
    let cacheMisses = 0;

    for (const documentPath of documentPaths) {
      try {
        // Check if we already have this document stored
        const hasStored = await parsedDocumentStorage.hasDocument(documentPath);
        
        if (hasStored) {
          console.log(chalk.green(`[DocumentService] Using cached parsed content for ${documentPath}`));
          const storedDocuments = await parsedDocumentStorage.getDocument(documentPath);
          
          if (storedDocuments && storedDocuments.length > 0) {
            // Add source metadata to cached documents
            const documentsWithMetadata = storedDocuments.map(doc => new Document({
              id_: doc.id_,
              text: doc.text,
              metadata: {
                ...doc.metadata,
                source_document: documentPath,
                document_name: this.getDocumentDisplayName(documentPath),
                indexed_at: new Date().toISOString(),
                cached: true // Mark as cached for debugging
              }
            }));

            allDocuments.push(...documentsWithMetadata);
            cacheHits++;
            console.log(chalk.gray(`[DocumentService] Retrieved ${documentsWithMetadata.length} cached chunks from ${documentPath}`));
            continue;
          }
        }

        // Document not in cache, parse it
        console.log(chalk.blue(`[DocumentService] Parsing document ${documentPath} (not in cache)`));
        const documents = await this.parseDocument(documentPath);
        
        // Store the parsed results in database for future use
        try {
          await parsedDocumentStorage.storeDocument(documentPath, documents);
          console.log(chalk.green(`[DocumentService] Cached parsed content for future use: ${documentPath}`));
        } catch (storageError) {
          console.warn(chalk.yellow(`[DocumentService] Failed to cache parsed content for ${documentPath}:`), storageError);
          // Continue processing even if caching fails
        }
        
        // Add source metadata to each document chunk
        const documentsWithMetadata = documents.map(doc => new Document({
          id_: doc.id_,
          text: doc.text,
          metadata: {
            ...doc.metadata,
            source_document: documentPath,
            document_name: this.getDocumentDisplayName(documentPath),
            indexed_at: new Date().toISOString(),
            cached: false // Mark as freshly parsed
          }
        }));

        allDocuments.push(...documentsWithMetadata);
        cacheMisses++;
        console.log(chalk.gray(`[DocumentService] Parsed ${documentsWithMetadata.length} chunks from ${documentPath}`));

      } catch (error) {
        console.error(chalk.red(`[DocumentService] Failed to parse ${documentPath}: ${error}`));
        throw error;
      }
    }

    // Log cache performance
    const totalDocuments = documentPaths.length;
    const cacheHitRate = totalDocuments > 0 ? ((cacheHits / totalDocuments) * 100).toFixed(1) : '0.0';
    console.log(chalk.blue(`[DocumentService] Cache performance: ${cacheHits} hits, ${cacheMisses} misses (${cacheHitRate}% hit rate)`));

    return allDocuments;
  }

  /**
   * Parse a single document using LlamaParse
   */
  private async parseDocument(documentPath: string): Promise<Document[]> {
    const parser = new LlamaParseReader({
      apiKey: config.llamaCloudApiKey,
      resultType: "markdown",
    });

    const filePath = join(process.cwd(), documentPath);
    return await parser.loadData(filePath);
  }

  /**
   * Get display name for a document path
   */
  private getDocumentDisplayName(documentPath: string): string {
    const filename = documentPath.split("/").pop() || documentPath;
    const lowerFilename = filename.toLowerCase();
    return TITLE_MAPPINGS[lowerFilename] || filename.replace(".pdf", "");
  }

  /**
   * Format query result from retrieved nodes
   */
  private formatQueryResult(nodes: any[], requestedDocumentPaths: string[]): QueryResult {
    const chunksBySource: { [key: string]: ContextChunk[] } = {};
    const processedChunks: ContextChunk[] = [];

    for (const nodeWithScore of nodes) {
      const sourceDoc = nodeWithScore.node.metadata?.source_document || "Unknown";
      const content = nodeWithScore.node.getContent();

      const chunk: ContextChunk = {
        content,
        source: sourceDoc,
        metadata: {
          score: nodeWithScore.score,
          retrievalMethod: "pgvector-native",
          ...nodeWithScore.node.metadata
        }
      };

      if (!chunksBySource[sourceDoc]) {
        chunksBySource[sourceDoc] = [];
      }
      chunksBySource[sourceDoc].push(chunk);
      processedChunks.push(chunk);
    }

    // Create formatted context with source attribution
    const contextParts = Object.entries(chunksBySource).map(
      ([source, chunks]) => {
        const displayName = this.getDocumentDisplayName(source);
        const chunkContents = chunks.map((chunk) => chunk.content).join("\n\n");
        return `**From ${displayName}:**\n${chunkContents}`;
      }
    );

    const formattedContext = contextParts.join("\n\n---\n\n");

    // Create source information
    const sources: DocumentSource[] = requestedDocumentPaths.map(path => ({
      path,
      name: path.split("/").pop() || path,
      displayTitle: this.getDocumentDisplayName(path)
    }));

    return {
      context: formattedContext,
      sources,
      chunks: processedChunks,
      totalChunks: nodes.length
    };
  }
}

// Export singleton instance
export const documentService = DocumentService.getInstance();