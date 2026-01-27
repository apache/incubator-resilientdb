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

import { HfInference } from '@huggingface/inference';
import env from '../config/environment';

type LocalPipeline = (input: string | string[], options?: Record<string, unknown>) => Promise<any>;

// Direct HTTP API calls using the Inference API endpoint
// This bypasses the SDK's Inference Providers feature which requires special permissions
async function fetchEmbeddingDirect(
  model: string,
  inputs: string | string[],
  apiKey?: string
): Promise<number[][]> {
  // Use the router endpoint for models
  // Format: https://router.huggingface.co/hf-inference/models/{model}
  const url = `https://router.huggingface.co/hf-inference/models/${model}`;
  
  const headers: Record<string, string> = {
    'Content-Type': 'application/json',
  };
  
  if (apiKey) {
    headers['Authorization'] = `Bearer ${apiKey}`;
  }

  // For feature extraction, the API expects "inputs" key with text(s) directly
  // The inputs can be a string or array of strings
  const response = await fetch(url, {
    method: 'POST',
    headers,
    body: JSON.stringify({ 
      inputs: inputs  // Send inputs directly (string or array of strings)
    }),
  });

  if (!response.ok) {
    const errorText = await response.text();
    let errorMessage = `HTTP ${response.status}: ${errorText}`;
    
    if (response.status === 429) {
      errorMessage = `Rate limit exceeded: ${errorText}`;
    } else if (response.status === 401 || response.status === 403) {
      errorMessage = `Authentication failed: ${errorText}`;
    } else if (response.status === 503) {
      // Model is loading - wait and retry
      errorMessage = `Model is loading, please wait: ${errorText}`;
    }
    
    throw new Error(errorMessage);
  }

  const data = await response.json();
  
  // Response format: array of arrays (one array per input)
  if (Array.isArray(data)) {
    if (data.length > 0) {
      if (Array.isArray(data[0])) {
        return data as number[][];
      } else if (typeof data[0] === 'number') {
        return [data as number[]];
      }
    }
  }
  
  throw new Error('Invalid response format from Hugging Face API');
}

/**
 * Embedding Service
 * 
 * Generates vector embeddings for text using Hugging Face.
 * This service is used to convert document chunks into vectors for semantic search.
 * 
 * Uses Hugging Face: sentence-transformers/all-MiniLM-L6-v2 (384 dimensions)
 */
export class EmbeddingService {
  private hfClient: HfInference | null = null;
  private model: string;
  private dimension: number;
  private provider: 'huggingface' | 'local';
  private localPipelinePromise?: Promise<LocalPipeline>;

  constructor() {
    this.model = 'sentence-transformers/all-MiniLM-L6-v2';
    this.dimension = 384;
    this.provider = env.EMBEDDINGS_PROVIDER;

    if (this.provider === 'local') {
      this.localPipelinePromise = this.initLocalPipeline();
    } else {
      if (env.HUGGINGFACE_API_KEY) {
        this.hfClient = new HfInference(env.HUGGINGFACE_API_KEY);
      } else {
        this.hfClient = new HfInference();
      }
    }
  }

  private async initLocalPipeline(): Promise<LocalPipeline> {
    const transformers = await import('@xenova/transformers');
    const pipeline = await transformers.pipeline('feature-extraction', 'Xenova/all-MiniLM-L6-v2');
    return pipeline;
  }

  /**
   * Check if the embedding service is available
   */
  isAvailable(): boolean {
    if (this.provider === 'local') {
      return true;
    }
    return this.hfClient !== null;
  }

  /**
   * Generate embedding for a single text
   * 
   * @param text - The text to generate embedding for
   * @returns Promise<number[]> - The embedding vector
   */
  async generateEmbedding(text: string): Promise<number[]> {
    if (!text || text.trim().length === 0) {
      throw new Error('Text cannot be empty');
    }

    const trimmedText = text.trim();
    if (this.provider === 'local') {
      return this.generateEmbeddingLocal(trimmedText);
    }
    return this.generateEmbeddingHuggingFace(trimmedText);
  }

  private async generateEmbeddingLocal(text: string): Promise<number[]> {
    if (!this.localPipelinePromise) {
      this.localPipelinePromise = this.initLocalPipeline();
    }
    const pipeline = await this.localPipelinePromise;
    const output = await pipeline(text, {
      pooling: 'mean',
      normalize: true,
    });
    // output.data may be typed array
    if (Array.isArray(output)) {
      return output[0] as number[];
    }
    if (output && Array.isArray(output.data)) {
      return output.data as number[];
    }
    if (output?.data instanceof Float32Array) {
      return Array.from(output.data);
    }
    throw new Error('Failed to generate embedding locally');
  }

  /**
   * Generate embedding using Hugging Face
   */
  private async generateEmbeddingHuggingFace(text: string): Promise<number[]> {
    if (!this.hfClient) {
      throw new Error(
        'Hugging Face client not initialized. Please set HUGGINGFACE_API_KEY environment variable (optional for public access).'
      );
    }

    try {
      // Try using SDK's featureExtraction method first
      // If it fails with Inference Providers error, fall back to direct HTTP
      try {
        const response = await this.hfClient.featureExtraction({
          model: this.model,
          inputs: text,
        });

        // Handle response format
        if (Array.isArray(response)) {
          if (response.length === 0) {
            throw new Error('No embedding returned');
          }
          if (typeof response[0] === 'number') {
            return response as number[];
          } else if (Array.isArray(response[0])) {
            return response[0] as number[];
          }
        }
        throw new Error('Invalid response format');
      } catch (sdkError) {
        // If SDK fails with Inference Providers error, use direct HTTP
        const errorMsg = sdkError instanceof Error ? sdkError.message : String(sdkError);
        if (errorMsg.includes('Inference Providers') || errorMsg.includes('permissions')) {
          const embeddings = await fetchEmbeddingDirect(
            this.model,
            text,
            env.HUGGINGFACE_API_KEY
          );
          if (embeddings.length > 0 && embeddings[0].length > 0) {
            return embeddings[0];
          }
        }
        throw sdkError;
      }

    } catch (error) {
      const errorMsg = error instanceof Error ? error.message : String(error);
      
      // Provide helpful error messages
      if (errorMsg.includes('rate limit') || errorMsg.includes('429')) {
        throw new Error(
          `Hugging Face rate limit reached. Set HUGGINGFACE_API_KEY for higher limits. Original error: ${errorMsg}`
        );
      }
      if (errorMsg.includes('401') || errorMsg.includes('Unauthorized')) {
        throw new Error(
          `Hugging Face authentication failed. Check your HUGGINGFACE_API_KEY. Original error: ${errorMsg}`
        );
      }
      
      throw new Error(
        `Failed to generate embedding with Hugging Face: ${errorMsg}. ` +
        `Note: Public API may be rate-limited. Consider setting HUGGINGFACE_API_KEY for better reliability.`
      );
    }
  }

  /**
   * Generate embeddings for multiple texts in batch
   * 
   * @param texts - Array of texts to generate embeddings for
   * @param batchSize - Number of texts to process in each batch (default: 100)
   * @returns Promise<number[][]> - Array of embedding vectors
   */
  async generateEmbeddings(
    texts: string[],
    batchSize: number = 100
  ): Promise<number[][]> {
    if (texts.length === 0) {
      return [];
    }

    // Filter out empty texts
    const validTexts = texts
      .map((text, index) => ({ text: text.trim(), index }))
      .filter(({ text }) => text.length > 0);

    if (validTexts.length === 0) {
      throw new Error('No valid texts provided');
    }

    const embeddings: number[][] = new Array(texts.length).fill(null);
    const errors: string[] = [];

    if (this.provider === 'local') {
      for (const item of validTexts) {
        try {
          embeddings[item.index] = await this.generateEmbeddingLocal(item.text);
        } catch (error) {
          errors.push(
            `Local embedding failed for chunk ${item.index + 1}: ${
              error instanceof Error ? error.message : String(error)
            }`
          );
          embeddings[item.index] = [];
        }
      }
    } else {
      // Hugging Face batch processing
      // Note: Hugging Face API handles batching automatically
      for (let i = 0; i < validTexts.length; i += batchSize) {
      const batch = validTexts.slice(i, i + batchSize);
      const batchTexts = batch.map(({ text }) => text);

      try {
        // Try SDK first, fall back to direct HTTP if needed
        let batchEmbeddings: number[][];
        try {
          const response = await this.hfClient!.featureExtraction({
            model: this.model,
            inputs: batchTexts,
          });
          
          // Handle response format
          if (Array.isArray(response)) {
            batchEmbeddings = response.map((emb: any) => {
              if (Array.isArray(emb)) return emb as number[];
              if (typeof emb === 'number') return [emb];
              return [];
            });
          } else {
            throw new Error('Invalid batch response format');
          }
        } catch (sdkError) {
          // If SDK fails, use direct HTTP
          const errorMsg = sdkError instanceof Error ? sdkError.message : String(sdkError);
          if (errorMsg.includes('Inference Providers') || errorMsg.includes('permissions')) {
            batchEmbeddings = await fetchEmbeddingDirect(
              this.model,
              batchTexts,
              env.HUGGINGFACE_API_KEY
            );
          } else {
            throw sdkError;
          }
        }

        // Map embeddings back to original indices
        batch.forEach((item, batchIndex) => {
          const originalIndex = item.index;
          if (batchEmbeddings[batchIndex] && batchEmbeddings[batchIndex].length > 0) {
            embeddings[originalIndex] = batchEmbeddings[batchIndex];
          } else {
            embeddings[originalIndex] = [];
          }
        });
      } catch (error) {
        const errorMsg = error instanceof Error ? error.message : String(error);
        errors.push(`Batch ${i / batchSize + 1}: ${errorMsg}`);
        console.error(`Failed to process batch ${i / batchSize + 1}:`, errorMsg);
      }
      }
    }

    if (errors.length > 0 && errors.length === Math.ceil(validTexts.length / batchSize)) {
      // All batches failed
      throw new Error(`All batches failed: ${errors.join('; ')}`);
    }

    // Fill null positions with empty arrays (for empty input texts)
    for (let i = 0; i < embeddings.length; i++) {
      if (embeddings[i] === null) {
        embeddings[i] = [];
      }
    }

    return embeddings;
  }

  /**
   * Get the embedding dimension
   */
  getDimension(): number {
    return this.dimension;
  }

  /**
   * Get the embedding model name
   */
  getModel(): string {
    return this.model;
  }

  /**
   * Get the current provider
   */
  getProvider(): 'huggingface' | 'local' {
    return this.provider;
  }

  /**
   * Set a custom embedding model (for future flexibility)
   */
  setModel(model: string, dimension: number): void {
    this.model = model;
    this.dimension = dimension;
  }
}
