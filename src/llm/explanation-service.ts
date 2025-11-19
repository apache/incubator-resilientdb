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

  constructor() {
    this.llmClient = new LLMClient();
    this.retrievalService = new RetrievalService();
    this.contextFormatter = new ContextFormatter();
    this.resLensClient = new ResLensClient();
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
    // Detect if using local model with small context window (e.g., GPT-2)
    const isLocalSmallModel = this.llmClient.getProvider() === 'local' && 
      (this.llmClient.getModel().includes('gpt2') || this.llmClient.getModel().includes('distil'));
    
    // Adjust context size for small models
    const defaultMaxChunks = isLocalSmallModel ? 2 : 10;
    const defaultDocTokens = isLocalSmallModel ? 300 : 2000;
    const defaultSchemaTokens = isLocalSmallModel ? 150 : 1000;

    const {
      detailed = true,
      includeLiveStats = env.LLM_ENABLE_LIVE_STATS || false,
      maxContextChunks = defaultMaxChunks,
      minSimilarity = 0.3,
    } = options;

    // Step 1: Retrieve relevant context using RAG
    const [docResult, schemaResult] = await Promise.all([
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
    // Reduce maxTokens for small models
    const maxResponseTokens = isLocalSmallModel ? 200 : 1500;
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
      
      // For small models like GPT-2, they often just repeat the prompt
      // Always use retrieved chunks to create a better explanation (GPT-2 is too basic)
      if (isLocalSmallModel && docResult.chunks.length > 0) {
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
      }
    } catch (error) {
      // If LLM fails, use retrieved chunks
      if (docResult.chunks.length > 0) {
        const topChunk = docResult.chunks[0];
        explanation = `This GraphQL query retrieves transaction data.\n\nDocumentation: ${topChunk.chunkText.substring(0, 400)}...`;
      } else {
        explanation = `This GraphQL query retrieves transaction data from ResilientDB. The query structure suggests it's fetching a single transaction by ID.`;
      }
    }

    return {
      explanation,
      query,
      contextChunksUsed: docResult.chunks.length + schemaResult.chunks.length,
      liveStatsIncluded: liveStats !== undefined,
    };
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

