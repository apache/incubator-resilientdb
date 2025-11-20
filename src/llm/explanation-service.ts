/**
 * Query Explanation Service
 * 
 * Generates natural language explanations for GraphQL queries using:
 * - RAG retrieval for relevant context
 * - LLM for generating explanations
 * - Live Stats integration for performance context
 */

import { LLMClient } from './llm-client';
import { RetrievalService } from '../rag/retrieval-service';
import { ContextFormatter } from '../rag/context-formatter';
import {
  formatExplanationPrompt,
  formatDetailedExplanationPrompt,
  EXPLANATION_SYSTEM_PROMPT,
  ExplanationPromptContext,
} from './prompts';
import { ResLensClient } from '../reslens/client';
import env from '../config/environment';
import { SimpleCache } from '../utils/cache';

export interface ExplanationOptions {
  /** Include detailed explanation (default: true) */
  detailed?: boolean;
  /** Include live stats in explanation */
  includeLiveStats?: boolean;
  /** Maximum context chunks to retrieve */
  maxContextChunks?: number;
  /** Minimum similarity threshold for retrieval */
  minSimilarity?: number;
}

export interface ExplanationResult {
  explanation: string;
  query: string;
  contextChunksUsed: number;
  liveStatsIncluded: boolean;
}

/**
 * Explanation Service
 * 
 * Generates explanations for GraphQL queries using RAG + LLM
 */
export class ExplanationService {
  private llmClient: LLMClient;
  private retrievalService: RetrievalService;
  private contextFormatter: ContextFormatter;
  private resLensClient: ResLensClient;
  private cache: SimpleCache<ExplanationResult>;
  private pendingRequests: Map<string, Promise<ExplanationResult>>;

  constructor() {
    this.llmClient = new LLMClient();
    this.retrievalService = new RetrievalService();
    this.contextFormatter = new ContextFormatter();
    this.resLensClient = new ResLensClient();
    // Cache for 1 hour (3600000ms) - reduces API calls for repeated queries
    this.cache = new SimpleCache<ExplanationResult>(3600000);
    // Deduplicate concurrent requests for the same query
    this.pendingRequests = new Map();
    
    // Clean expired cache entries every 30 minutes
    setInterval(() => this.cache.clearExpired(), 30 * 60 * 1000);
  }

  /**
   * Generate explanation for a GraphQL query
   * 
   * @param query - GraphQL query to explain
   * @param options - Explanation options
   * @returns Explanation result
   */
  async explain(
    query: string,
    options: ExplanationOptions = {}
  ): Promise<ExplanationResult> {
    // Check cache first (skip cache for questions to get fresh answers)
    const trimmedQuery = query.trim();
    const looksLikeQuery = trimmedQuery.startsWith('{') || 
                          trimmedQuery.match(/\{\s*\w+\s*\(/) ||
                          trimmedQuery.match(/query\s+|mutation\s+|subscription\s+/i);
    
    // Only cache GraphQL queries, not questions
    if (looksLikeQuery) {
      const cacheKey = JSON.stringify({ query: trimmedQuery, options });
      const cached = this.cache.get(trimmedQuery, { options });
      if (cached) {
        console.log(`[Cache] Hit for query: ${trimmedQuery.substring(0, 50)}...`);
        return cached;
      }

      // Check if there's already a pending request for this query
      if (this.pendingRequests.has(cacheKey)) {
        console.log(`[Deduplication] Reusing pending request for: ${trimmedQuery.substring(0, 50)}...`);
        return this.pendingRequests.get(cacheKey)!;
      }
    }

    try {
      // Create promise for this request (for deduplication)
      const cacheKey = JSON.stringify({ query: trimmedQuery, options });
      const requestPromise = this.executeExplain(query, options);
      
      if (looksLikeQuery) {
        this.pendingRequests.set(cacheKey, requestPromise);
      }

      const result = await requestPromise;

      // Cache the result (only for GraphQL queries)
      if (looksLikeQuery) {
        this.cache.set(trimmedQuery, result, { options });
        this.pendingRequests.delete(cacheKey);
      }

      return result;
    } catch (error) {
      // Remove from pending on error
      const cacheKey = JSON.stringify({ query: trimmedQuery, options });
      this.pendingRequests.delete(cacheKey);
      throw error;
    }
  }

  /**
   * Internal method to execute explanation (without caching)
   */
  private async executeExplain(
    query: string,
    options: ExplanationOptions = {}
  ): Promise<ExplanationResult> {
    try {
      // Check if this is a question (not a GraphQL query) - if so, use RAG to answer it
      const trimmedQuery = query.trim();
      const looksLikeQuery = trimmedQuery.startsWith('{') || 
                            trimmedQuery.match(/\{\s*\w+\s*\(/) ||
                            trimmedQuery.match(/query\s+|mutation\s+|subscription\s+/i);
      
      // If it doesn't look like a GraphQL query, treat it as a question and use RAG
      if (!looksLikeQuery && trimmedQuery.length > 0 && !trimmedQuery.startsWith('{')) {
        // Use RAG to answer the question using ingested documentation
        let docResult: { chunks: any[] };
        try {
          docResult = await this.retrievalService.retrieveDocumentationContext(trimmedQuery, {
            limit: 5,
            minSimilarity: 0.2, // Lower threshold for questions
            includeScores: false,
          });
          console.log(`RAG retrieval for question "${trimmedQuery}": found ${docResult.chunks.length} chunks`);
        } catch (error) {
          console.warn('RAG retrieval failed for question, using fallback:', error instanceof Error ? error.message : String(error));
          docResult = { chunks: [] };
        }
        
        // Generate answer from retrieved documentation using LLM
        if (docResult.chunks.length > 0) {
          const topChunks = docResult.chunks.slice(0, 3);
          const contextText = topChunks.map((chunk, index) => {
            const cleanChunk = chunk.chunkText
              .replace(/^\[Source:[^\]]+\]\s*/gm, '')
              .replace(/^---\s*$/gm, '')
              .trim();
            return `[Document ${index + 1}]\n${cleanChunk}`;
          }).join('\n\n');
          
          // Use LLM to generate a proper answer from the context
          try {
            const response = await this.llmClient.generateCompletion([
              { 
                role: 'system', 
                content: 'You are a helpful assistant that answers questions about GraphQL and ResilientDB using the provided documentation. Provide clear, concise answers based only on the documentation provided.' 
              },
              { 
                role: 'user', 
                content: `Question: ${trimmedQuery}\n\nDocumentation:\n${contextText}\n\nAnswer the question using the documentation above. Be clear and concise.` 
              },
            ], {
              temperature: 0.7,
              maxTokens: 1000,
            });
            
            return {
              explanation: response.content,
              query: trimmedQuery,
              contextChunksUsed: docResult.chunks.length,
              liveStatsIncluded: false,
            };
          } catch (llmError) {
            // Fallback to raw chunks if LLM fails
            console.warn('LLM generation failed for question, using raw chunks:', llmError instanceof Error ? llmError.message : String(llmError));
            let answer = `**Answer to your question:**\n\n`;
            topChunks.forEach((chunk, index) => {
              const cleanChunk = chunk.chunkText
                .replace(/^\[Source:[^\]]+\]\s*/gm, '')
                .replace(/^---\s*$/gm, '')
                .trim();
              
              if (index === 0) {
                answer += `${cleanChunk.substring(0, 800)}`;
                if (cleanChunk.length > 800) answer += '...';
              } else {
                answer += `\n\n**Additional information:**\n${cleanChunk.substring(0, 400)}`;
                if (cleanChunk.length > 400) answer += '...';
              }
            });
            
            return {
              explanation: answer,
              query: trimmedQuery,
              contextChunksUsed: docResult.chunks.length,
              liveStatsIncluded: false,
            };
          }
        } else {
          // No documentation found, provide general guidance
          return {
            explanation: `I couldn't find specific documentation to answer "${trimmedQuery}".\n\n**To get an explanation of a GraphQL query, please provide a valid query like:**\n- \`{ getTransaction(id: "test") { asset } }\`\n- \`{ getTransaction(id: "123") { asset amount timestamp } }\`\n\n**What is GraphQL?**\nGraphQL is a query language for APIs that allows you to request exactly the data you need. In ResilientDB, you can use GraphQL to query transaction data.`,
            query: trimmedQuery,
            contextChunksUsed: 0,
            liveStatsIncluded: false,
          };
        }
      }
      
      // Detect if using local model with small context window (e.g., GPT-2)
      const isLocalSmallModel = this.llmClient.getProvider() === 'local' && 
        (this.llmClient.getModel().includes('gpt2') || this.llmClient.getModel().includes('distil'));
      
      // Check if using Gemini (for token optimization)
      const isGeminiProvider = this.llmClient.getProvider() === 'gemini';
    
      // Adjust context size for small models and optimize for Gemini
      const defaultMaxChunks = isLocalSmallModel ? 2 : (isGeminiProvider ? 5 : 10); // Reduce chunks for Gemini
      const defaultDocTokens = isLocalSmallModel ? 300 : (isGeminiProvider ? 1000 : 2000); // Reduce tokens for Gemini
      const defaultSchemaTokens = isLocalSmallModel ? 150 : (isGeminiProvider ? 500 : 1000); // Reduce tokens for Gemini

    const {
      detailed = true,
      includeLiveStats = env.LLM_ENABLE_LIVE_STATS || false,
      maxContextChunks = defaultMaxChunks,
      minSimilarity = 0.3,
    } = options;

    // Step 1: Retrieve relevant context using RAG (with error handling)
    let docResult: { chunks: any[] };
    let schemaResult: { chunks: any[] };
    
    try {
      [docResult, schemaResult] = await Promise.all([
        this.retrievalService.retrieveDocumentationContext(query, {
          limit: maxContextChunks,
          minSimilarity,
          includeScores: false,
        }),
        this.retrievalService.retrieveSchemaContext(query, {
          limit: Math.floor(maxContextChunks / 2),
          minSimilarity: 0.25, // Lower threshold for schema
          includeScores: false,
        }),
      ]);
    } catch (error) {
      // If retrieval fails (e.g., ResilientDB not available), use empty results
      console.warn('RAG retrieval failed, using fallback explanation:', error instanceof Error ? error.message : String(error));
      docResult = { chunks: [] };
      schemaResult = { chunks: [] };
    }

    // Step 2: Format context for LLM
    const documentationContext = this.contextFormatter.formatForExplanation(
      docResult.chunks,
      undefined,
      { maxTokens: defaultDocTokens }
    );

    const schemaContext = schemaResult.chunks.length > 0
      ? this.contextFormatter.format(schemaResult.chunks, undefined, {
          maxTokens: defaultSchemaTokens,
          format: 'compact',
        })
      : undefined;

    // Step 3: Get live stats if enabled
    let liveStats: ExplanationPromptContext['liveStats'] | undefined;
    if (includeLiveStats) {
      try {
        // Try to get metrics for this query (if it was executed)
        const metrics = await this.resLensClient.getMetrics(query);

        if (metrics && metrics.query === query) {
          const resourceUsageStr = metrics.resourceUsage
            ? `CPU: ${metrics.resourceUsage.cpu || 'N/A'}%, Memory: ${metrics.resourceUsage.memory || 'N/A'}MB, Network: ${metrics.resourceUsage.network || 'N/A'}KB`
            : undefined;
          
          liveStats = {
            executionTime: metrics.executionTime,
            resourceUsage: resourceUsageStr,
            queryComplexity: metrics.resultSize ? `Result Size: ${metrics.resultSize} items` : undefined,
          };
        }
      } catch (error) {
        // Live stats not available, continue without them
        console.warn('Live stats not available:', error instanceof Error ? error.message : String(error));
      }
    }

    // Step 4: Format prompt
    // For small models like GPT-2, use a simpler, more directive prompt
    let prompt: string;
    let systemPrompt: string;
    
    if (isLocalSmallModel) {
      // Very simple, directive prompt for small models (like GPT-2)
      // Avoid numbered lists or complex structures that GPT-2 will repeat
      systemPrompt = 'Answer questions about GraphQL queries using the documentation provided.';
      const cleanContext = documentationContext 
        ? documentationContext.replace(/^Relevant Documentation Context:\s*\n*/i, '').trim().substring(0, 400)
        : '';
      
      prompt = `Question: What does this GraphQL query do?\n\nQuery: ${query}\n\n`;
      if (cleanContext) {
        prompt += `Documentation: ${cleanContext}\n\n`;
      }
      prompt += `Answer: This query`;
    } else {
      // Use full prompt for better models
      const promptContext: ExplanationPromptContext = {
        query,
        documentationContext,
        schemaContext,
        liveStats,
      };
      systemPrompt = EXPLANATION_SYSTEM_PROMPT;
      prompt = detailed
        ? formatDetailedExplanationPrompt(query, documentationContext, schemaContext)
        : formatExplanationPrompt(promptContext);
    }

    // Step 5: Generate explanation using LLM
    // Optimize token usage for Gemini API limits
    // For Gemini, use shorter responses to save tokens
    const isGeminiModel = this.llmClient.getProvider() === 'gemini';
    const maxResponseTokens = isLocalSmallModel ? 200 : (isGeminiModel ? 800 : 1500); // Reduced for Gemini
    let explanation: string;
    
    try {
      const response = await this.llmClient.generateCompletion([
        { role: 'system', content: systemPrompt },
        { role: 'user', content: prompt },
      ], {
        temperature: isLocalSmallModel ? 0.3 : 0.7, // Lower temperature for small models
        maxTokens: maxResponseTokens,
      });

      explanation = response.content;
      
      // Detect if LLM output is repetitive or poor quality
      const isRepetitive = explanation.split('\n').filter(line => 
        line.includes('Question:') || line.includes('Query:') || line.includes('Answer:')
      ).length > 3;
      const isTooShort = explanation.trim().length < 50;
      const repeatsPrompt = explanation.includes('Question: What does this GraphQL query do?') && 
                           explanation.split('Question:').length > 2;
      
      // For small models like GPT-2, they often just repeat the prompt
      // Always use retrieved chunks or fallback for better explanation (GPT-2 is too basic)
      if (isLocalSmallModel || isRepetitive || isTooShort || repeatsPrompt) {
        if (docResult.chunks.length > 0) {
        // Always use retrieved chunks for small models - they're more reliable
        const topChunk = docResult.chunks[0];
        const chunkText = topChunk.chunkText.trim();
        
        // Extract the query name and fields
        const queryMatch = query.match(/\{?\s*(\w+)\s*\(/);
        const queryName = queryMatch ? queryMatch[1] : 'the query';
        const fieldsMatch = query.match(/\{\s*([^}]+)\s*\}/);
        const fields = fieldsMatch ? fieldsMatch[1].trim() : '';
        
        explanation = `This GraphQL query uses \`${queryName}\` to retrieve data from ResilientDB.\n\n`;
        
        if (chunkText) {
          // Clean up chunk text
          const cleanChunk = chunkText
            .replace(/^\[Source:[^\]]+\]\s*/gm, '')
            .replace(/^---\s*$/gm, '')
            .trim();
          
          explanation += `According to the documentation:\n\n${cleanChunk.substring(0, 600)}`;
          if (cleanChunk.length > 600) {
            explanation += '...';
          }
        }
        
        if (fields) {
          explanation += `\n\nThe query requests the following fields: ${fields}`;
        }
        
        if (docResult.chunks.length > 1) {
          const secondChunk = docResult.chunks[1].chunkText
            .replace(/^\[Source:[^\]]+\]\s*/gm, '')
            .replace(/^---\s*$/gm, '')
            .trim();
          explanation += `\n\nAdditional information: ${secondChunk.substring(0, 200)}...`;
        }
      } else {
        // No chunks available, provide structured explanation from query analysis
        const queryMatch = query.match(/\{?\s*(\w+)\s*\(/);
        const queryName = queryMatch ? queryMatch[1] : 'the query';
        // Extract fields from the innermost braces (the selection set)
        const fieldsMatch = query.match(/\{\s*([^}]+)\s*\}/);
        let fields: string[] = [];
        if (fieldsMatch) {
          // Extract just the field names, ignoring arguments and nested structures
          fields = fieldsMatch[1]
            .split(/\s+/)
            .map(f => f.trim())
            .filter(f => f && !f.match(/^[{}()[\]:,="]+$/) && !f.includes('(') && !f.includes(')'))
            .filter(f => f.length > 0 && f !== 'getTransaction' && f !== queryName);
        }
        
        explanation = `This GraphQL query uses \`${queryName}\` to retrieve data from ResilientDB.\n\n`;
        
        if (fields.length > 0) {
          explanation += `**Fields requested:**\n`;
          fields.forEach(field => {
            explanation += `- \`${field}\`: Retrieves the ${field} field from the transaction\n`;
          });
        } else {
          explanation += `**Fields requested:** All available fields for the transaction\n`;
        }
        
        explanation += `\n**What it does:**\nThis query fetches a single transaction by its ID and returns the specified fields. The \`getTransaction\` operation is a read operation that retrieves transaction data from the ResilientDB database.`;
      }
    }
    } catch (error) {
      // If LLM fails, use retrieved chunks or provide basic explanation
      console.warn('LLM generation failed, using fallback:', error instanceof Error ? error.message : String(error));
      if (docResult.chunks.length > 0) {
        const topChunk = docResult.chunks[0];
        explanation = `This GraphQL query retrieves transaction data.\n\nDocumentation: ${topChunk.chunkText.substring(0, 400)}...`;
      } else {
        // Extract query info for basic explanation
        const queryMatch = query.match(/\{?\s*(\w+)\s*\(/);
        const queryName = queryMatch ? queryMatch[1] : 'the query';
        const fieldsMatch = query.match(/\{\s*([^}]+)\s*\}/);
        const fields = fieldsMatch ? fieldsMatch[1].trim() : 'data';
        
        explanation = `This GraphQL query uses \`${queryName}\` to retrieve ${fields} from ResilientDB. The query structure suggests it's fetching transaction data by ID.`;
      }
    }

      return {
        explanation,
        query,
        contextChunksUsed: docResult.chunks.length + schemaResult.chunks.length,
        liveStatsIncluded: liveStats !== undefined,
      };
    } catch (error) {
      // Comprehensive error handling - provide fallback explanation
      console.error('Explanation service error:', error instanceof Error ? error.message : String(error));
      const queryMatch = query.match(/\{?\s*(\w+)\s*\(/);
      const queryName = queryMatch ? queryMatch[1] : 'the query';
      const fieldsMatch = query.match(/\{\s*([^}]+)\s*\}/);
      const fields = fieldsMatch ? fieldsMatch[1].trim() : 'data';
      
      return {
        explanation: `This GraphQL query uses \`${queryName}\` to retrieve ${fields} from ResilientDB. The query structure suggests it's fetching transaction data by ID.`,
        query,
        contextChunksUsed: 0,
        liveStatsIncluded: false,
      };
    }
  }

  /**
   * Generate quick explanation (concise version)
   */
  async explainQuick(query: string): Promise<string> {
    const result = await this.retrievalService.retrieve(query, {
      limit: 3,
      minSimilarity: 0.3,
    });

    const context = result.chunks.length > 0
      ? this.contextFormatter.format(result.chunks, undefined, {
          maxTokens: 500,
          format: 'compact',
        })
      : undefined;

    const prompt = `Briefly explain this GraphQL query (2-3 sentences):\n\n\`\`\`graphql\n${query}\n\`\`\`\n\n${context ? `Context: ${context}\n\n` : ''}Provide a concise explanation.`;

    const response = await this.llmClient.generateCompletion([
      { role: 'system', content: EXPLANATION_SYSTEM_PROMPT },
      { role: 'user', content: prompt },
    ], {
      temperature: 0.7,
      maxTokens: 200,
    });

    return response.content;
  }

  /**
   * Generate explanation with custom context
   * 
   * Useful when you already have context retrieved elsewhere
   */
  async explainWithContext(
    query: string,
    context: string,
    options: { includeLiveStats?: boolean } = {}
  ): Promise<string> {
    const { includeLiveStats = env.LLM_ENABLE_LIVE_STATS || false } = options;

    let liveStats: ExplanationPromptContext['liveStats'] | undefined;
    if (includeLiveStats) {
      try {
        const metrics = await this.resLensClient.getMetrics(query);

        if (metrics && metrics.query === query) {
          const resourceUsageStr = metrics.resourceUsage
            ? `CPU: ${metrics.resourceUsage.cpu || 'N/A'}%, Memory: ${metrics.resourceUsage.memory || 'N/A'}MB, Network: ${metrics.resourceUsage.network || 'N/A'}KB`
            : undefined;
          
          liveStats = {
            executionTime: metrics.executionTime,
            resourceUsage: resourceUsageStr,
            queryComplexity: metrics.resultSize ? `Result Size: ${metrics.resultSize} items` : undefined,
          };
        }
      } catch (error) {
        // Continue without live stats
      }
    }

    const promptContext: ExplanationPromptContext = {
      query,
      documentationContext: context,
      liveStats,
    };

    const prompt = formatExplanationPrompt(promptContext);

    const response = await this.llmClient.generateCompletion([
      { role: 'system', content: EXPLANATION_SYSTEM_PROMPT },
      { role: 'user', content: prompt },
    ], {
      temperature: 0.7,
      maxTokens: 1500,
    });

    return response.content;
  }

  /**
   * Check service availability
   */
  async checkAvailability(): Promise<{
    llm: boolean;
    retrieval: boolean;
    resLens: boolean;
  }> {
    const retrievalAvailability = await this.retrievalService.checkAvailability();
    const llmAvailable = this.llmClient.isAvailable();

    let resLensAvailable = false;
    try {
      // Simple check - try to get metrics
      await this.resLensClient.getMetrics('test');
      resLensAvailable = true;
    } catch {
      // ResLens not available (expected if not configured)
    }

    return {
      llm: llmAvailable,
      retrieval: retrievalAvailability.vectorStore && retrievalAvailability.embeddingService,
      resLens: resLensAvailable,
    };
  }
}

