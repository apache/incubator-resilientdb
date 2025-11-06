/**
 * RAG Retrieval Service
 * 
 * Provides semantic search functionality for retrieving relevant document chunks
 * based on query embeddings. This service is the core of the RAG (Retrieval-Augmented Generation)
 * system, enabling the LLM to access relevant context from stored documentation.
 */

import { ResilientDBVectorStore, DocumentChunk } from './resilientdb-vector-store';
import { EmbeddingService } from './embedding-service';

export interface RetrievalOptions {
  /** Maximum number of chunks to retrieve */
  limit?: number;
  /** Minimum similarity threshold (0-1) */
  minSimilarity?: number;
  /** Filter by source file */
  source?: string;
  /** Filter by chunk type */
  type?: DocumentChunk['metadata']['type'];
  /** Include similarity scores in results */
  includeScores?: boolean;
}

export interface RetrievalResult {
  chunks: DocumentChunk[];
  scores?: number[];
  queryEmbedding: number[];
}

/**
 * Retrieval Service for RAG
 * 
 * Handles semantic search over document chunks stored in ResilientDB.
 * Uses embedding-based similarity search to find relevant context.
 */
export class RetrievalService {
  private vectorStore: ResilientDBVectorStore;
  private embeddingService: EmbeddingService;

  constructor() {
    this.vectorStore = new ResilientDBVectorStore();
    this.embeddingService = new EmbeddingService();
  }

  /**
   * Retrieve relevant chunks for a query using semantic search
   * 
   * @param query - Natural language query or search text
   * @param options - Retrieval options (limit, filters, etc.)
   * @returns Retrieved chunks with optional similarity scores
   */
  async retrieve(
    query: string,
    options: RetrievalOptions = {}
  ): Promise<RetrievalResult> {
    const {
      limit = 10,
      minSimilarity = 0.3,
      source,
      type,
      includeScores = false,
    } = options;

    // Generate embedding for the query
    const queryEmbedding = await this.embeddingService.generateEmbedding(query);

    // Perform semantic search
    const chunks = await this.vectorStore.searchSimilar(queryEmbedding, limit * 2, {
      source,
      type,
    });

    // Filter by minimum similarity threshold
    const filteredResults: Array<{ chunk: DocumentChunk; score: number }> = [];
    
    for (const chunk of chunks) {
      const similarity = this.computeSimilarity(queryEmbedding, chunk.embedding);
      
      if (similarity >= minSimilarity) {
        filteredResults.push({ chunk, score: similarity });
      }
    }

    // Sort by similarity and limit results
    filteredResults.sort((a, b) => b.score - a.score);
    const topResults = filteredResults.slice(0, limit);

    return {
      chunks: topResults.map(r => r.chunk),
      scores: includeScores ? topResults.map(r => r.score) : undefined,
      queryEmbedding,
    };
  }

  /**
   * Retrieve chunks by multiple queries (for complex context needs)
   * 
   * Useful when you need context from multiple perspectives.
   * 
   * @param queries - Array of query strings
   * @param options - Retrieval options
   * @returns Combined and deduplicated chunks
   */
  async retrieveMultiple(
    queries: string[],
    options: RetrievalOptions = {}
  ): Promise<RetrievalResult> {
    const {
      limit = 10,
      includeScores = false,
    } = options;

    // Retrieve for each query
    const allResults: Array<{ chunk: DocumentChunk; score: number }> = [];

    for (const query of queries) {
      const result = await this.retrieve(query, {
        ...options,
        limit: limit * 2, // Get more per query to allow for deduplication
        includeScores: true,
      });

      // Add chunks with scores
      if (result.scores && result.chunks.length > 0) {
        for (let i = 0; i < result.chunks.length; i++) {
          allResults.push({
            chunk: result.chunks[i],
            score: result.scores[i],
          });
        }
      }
    }

    // Deduplicate by chunk ID, keeping highest score
    const chunkMap = new Map<string, { chunk: DocumentChunk; score: number }>();
    
    for (const result of allResults) {
      const existing = chunkMap.get(result.chunk.id);
      if (!existing || result.score > existing.score) {
        chunkMap.set(result.chunk.id, result);
      }
    }

    // Sort by score and limit
    const sortedResults = Array.from(chunkMap.values())
      .sort((a, b) => b.score - a.score)
      .slice(0, limit);

    return {
      chunks: sortedResults.map(r => r.chunk),
      scores: includeScores ? sortedResults.map(r => r.score) : undefined,
      queryEmbedding: await this.embeddingService.generateEmbedding(queries.join(' ')),
    };
  }

  /**
   * Retrieve chunks specifically for GraphQL schema context
   * 
   * Specialized retrieval for schema-related queries.
   * Filters for schema-type chunks and ensures schema relevance.
   */
  async retrieveSchemaContext(
    query: string,
    options: Omit<RetrievalOptions, 'type'> = {}
  ): Promise<RetrievalResult> {
    return this.retrieve(query, {
      ...options,
      type: 'schema',
      minSimilarity: options.minSimilarity ?? 0.25, // Lower threshold for schema
    });
  }

  /**
   * Retrieve chunks specifically for documentation context
   * 
   * Specialized retrieval for general documentation queries.
   */
  async retrieveDocumentationContext(
    query: string,
    options: Omit<RetrievalOptions, 'type'> = {}
  ): Promise<RetrievalResult> {
    return this.retrieve(query, {
      ...options,
      type: 'documentation',
      minSimilarity: options.minSimilarity ?? 0.3,
    });
  }

  /**
   * Compute cosine similarity between two embeddings
   */
  private computeSimilarity(a: number[], b: number[]): number {
    if (a.length !== b.length) {
      throw new Error('Vectors must have same length');
    }

    let dotProduct = 0;
    let normA = 0;
    let normB = 0;

    for (let i = 0; i < a.length; i++) {
      dotProduct += a[i] * b[i];
      normA += a[i] * a[i];
      normB += b[i] * b[i];
    }

    const denominator = Math.sqrt(normA) * Math.sqrt(normB);
    if (denominator === 0) return 0;

    return dotProduct / denominator;
  }

  /**
   * Get availability status
   */
  async checkAvailability(): Promise<{
    vectorStore: boolean;
    embeddingService: boolean;
  }> {
    try {
      // Check vector store by trying to get chunks
      const vectorStoreAvailable = await this.vectorStore.healthCheck();
      
      // Check embedding service by generating a test embedding
      const testEmbedding = await this.embeddingService.generateEmbedding('test');
      
      return {
        vectorStore: vectorStoreAvailable,
        embeddingService: testEmbedding.length > 0,
      };
    } catch (error) {
      return {
        vectorStore: false,
        embeddingService: false,
      };
    }
  }
}

