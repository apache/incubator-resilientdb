/**
 * Query Optimization Service
 * 
 * Generates optimization suggestions for GraphQL queries using:
 * - Query structure analysis
 * - RAG retrieval for best practices
 * - LLM for intelligent suggestions
 * - ResLens metrics for comparison
 */

import { LLMClient } from './llm-client';
import { RetrievalService } from '../rag/retrieval-service';
import { ContextFormatter } from '../rag/context-formatter';
import { QueryAnalyzer, QueryAnalysis } from '../services/query-analyzer';
import { ResLensClient } from '../reslens/client';
import {
  formatOptimizationPrompt,
  OPTIMIZATION_SYSTEM_PROMPT,
  OptimizationPromptContext,
} from './prompts';
// import env from '../config/environment'; // Not currently used

export interface OptimizationSuggestion {
  /** Suggested improvement */
  suggestion: string;
  /** Reasoning for the suggestion */
  reason: string;
  /** Expected improvement (e.g., "30% faster execution") */
  expectedImprovement?: string;
  /** Optimized query version (if applicable) */
  optimizedQuery?: string;
  /** Priority level */
  priority: 'low' | 'medium' | 'high';
}

export interface OptimizationResult {
  query: string;
  suggestions: OptimizationSuggestion[];
  analysis: QueryAnalysis;
  similarQueries?: Array<{
    query: string;
    executionTime: number;
    description: string;
  }>;
}

export interface OptimizationOptions {
  /** Include similar queries from ResLens for comparison */
  includeSimilarQueries?: boolean;
  /** Maximum context chunks to retrieve */
  maxContextChunks?: number;
  /** Minimum similarity threshold for retrieval */
  minSimilarity?: number;
}

/**
 * Optimization Service
 * 
 * Generates optimization suggestions for GraphQL queries
 */
export class OptimizationService {
  private llmClient: LLMClient;
  private retrievalService: RetrievalService;
  private contextFormatter: ContextFormatter;
  private queryAnalyzer: QueryAnalyzer;
  private resLensClient: ResLensClient;

  constructor() {
    this.llmClient = new LLMClient();
    this.retrievalService = new RetrievalService();
    this.contextFormatter = new ContextFormatter();
    this.queryAnalyzer = new QueryAnalyzer();
    this.resLensClient = new ResLensClient();
  }

  /**
   * Generate optimization suggestions for a GraphQL query
   * 
   * @param query - GraphQL query to optimize
   * @param options - Optimization options
   * @returns Optimization result with suggestions
   */
  async optimize(
    query: string,
    options: OptimizationOptions = {}
  ): Promise<OptimizationResult> {
    const {
      includeSimilarQueries = true,
      maxContextChunks = 10,
      minSimilarity = 0.3,
    } = options;

    // Step 1: Analyze query structure
    const analysis = this.queryAnalyzer.analyze(query);

    // Step 2: Retrieve relevant optimization context
    const [docResult, schemaResult] = await Promise.all([
      this.retrievalService.retrieveDocumentationContext(
        `GraphQL query optimization best practices ${query}`,
        {
          limit: maxContextChunks,
          minSimilarity,
          includeScores: false,
        }
      ),
      this.retrievalService.retrieveSchemaContext(query, {
        limit: Math.floor(maxContextChunks / 2),
        minSimilarity: 0.25,
        includeScores: false,
      }),
    ]);

    // Step 3: Format context for optimization
    const documentationContext = this.contextFormatter.formatForOptimization(
      docResult.chunks,
      undefined,
      { maxTokens: 2000 }
    );

    const schemaContext = schemaResult.chunks.length > 0
      ? this.contextFormatter.format(schemaResult.chunks, undefined, {
          maxTokens: 1000,
          format: 'compact',
        })
      : undefined;

    // Step 4: Get similar queries from ResLens (if enabled)
    let similarQueries: Array<{
      query: string;
      executionTime: number;
      description: string;
    }> | undefined;

    if (includeSimilarQueries) {
      try {
        const recentQueries = await this.resLensClient.getRecentQueries(10);
        
        // Find similar queries
        similarQueries = recentQueries
          .map((q: { query: string; executionTime: number }) => ({
            query: q.query,
            executionTime: q.executionTime,
            description: `Similar query executed ${q.executionTime}ms`,
          }))
          .filter((sq: { query: string }, index: number, self: Array<{ query: string }>) => {
            // Filter out exact duplicates
            return index === self.findIndex((s: { query: string }) => s.query === sq.query);
          })
          .slice(0, 5); // Limit to top 5 similar queries
      } catch (error) {
        // ResLens not available, continue without similar queries
        console.warn('Could not fetch similar queries:', error instanceof Error ? error.message : String(error));
      }
    }

    // Step 5: Format optimization prompt
    const promptContext: OptimizationPromptContext = {
      query,
      documentationContext,
      schemaContext,
      similarQueries,
    };

    const prompt = formatOptimizationPrompt(promptContext);

    // Step 6: Generate optimization suggestions using LLM
    const response = await this.llmClient.generateCompletion([
      { role: 'system', content: OPTIMIZATION_SYSTEM_PROMPT },
      { role: 'user', content: prompt },
    ], {
      temperature: 0.5, // Lower temperature for more focused suggestions
      maxTokens: 2000,
    });

    // Step 7: Parse LLM response into structured suggestions
    const suggestions = this.parseOptimizationResponse(
      response.content,
      analysis
    );

    return {
      query,
      suggestions,
      analysis,
      similarQueries,
    };
  }

  /**
   * Parse LLM response into structured optimization suggestions
   */
  private parseOptimizationResponse(
    llmResponse: string,
    analysis: QueryAnalysis
  ): OptimizationSuggestion[] {
    const suggestions: OptimizationSuggestion[] = [];

    // First, add suggestions from query analysis
    for (const opportunity of analysis.opportunities) {
      suggestions.push({
        suggestion: opportunity.suggestion || opportunity.description,
        reason: opportunity.description,
        priority: opportunity.severity,
      });
    }

    // Parse LLM response (look for numbered lists or sections)
    const lines = llmResponse.split('\n');
    let currentSection = '';
    
    for (const line of lines) {
      const trimmed = line.trim();
      
      // Look for numbered items (1., 2., etc.)
      if (/^\d+\./.test(trimmed)) {
        const suggestion = trimmed.replace(/^\d+\.\s*/, '');
        if (suggestion.length > 20) { // Only include substantial suggestions
          suggestions.push({
            suggestion,
            reason: currentSection || 'Based on best practices',
            priority: this.inferPriority(suggestion),
          });
        }
      }
      
      // Look for section headers
      if (trimmed.startsWith('**') || /^[A-Z][^:]+:/.test(trimmed)) {
        currentSection = trimmed.replace(/[**]/g, '').replace(':', '');
      }
    }

    // If no structured suggestions found, treat entire response as one suggestion
    if (suggestions.length === analysis.opportunities.length && llmResponse.length > 100) {
      suggestions.push({
        suggestion: llmResponse.substring(0, 500),
        reason: 'LLM analysis',
        priority: 'medium',
      });
    }

    // Deduplicate and prioritize
    return this.deduplicateSuggestions(suggestions);
  }

  /**
   * Infer priority from suggestion text
   */
  private inferPriority(suggestion: string): 'low' | 'medium' | 'high' {
    const lower = suggestion.toLowerCase();
    
    if (lower.includes('critical') || lower.includes('urgent') || lower.includes('severe')) {
      return 'high';
    }
    if (lower.includes('important') || lower.includes('significant') || lower.includes('major')) {
      return 'medium';
    }
    return 'low';
  }

  /**
   * Deduplicate suggestions
   */
  private deduplicateSuggestions(
    suggestions: OptimizationSuggestion[]
  ): OptimizationSuggestion[] {
    const seen = new Set<string>();
    const unique: OptimizationSuggestion[] = [];

    for (const suggestion of suggestions) {
      const key = suggestion.suggestion.substring(0, 100).toLowerCase();
      if (!seen.has(key)) {
        seen.add(key);
        unique.push(suggestion);
      }
    }

    // Sort by priority
    const priorityOrder = { high: 3, medium: 2, low: 1 };
    unique.sort((a, b) => priorityOrder[b.priority] - priorityOrder[a.priority]);

    return unique;
  }

  /**
   * Quick optimization check (lightweight analysis only)
   */
  quickAnalyze(query: string): QueryAnalysis {
    return this.queryAnalyzer.analyze(query);
  }

  /**
   * Check service availability
   */
  async checkAvailability(): Promise<{
    llm: boolean;
    retrieval: boolean;
    analyzer: boolean;
    resLens: boolean;
  }> {
    const retrievalAvailability = await this.retrievalService.checkAvailability();
    const llmAvailable = this.llmClient.isAvailable();

    let resLensAvailable = false;
    try {
      await this.resLensClient.getMetrics('test');
      resLensAvailable = true;
    } catch {
      // ResLens not available
    }

    return {
      llm: llmAvailable,
      retrieval: retrievalAvailability.vectorStore && retrievalAvailability.embeddingService,
      analyzer: true, // Always available (local analysis)
      resLens: resLensAvailable,
    };
  }
}

