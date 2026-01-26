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

import { DocumentLoader, LoadedDocument } from './document-loader';
import { ChunkingService } from './chunking-service';
import { EmbeddingService } from './embedding-service';
import { ResilientDBVectorStore } from './resilientdb-vector-store';
import { ResilientDBClient } from '../resilientdb/client';

/**
 * Ingestion Pipeline
 * 
 * End-to-end pipeline: load → chunk → embed → store
 * Orchestrates the entire document ingestion process
 */
export interface IngestionProgress {
  totalDocuments: number;
  processedDocuments: number;
  totalChunks: number;
  processedChunks: number;
  embeddedChunks: number;
  storedChunks: number;
  errors: Array<{ documentId: string; error: string }>;
}

export interface IngestionOptions {
  maxChunkTokens?: number;
  chunkOverlapTokens?: number;
  batchSize?: number;
  onProgress?: (progress: IngestionProgress) => void;
}

export class IngestionPipeline {
  private documentLoader: DocumentLoader;
  private chunkingService: ChunkingService;
  private embeddingService: EmbeddingService;
  private vectorStore: ResilientDBVectorStore;
  private resilientDBClient: ResilientDBClient;

  constructor(options: IngestionOptions = {}) {
    this.documentLoader = new DocumentLoader();
    this.chunkingService = new ChunkingService({
      maxTokens: options.maxChunkTokens,
      chunkOverlapTokens: options.chunkOverlapTokens,
    });
    this.embeddingService = new EmbeddingService();
    this.vectorStore = new ResilientDBVectorStore();
    this.resilientDBClient = new ResilientDBClient();
  }

  /**
   * Check if all services are available
   */
  async checkAvailability(): Promise<{
    embeddingService: boolean;
    vectorStore: boolean;
    message: string;
  }> {
    const embeddingAvailable = this.embeddingService.isAvailable();
    
    let vectorStoreAvailable = false;
    try {
      // Try to introspect schema to check connection
      await this.resilientDBClient.introspectSchema();
      vectorStoreAvailable = true;
    } catch {
      vectorStoreAvailable = false;
    }

    const missing: string[] = [];
    if (!embeddingAvailable) {
      missing.push('Embedding Service (Hugging Face - optional API key for better rate limits)');
    }
    if (!vectorStoreAvailable) missing.push('Vector Store (needs ResilientDB connection)');

    return {
      embeddingService: embeddingAvailable,
      vectorStore: vectorStoreAvailable,
      message: missing.length > 0
        ? `Missing: ${missing.join(', ')}`
        : 'All services available',
    };
  }

  /**
   * Ingest documents from a directory
   */
  async ingestDirectory(
    dirPath: string,
    options: IngestionOptions = {}
  ): Promise<IngestionProgress> {
    const progress: IngestionProgress = {
      totalDocuments: 0,
      processedDocuments: 0,
      totalChunks: 0,
      processedChunks: 0,
      embeddedChunks: 0,
      storedChunks: 0,
      errors: [],
    };

    try {
      // Step 1: Load documents
      const documents = await this.documentLoader.loadDirectory(dirPath);
      progress.totalDocuments = documents.length;

      if (documents.length === 0) {
        console.warn(`No documents found in ${dirPath}`);
        return progress;
      }

      // Step 2: Process documents
      await this.ingestDocuments(documents, progress, options);

      return progress;
    } catch (error) {
      progress.errors.push({
        documentId: 'directory_load',
        error: error instanceof Error ? error.message : String(error),
      });
      return progress;
    }
  }

  /**
   * Ingest a list of documents
   */
  async ingestDocuments(
    documents: LoadedDocument[],
    progress?: IngestionProgress,
    options: IngestionOptions = {}
  ): Promise<IngestionProgress> {
    const currentProgress = progress || {
      totalDocuments: documents.length,
      processedDocuments: 0,
      totalChunks: 0,
      processedChunks: 0,
      embeddedChunks: 0,
      storedChunks: 0,
      errors: [],
    };

    const batchSize = options.batchSize || 10;

    // Step 1: Chunk all documents
    const allChunks = this.chunkingService.chunkDocuments(documents);
    currentProgress.totalChunks = allChunks.length;

    if (allChunks.length === 0) {
      console.warn('No chunks generated from documents');
      return currentProgress;
    }

    // Step 2: Generate embeddings in batches
    // Use smaller batches for Hugging Face to avoid rate limits
    const embeddingBatchSize = Math.min(batchSize, 10); // Smaller batches for rate limit handling
    
    for (let i = 0; i < allChunks.length; i += embeddingBatchSize) {
      const batch = allChunks.slice(i, i + embeddingBatchSize);
      
      try {
        // Generate embeddings for batch
        const texts = batch.map(chunk => chunk.chunkText);
        
        // Add delay between batches to avoid rate limits (especially for public API)
        if (i > 0) {
          await new Promise(resolve => setTimeout(resolve, 2000)); // 2 second delay
        }
        
        const embeddings = await this.embeddingService.generateEmbeddings(texts, embeddingBatchSize);

        // Store chunks with embeddings
        for (let j = 0; j < batch.length; j++) {
          const chunk = batch[j];
          const embedding = embeddings[j];

          if (embedding && embedding.length > 0) {
            try {
              await this.vectorStore.storeChunk({
                chunkText: chunk.chunkText,
                embedding,
                source: chunk.source,
                chunkIndex: chunk.chunkIndex,
                metadata: {
                  documentId: chunk.metadata.documentId,
                  section: chunk.metadata.section,
                  type: chunk.metadata.type,
                  timestamp: new Date().toISOString(),
                },
              });

              currentProgress.storedChunks++;
              currentProgress.embeddedChunks++;
            } catch (error) {
              currentProgress.errors.push({
                documentId: chunk.metadata.documentId,
                error: error instanceof Error ? error.message : String(error),
              });
            }
          }

          currentProgress.processedChunks++;
          
          // Report progress if callback provided
          if (options.onProgress) {
            options.onProgress({ ...currentProgress });
          }
        }
      } catch (error) {
        // Batch failed - check if it's a rate limit error
        const errorMsg = error instanceof Error ? error.message : String(error);
        const isRateLimit = errorMsg.includes('rate limit') || errorMsg.includes('429') || errorMsg.includes('HTTP error');
        
        if (isRateLimit) {
          console.warn(`⏳ Rate limit detected on batch ${Math.floor(i / embeddingBatchSize) + 1}. Waiting 10 seconds before retry...`);
          await new Promise(resolve => setTimeout(resolve, 10000)); // Wait 10 seconds
          
          // Retry the batch once
          try {
            const texts = batch.map(chunk => chunk.chunkText);
            const embeddings = await this.embeddingService.generateEmbeddings(texts, embeddingBatchSize);
            
            // Store chunks with embeddings
            for (let j = 0; j < batch.length; j++) {
              const chunk = batch[j];
              const embedding = embeddings[j];

              if (embedding && embedding.length > 0) {
                try {
                  await this.vectorStore.storeChunk({
                    chunkText: chunk.chunkText,
                    embedding,
                    source: chunk.source,
                    chunkIndex: chunk.chunkIndex,
                    metadata: {
                      documentId: chunk.metadata.documentId,
                      section: chunk.metadata.section,
                      type: chunk.metadata.type,
                      timestamp: new Date().toISOString(),
                    },
                  });

                  currentProgress.storedChunks++;
                  currentProgress.embeddedChunks++;
                } catch (storeError) {
                  currentProgress.errors.push({
                    documentId: chunk.metadata.documentId,
                    error: storeError instanceof Error ? storeError.message : String(storeError),
                  });
                }
              }

              currentProgress.processedChunks++;
              
              if (options.onProgress) {
                options.onProgress({ ...currentProgress });
              }
            }
            
            console.log(`✅ Batch ${Math.floor(i / embeddingBatchSize) + 1} retried successfully`);
          } catch (retryError) {
            // Retry also failed, add errors
            const retryErrorMsg = retryError instanceof Error ? retryError.message : String(retryError);
            for (const chunk of batch) {
              currentProgress.errors.push({
                documentId: chunk.metadata.documentId,
                error: retryErrorMsg,
              });
              currentProgress.processedChunks++;
            }
          }
        } else {
          // Non-rate-limit error, just add errors
          for (const chunk of batch) {
            currentProgress.errors.push({
              documentId: chunk.metadata.documentId,
              error: errorMsg,
            });
            currentProgress.processedChunks++;
          }
        }
      }
    }

    currentProgress.processedDocuments = documents.length;

    return currentProgress;
  }

  /**
   * Ingest GraphQL schema information
   */
  async ingestSchema(): Promise<IngestionProgress> {
    const progress: IngestionProgress = {
      totalDocuments: 1,
      processedDocuments: 0,
      totalChunks: 0,
      processedChunks: 0,
      embeddedChunks: 0,
      storedChunks: 0,
      errors: [],
    };

    try {
      // Introspect schema
      const schema = await this.resilientDBClient.introspectSchema();
      const schemaContent = JSON.stringify(schema, null, 2);

      // Load schema as document
      const schemaDoc = await this.documentLoader.loadSchema(schemaContent, 'graphql_schema');

      // Ingest the schema document
      const result = await this.ingestDocuments([schemaDoc], progress);

      return result;
    } catch (error) {
      progress.errors.push({
        documentId: 'graphql_schema',
        error: error instanceof Error ? error.message : String(error),
      });
      return progress;
    }
  }

  /**
   * Get ingestion statistics
   */
  async getStats(): Promise<{
    totalChunks: number;
    chunksByType: Record<string, number>;
    chunksBySource: Record<string, number>;
  }> {
    try {
      const allChunks = await this.vectorStore.getAllChunks();
      
      const chunksByType: Record<string, number> = {};
      const chunksBySource: Record<string, number> = {};

      for (const chunk of allChunks) {
        const type = chunk.metadata.type || 'unknown';
        chunksByType[type] = (chunksByType[type] || 0) + 1;
        
        const source = chunk.source || 'unknown';
        chunksBySource[source] = (chunksBySource[source] || 0) + 1;
      }

      return {
        totalChunks: allChunks.length,
        chunksByType,
        chunksBySource,
      };
    } catch (error) {
      console.warn('Failed to get ingestion stats:', error);
      return {
        totalChunks: 0,
        chunksByType: {},
        chunksBySource: {},
      };
    }
  }
}

