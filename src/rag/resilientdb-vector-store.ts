import { ResilientDBClient } from '../resilientdb/client';
import { SchemaContext } from '../types';
import env from '../config/environment';
import axios from 'axios';

/**
 * ResilientDB Vector Store
 * 
 * Uses ResilientDB itself as the vector database for RAG.
 * This showcases ResilientDB's versatility and aligns with course objectives.
 * 
 * Stores document chunks as assets in ResilientDB with:
 * - chunkText: The actual text content
 * - embedding: Vector embedding (as JSON array)
 * - metadata: Source, type, indexing info
 */
export interface DocumentChunk {
  id: string;
  chunkText: string;
  embedding: number[];
  source: string;
  chunkIndex: number;
  metadata: {
    documentId: string;
    section?: string;
    type: 'documentation' | 'schema' | 'example' | 'api';
    timestamp?: string;
  };
}

export class ResilientDBVectorStore {
  private client: ResilientDBClient;
  private schemaCache: SchemaContext | null = null;
  private mutationName: string | null = null;
  private queryName: string | null = null;

  constructor() {
    this.client = new ResilientDBClient();
  }

  /**
   * Introspect the schema to find the correct mutation and query names
   * This ensures we use the actual API, not assumptions
   */
  private async discoverSchema(): Promise<void> {
    if (this.schemaCache && this.mutationName && this.queryName) {
      return; // Already discovered
    }

    try {
      const schema = await this.client.introspectSchema();
      this.schemaCache = schema;

      // Find mutation name - look for common patterns
      const mutationNames = schema.mutations.map(m => m.name.toLowerCase());
      const possibleMutations = [
        'posttransaction',
        'createtransaction',
        'createasset',
        'addtransaction',
        'addasset',
      ];
      
      for (const possible of possibleMutations) {
        if (mutationNames.includes(possible)) {
          // Find the actual name (case-sensitive)
          const actual = schema.mutations.find(
            m => m.name.toLowerCase() === possible
          );
          if (actual) {
            this.mutationName = actual.name;
            break;
          }
        }
      }

      // Find query name - look for common patterns
      const queryNames = schema.queries.map(q => q.name.toLowerCase());
      const possibleQueries = [
        'transactions',
        'gettransactions',
        'alltransactions',
        'assets',
        'getassets',
        'allassets',
        'querytransactions',
        'gettransaction', // Singular form (requires ID, but we'll note it)
      ];

      for (const possible of possibleQueries) {
        if (queryNames.includes(possible)) {
          // Find the actual name (case-sensitive)
          const actual = schema.queries.find(
            q => q.name.toLowerCase() === possible
          );
          if (actual) {
            this.queryName = actual.name;
            break;
          }
        }
      }

      if (!this.mutationName) {
        console.warn(
          '⚠️  Could not find mutation in schema. Available mutations:',
          schema.mutations.map(m => m.name).join(', ')
        );
      }

      if (!this.queryName) {
        console.warn(
          '⚠️  Could not find query in schema. Available queries:',
          schema.queries.map(q => q.name).join(', ')
        );
      }
    } catch (error) {
      console.warn(
        '⚠️  Failed to introspect schema, will use fallback patterns:',
        error instanceof Error ? error.message : String(error)
      );
    }
  }

  /**
   * Store a document chunk in ResilientDB as an asset
   */
  async storeChunk(chunk: {
    chunkText: string;
    embedding: number[];
    source: string;
    chunkIndex: number;
    metadata: Record<string, unknown>;
  }): Promise<string> {
    // Discover the actual schema first
    await this.discoverSchema();

    // Generate a unique ID for this chunk
    const chunkId = `chunk_${Date.now()}_${chunk.chunkIndex}_${Math.random().toString(36).substring(2, 9)}`;
    
    // Prepare the asset data
    // For ResilientDB HTTP API, we use simple key-value format: {"id": "...", "value": {...}}
    // The value contains our chunk data
    const chunkData = {
      id: chunkId,
      type: 'DocumentChunk',
      chunkText: chunk.chunkText,
      embedding: chunk.embedding,
      source: chunk.source,
      chunkIndex: chunk.chunkIndex,
      metadata: {
        ...chunk.metadata,
        timestamp: new Date().toISOString(),
      },
    };

    // Use GraphQL as primary method (as per project requirements)
    // GraphQL is the main interface for ResilientDB per the proposal
    try {
      await this.discoverSchema();
      
      const signerPublicKey = env.RESILIENTDB_SIGNER_PUBLIC_KEY || 'default_signer_public_key';
      const signerPrivateKey = env.RESILIENTDB_SIGNER_PRIVATE_KEY || 'default_signer_private_key';
      const recipientPublicKey = env.RESILIENTDB_RECIPIENT_PUBLIC_KEY || signerPublicKey;

      // GraphQL format: asset needs to be passed as JSON string for JSONScalar
      // The JSONScalar expects a string representation, not an object
      const graphqlAsset = JSON.stringify({
        data: chunkData,
      });

      const result = await this.client.executeQuery<{
        postTransaction?: {
          id: string;
        };
      }>({
        query: `
          mutation StoreChunk($data: PrepareAsset!) {
            postTransaction(data: $data) {
              id
            }
          }
        `,
        variables: {
          data: {
            operation: 'CREATE',
            amount: 1,
            signerPublicKey: signerPublicKey,
            signerPrivateKey: signerPrivateKey,
            recipientPublicKey: recipientPublicKey,
            asset: graphqlAsset, // Pass as JSON string for JSONScalar
          },
        },
      });

      if (result.postTransaction?.id) {
        return result.postTransaction.id;
      }
      
      return chunkId;
    } catch (graphqlError) {
      // If GraphQL fails, try HTTP REST API as fallback
      const errorMsg = graphqlError instanceof Error ? graphqlError.message : String(graphqlError);
      
      try {
        // Extract base URL from GraphQL URL (e.g., http://localhost:5001/graphql -> http://localhost:5001)
        let resilientDBUrl = env.RESILIENTDB_GRAPHQL_URL.replace('/graphql', '');
        // Try HTTP wrapper on 18001 if available
        if (resilientDBUrl.includes(':5001')) {
          resilientDBUrl = resilientDBUrl.replace(':5001', ':18001');
        }
        const httpApiUrl = `${resilientDBUrl}/v1/transactions/commit`;
        
        const response = await fetch(httpApiUrl, {
          method: 'POST',
          headers: {
            'Content-Type': 'application/json',
          },
          body: JSON.stringify({
            id: chunkId,
            value: chunkData,
          }),
        });

        if (!response.ok) {
          const errorText = await response.text();
          throw new Error(`HTTP ${response.status}: ${errorText}`);
        }

        const result = await response.json();
        return result.id || chunkId;
      } catch (httpError) {
        // Both methods failed
        throw new Error(
          `Failed to store chunk in ResilientDB via both GraphQL and HTTP API.\n` +
          `GraphQL Error: ${errorMsg}\n` +
          `HTTP Error: ${httpError instanceof Error ? httpError.message : String(httpError)}\n` +
          `Please verify ResilientDB GraphQL is running and accessible at ${env.RESILIENTDB_GRAPHQL_URL}`
        );
      }
    }
  }

  /**
   * Retrieve all transactions via HTTP REST API
   * Fallback when GraphQL doesn't support listing all transactions
   */
  private async getAllTransactionsViaHTTP(): Promise<Array<{
    id: string;
    value?: string | Record<string, unknown>;
    timestamp?: string;
  }>> {
    try {
      // Extract base URL from GraphQL URL (e.g., http://localhost:18000/graphql -> http://localhost:18000)
      const graphqlUrl = new URL(env.RESILIENTDB_GRAPHQL_URL);
      let baseUrl = `${graphqlUrl.protocol}//${graphqlUrl.host}`;
      
      // Try HTTP wrapper on 18001 if available (where chunks are actually stored)
      if (baseUrl.includes(':5001')) {
        baseUrl = baseUrl.replace(':5001', ':18001');
      } else if (baseUrl.includes(':18000') && !baseUrl.includes(':18001')) {
        baseUrl = baseUrl.replace(':18000', ':18001');
      }
      
      // First, get list of all transaction IDs
      const listUrl = `${baseUrl}/v1/transactions`;
      const listResponse = await axios.get(listUrl, {
        headers: env.RESILIENTDB_API_KEY
          ? { Authorization: `Bearer ${env.RESILIENTDB_API_KEY}` }
          : {},
      });

      // Parse list of IDs
      let ids: string[] = [];
      if (Array.isArray(listResponse.data)) {
        // Response is array of { id: string } or just strings
        ids = listResponse.data.map((item: unknown) => {
          if (typeof item === 'string') {
            return item;
          }
          if (typeof item === 'object' && item !== null && 'id' in item) {
            return (item as { id: string }).id;
          }
          return String(item);
        });
      } else if (listResponse.data && typeof listResponse.data === 'object') {
        // Try to extract IDs from object
        ids = Object.keys(listResponse.data);
      }

      // Fetch each transaction individually (limit to first N for performance)
      const transactions: Array<{
        id: string;
        value?: string | Record<string, unknown>;
        timestamp?: string;
      }> = [];

      // Limit to reasonable number to avoid overwhelming the server
      const maxIds = Math.min(ids.length, 1000);
      
      for (let i = 0; i < maxIds; i++) {
        try {
          const id = ids[i];
          const detailUrl = `${baseUrl}/v1/transactions/${id}`;
          const detailResponse = await axios.get(detailUrl, {
            headers: env.RESILIENTDB_API_KEY
              ? { Authorization: `Bearer ${env.RESILIENTDB_API_KEY}` }
              : {},
          });

          if (detailResponse.data) {
            const data = detailResponse.data;
            if (typeof data === 'object' && data !== null) {
              transactions.push({
                id: data.id || id,
                value: data.value || data,
                timestamp: data.timestamp,
              });
            }
          }
        } catch (error) {
          // Skip individual failures, continue with others
          continue;
        }
      }

      return transactions;
    } catch (error) {
      throw new Error(
        `Failed to retrieve transactions via HTTP API: ${error instanceof Error ? error.message : String(error)}`
      );
    }
  }

  /**
   * Retrieve all document chunks from ResilientDB
   */
  async getAllChunks(limit: number = 1000): Promise<DocumentChunk[]> {
    // Discover the actual schema first
    await this.discoverSchema();

    // Try discovered query first, then fallback patterns
    const queriesToTry: Array<{
      name: string;
      query: string;
      variables: Record<string, unknown>;
    }> = [];

    if (this.queryName) {
      // Use discovered query name
      const query = this.queryName;
      
      queriesToTry.push(
        {
          name: `${query} (pattern 1)`,
          query: `
            query GetDocumentChunks($limit: Int, $type: String) {
              ${query}(limit: $limit, filter: { assetType: $type }) {
                id
                timestamp
                asset {
                  id
                  type
                  chunkText
                  embedding
                  source
                  chunkIndex
                  metadata
                }
              }
            }
          `,
          variables: {
            limit,
            type: 'DocumentChunk',
          },
        },
        {
          name: `${query} (pattern 2)`,
          query: `
            query GetDocumentChunks($limit: Int) {
              ${query}(limit: $limit) {
                id
                timestamp
                asset
              }
            }
          `,
          variables: { limit },
        }
      );
    }

    // Add fallback query patterns
    const fallbackQueries = [
      {
        name: 'transactions (fallback)',
        query: `
          query GetDocumentChunks($limit: Int, $type: String) {
            transactions(limit: $limit, filter: { assetType: $type }) {
              id
              timestamp
              asset {
                id
                type
                chunkText
                embedding
                source
                chunkIndex
                metadata
              }
            }
          }
        `,
        variables: {
          limit,
          type: 'DocumentChunk',
        },
      },
      {
        name: 'assets (fallback)',
        query: `
          query GetAssets($limit: Int, $type: String) {
            assets(limit: $limit, type: $type) {
              id
              type
              chunkText
              embedding
              source
              chunkIndex
              metadata
            }
          }
        `,
        variables: { limit, type: 'DocumentChunk' },
      },
      {
        name: 'allTransactions (fallback)',
        query: `
          query GetAllTransactions($limit: Int) {
            allTransactions(limit: $limit) {
              id
              timestamp
              asset
            }
          }
        `,
        variables: { limit },
      },
    ];

    // Try all queries in order
    const allQueries = [...queriesToTry, ...fallbackQueries];
    const errors: string[] = [];

    for (const queryPattern of allQueries) {
      try {
        const result = await this.client.executeQuery<{
          [key: string]: Array<{
            id: string;
            timestamp?: string;
            asset?: string | {
              id?: string;
              type?: string;
              chunkText?: string;
              embedding?: number[];
              source?: string;
              chunkIndex?: number;
              metadata?: Record<string, unknown>;
            };
          }>;
        }>({
          query: queryPattern.query,
          variables: queryPattern.variables,
        });

        // Find the first array result (query name might vary)
        const transactionsArray = Object.values(result).find(
          Array.isArray
        );
        
        if (!transactionsArray || !Array.isArray(transactionsArray)) {
          continue; // Try next query pattern
        }

        const transactions = transactionsArray as Array<{
          id: string;
          timestamp?: string;
          asset?: string | Record<string, unknown>;
        }>;

        if (!transactions) {
          continue; // Try next query pattern
        }

        // Parse and transform results to DocumentChunk format
        const chunks: DocumentChunk[] = [];
        
        for (const transaction of transactions) {
          // Handle asset - could be string (JSON) or object
          const asset = typeof transaction.asset === 'string' 
            ? JSON.parse(transaction.asset) 
            : transaction.asset;
          
          if (!asset || asset.type !== 'DocumentChunk') {
            continue;
          }

          if (!asset.chunkText || !asset.embedding || !asset.source) {
            continue; // Skip invalid chunks
          }

          chunks.push({
            id: asset.id || transaction.id,
            chunkText: asset.chunkText,
            embedding: asset.embedding,
            source: asset.source,
            chunkIndex: asset.chunkIndex ?? 0,
            metadata: {
              documentId: asset.metadata?.documentId || '',
              section: asset.metadata?.section,
              type: (asset.metadata?.type as DocumentChunk['metadata']['type']) || 'documentation',
              timestamp: asset.metadata?.timestamp || transaction.timestamp,
            },
          });
        }

        return chunks.slice(0, limit);
      } catch (error) {
        const errorMsg = error instanceof Error ? error.message : String(error);
        errors.push(`${queryPattern.name}: ${errorMsg}`);
        // Continue to next query pattern
      }
    }

    // If all GraphQL queries fail, try HTTP REST API as fallback
    console.log('⚠️  GraphQL queries failed, trying HTTP REST API fallback...');
    try {
      const httpTransactions = await this.getAllTransactionsViaHTTP();
      
      // Parse and transform HTTP response to DocumentChunk format
      const chunks: DocumentChunk[] = [];
      
      for (const transaction of httpTransactions) {
        // Handle value - could be string (JSON) or object
        let asset: Record<string, unknown>;
        
        if (typeof transaction.value === 'string') {
          try {
            const parsed = JSON.parse(transaction.value);
            asset = typeof parsed === 'object' && parsed !== null ? parsed : { value: transaction.value };
          } catch {
            continue; // Skip invalid transactions that can't be parsed
          }
        } else if (transaction.value && typeof transaction.value === 'object') {
          asset = transaction.value as Record<string, unknown>;
        } else {
          continue; // Skip transactions without value
        }
        
        // Check if this is a DocumentChunk
        if (asset.type !== 'DocumentChunk') {
          continue;
        }

        if (!asset.chunkText || !asset.embedding || !asset.source) {
          continue; // Skip invalid chunks
        }

        chunks.push({
          id: asset.id as string || transaction.id,
          chunkText: asset.chunkText as string,
          embedding: asset.embedding as number[],
          source: asset.source as string,
          chunkIndex: (asset.chunkIndex as number) ?? 0,
          metadata: {
            documentId: (asset.metadata as Record<string, unknown>)?.documentId as string || '',
            section: (asset.metadata as Record<string, unknown>)?.section as string,
            type: ((asset.metadata as Record<string, unknown>)?.type as DocumentChunk['metadata']['type']) || 'documentation',
            timestamp: (asset.metadata as Record<string, unknown>)?.timestamp as string || transaction.timestamp,
          },
        });
      }

      return chunks.slice(0, limit);
    } catch (httpError) {
      // If HTTP also fails, throw comprehensive error
      throw new Error(
        `Failed to retrieve chunks from ResilientDB. Tried ${allQueries.length} GraphQL query patterns and HTTP REST API.\n` +
        `GraphQL Errors: ${errors.slice(0, 3).join('; ')}${errors.length > 3 ? '...' : ''}\n` +
        `HTTP Error: ${httpError instanceof Error ? httpError.message : String(httpError)}\n` +
        `Please verify your ResilientDB is accessible and contains document chunks. ` +
        `Available queries: ${this.schemaCache?.queries.map(q => q.name).join(', ') || 'unknown'}`
      );
    }
  }

  /**
   * Semantic search: Find chunks similar to query embedding
   * 
   * Retrieves all chunks, computes cosine similarity in-memory,
   * and returns top-k most similar chunks.
   */
  async searchSimilar(
    queryEmbedding: number[],
    limit: number = 10,
    filters?: {
      source?: string;
      type?: DocumentChunk['metadata']['type'];
    }
  ): Promise<DocumentChunk[]> {
    // Get all chunks (or filtered subset)
    const allChunks = await this.getAllChunks();
    
    // Apply filters if provided
    let filteredChunks = allChunks;
    if (filters) {
      filteredChunks = allChunks.filter(chunk => {
        if (filters.source && chunk.source !== filters.source) return false;
        if (filters.type && chunk.metadata.type !== filters.type) return false;
        return true;
      });
    }

    // Compute cosine similarity for each chunk
    const similarities = filteredChunks.map(chunk => ({
      chunk,
      similarity: this.cosineSimilarity(queryEmbedding, chunk.embedding),
    }));

    // Sort by similarity (descending) and return top-k
    return similarities
      .sort((a, b) => b.similarity - a.similarity)
      .slice(0, limit)
      .map(item => item.chunk);
  }

  /**
   * Health check for the vector store
   */
  async healthCheck(): Promise<boolean> {
    try {
      // Try to get at least one chunk to verify storage is accessible
      const chunks = await this.getAllChunks();
      return chunks.length >= 0; // Even 0 chunks means the store is accessible
    } catch {
      return false;
    }
  }

  /**
   * Compute cosine similarity between two vectors
   */
  private cosineSimilarity(a: number[], b: number[]): number {
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
   * Get chunks by metadata filter
   */
  async getChunksByMetadata(
    filters: {
      source?: string;
      type?: DocumentChunk['metadata']['type'];
      documentId?: string;
    }
  ): Promise<DocumentChunk[]> {
    const allChunks = await this.getAllChunks();
    
    return allChunks.filter(chunk => {
      if (filters.source && chunk.source !== filters.source) return false;
      if (filters.type && chunk.metadata.type !== filters.type) return false;
      if (filters.documentId && chunk.metadata.documentId !== filters.documentId) return false;
      return true;
    });
  }

  /**
   * Delete a chunk (if needed)
   */
  async deleteChunk(_chunkId: string): Promise<void> {
    // TODO: Implement using ResilientDB mutations if deletion is supported
    throw new Error('Not yet implemented');
  }

  /**
   * Get chunk count
   */
  async getChunkCount(): Promise<number> {
    const chunks = await this.getAllChunks();
    return chunks.length;
  }
}
