/**
 * Context Formatter for RAG
 * 
 * Formats retrieved document chunks into context strings suitable for LLM prompts.
 * Handles context window limits, chunk organization, and formatting for different use cases.
 */

import { DocumentChunk } from './resilientdb-vector-store';

export interface FormatOptions {
  /** Maximum number of tokens/characters in context */
  maxTokens?: number;
  /** Include chunk metadata in formatted output */
  includeMetadata?: boolean;
  /** Include similarity scores if available */
  includeScores?: boolean;
  /** Format style: 'compact', 'detailed', or 'markdown' */
  format?: 'compact' | 'detailed' | 'markdown';
  /** Separator between chunks */
  separator?: string;
}

/**
 * Context Formatter
 * 
 * Formats retrieved chunks into context strings for LLM consumption.
 */
export class ContextFormatter {
  /**
   * Format chunks into a single context string
   * 
   * @param chunks - Retrieved document chunks
   * @param scores - Optional similarity scores
   * @param options - Formatting options
   * @returns Formatted context string
   */
  format(
    chunks: DocumentChunk[],
    scores?: number[],
    options: FormatOptions = {}
  ): string {
    const {
      maxTokens = 4000,
      includeMetadata = true,
      includeScores = false,
      format = 'detailed',
      separator = '\n\n---\n\n',
    } = options;

    const formattedChunks: string[] = [];
    let currentLength = 0;

    for (let i = 0; i < chunks.length; i++) {
      const chunk = chunks[i];
      const score = scores?.[i];
      
      const formatted = this.formatChunk(chunk, score, {
        includeMetadata,
        includeScores,
        format,
      });

      // Estimate tokens (rough: 1 token ≈ 4 characters)
      const estimatedTokens = formatted.length / 4;

      if (currentLength + estimatedTokens > maxTokens) {
        break; // Stop if we'd exceed token limit
      }

      formattedChunks.push(formatted);
      currentLength += estimatedTokens;
    }

    return formattedChunks.join(separator);
  }

  /**
   * Format a single chunk
   */
  private formatChunk(
    chunk: DocumentChunk,
    score: number | undefined,
    options: {
      includeMetadata: boolean;
      includeScores: boolean;
      format: 'compact' | 'detailed' | 'markdown';
    }
  ): string {
    const { includeMetadata, includeScores, format } = options;

    let formatted = '';

    // Add metadata header if requested
    if (includeMetadata) {
      if (format === 'markdown') {
        formatted += `## ${this.getSourceName(chunk.source)}\n\n`;
        if (chunk.metadata.section) {
          formatted += `**Section:** ${chunk.metadata.section}\n\n`;
        }
        if (includeScores && score !== undefined) {
          formatted += `**Relevance:** ${(score * 100).toFixed(1)}%\n\n`;
        }
        formatted += chunk.chunkText;
      } else if (format === 'detailed') {
        formatted += `[Source: ${this.getSourceName(chunk.source)}`;
        if (chunk.metadata.section) {
          formatted += ` | Section: ${chunk.metadata.section}`;
        }
        if (includeScores && score !== undefined) {
          formatted += ` | Relevance: ${(score * 100).toFixed(1)}%`;
        }
        formatted += ']\n\n';
        formatted += chunk.chunkText;
      } else {
        // Compact format
        formatted = chunk.chunkText;
        if (includeScores && score !== undefined) {
          formatted = `[Relevance: ${(score * 100).toFixed(0)}%] ${formatted}`;
        }
      }
    } else {
      formatted = chunk.chunkText;
    }

    return formatted;
  }

  /**
   * Format context for query explanation prompts
   * 
   * Specialized formatting for query explanation use case.
   */
  formatForExplanation(
    chunks: DocumentChunk[],
    scores?: number[],
    options: FormatOptions = {}
  ): string {
    const context = this.format(chunks, scores, {
      ...options,
      format: 'detailed',
      includeMetadata: true,
      includeScores: false, // Don't show scores in explanations
    });

    return `Relevant Documentation Context:\n\n${context}`;
  }

  /**
   * Format context for query optimization prompts
   * 
   * Specialized formatting for optimization suggestions.
   */
  formatForOptimization(
    chunks: DocumentChunk[],
    scores?: number[],
    options: FormatOptions = {}
  ): string {
    // Prioritize schema and example chunks for optimization
    const schemaChunks = chunks.filter(c => c.metadata.type === 'schema');
    const exampleChunks = chunks.filter(c => c.metadata.type === 'example');
    const docChunks = chunks.filter(c => 
      c.metadata.type === 'documentation' || c.metadata.type === 'api'
    );

    const prioritizedChunks = [...schemaChunks, ...exampleChunks, ...docChunks];

    const context = this.format(prioritizedChunks.slice(0, options.maxTokens ? Math.floor(options.maxTokens / 2) : 10), 
      scores, {
      ...options,
      format: 'compact',
      includeMetadata: false,
    });

    return `Optimization References:\n\n${context}`;
  }

  /**
   * Format context combining documentation and schema
   * 
   * Creates a balanced context with both documentation and schema information.
   */
  formatCombined(
    docChunks: DocumentChunk[],
    schemaChunks: DocumentChunk[],
    options: FormatOptions = {}
  ): string {
    const {
      maxTokens = 4000,
      format = 'detailed',
    } = options;

    // Allocate 60% to documentation, 40% to schema
    const docTokens = Math.floor(maxTokens * 0.6);
    const schemaTokens = Math.floor(maxTokens * 0.4);

    const docContext = this.format(docChunks, undefined, {
      ...options,
      maxTokens: docTokens,
      format,
    });

    const schemaContext = this.format(schemaChunks, undefined, {
      ...options,
      maxTokens: schemaTokens,
      format,
    });

    return `Documentation:\n\n${docContext}\n\n${'='.repeat(50)}\n\nSchema Information:\n\n${schemaContext}`;
  }

  /**
   * Extract source name from full path
   */
  private getSourceName(source: string): string {
    const parts = source.split('/');
    return parts[parts.length - 1] || source;
  }

  /**
   * Estimate token count for a string
   * 
   * Rough estimation: 1 token ≈ 4 characters
   */
  estimateTokens(text: string): number {
    return Math.ceil(text.length / 4);
  }

  /**
   * Truncate context to fit token limit
   */
  truncateToTokens(text: string, maxTokens: number): string {
    const estimated = this.estimateTokens(text);
    if (estimated <= maxTokens) {
      return text;
    }

    // Rough truncation (4 chars per token)
    const maxChars = maxTokens * 4;
    return text.substring(0, maxChars) + '...';
  }
}

