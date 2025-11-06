/**
 * Prompt Templates for LLM
 * 
 * Contains reusable prompt templates for various LLM tasks:
 * - Query explanations
 * - Query optimizations
 * - Context formatting
 * - System prompts
 */

export interface ExplanationPromptContext {
  query: string;
  documentationContext: string;
  schemaContext?: string;
  liveStats?: {
    executionTime?: number;
    resourceUsage?: string;
    queryComplexity?: string;
    throughput?: number;
  };
}

export interface OptimizationPromptContext {
  query: string;
  documentationContext: string;
  schemaContext?: string;
  similarQueries?: Array<{
    query: string;
    executionTime: number;
    description: string;
  }>;
}

/**
 * System prompt for query explanations
 */
export const EXPLANATION_SYSTEM_PROMPT = `You are an expert GraphQL tutor for ResilientDB. Your role is to help developers understand GraphQL queries by providing clear, accurate, and helpful explanations.

Guidelines:
- Explain what the query does in simple terms
- Break down complex queries into understandable parts
- Reference relevant documentation when available
- Use examples when helpful
- Be concise but thorough
- If the query has issues, point them out constructively
- Reference schema information when explaining fields and types`;

/**
 * System prompt for query optimizations
 */
export const OPTIMIZATION_SYSTEM_PROMPT = `You are an expert GraphQL query optimizer for ResilientDB. Your role is to analyze queries and suggest improvements for better performance and efficiency.

Guidelines:
- Identify performance bottlenecks
- Suggest more efficient query patterns
- Reference documentation for best practices
- Compare with similar queries when available
- Provide actionable recommendations
- Explain why optimizations improve performance
- Consider query complexity and resource usage`;

/**
 * Format explanation prompt with context
 */
export function formatExplanationPrompt(context: ExplanationPromptContext): string {
  let prompt = `Explain the following GraphQL query for ResilientDB:\n\n`;
  prompt += `\`\`\`graphql\n${context.query}\n\`\`\`\n\n`;

  if (context.documentationContext) {
    prompt += `Relevant Documentation:\n${context.documentationContext}\n\n`;
  }

  if (context.schemaContext) {
    prompt += `Schema Information:\n${context.schemaContext}\n\n`;
  }

  if (context.liveStats) {
    prompt += `Query Performance Metrics:\n`;
    if (context.liveStats.executionTime) {
      prompt += `- Execution Time: ${context.liveStats.executionTime}ms\n`;
    }
    if (context.liveStats.resourceUsage) {
      prompt += `- Resource Usage: ${context.liveStats.resourceUsage}\n`;
    }
    if (context.liveStats.queryComplexity) {
      prompt += `- Query Complexity: ${context.liveStats.queryComplexity}\n`;
    }
    prompt += `\n`;
  }

  prompt += `Please provide a clear explanation of what this query does, how it works, and any important considerations.`;

  return prompt;
}

/**
 * Format optimization prompt with context
 */
export function formatOptimizationPrompt(context: OptimizationPromptContext): string {
  let prompt = `Analyze and optimize the following GraphQL query for ResilientDB:\n\n`;
  prompt += `\`\`\`graphql\n${context.query}\n\`\`\`\n\n`;

  if (context.documentationContext) {
    prompt += `Optimization References:\n${context.documentationContext}\n\n`;
  }

  if (context.schemaContext) {
    prompt += `Schema Information:\n${context.schemaContext}\n\n`;
  }

  if (context.similarQueries && context.similarQueries.length > 0) {
    prompt += `Similar Queries for Comparison:\n\n`;
    context.similarQueries.forEach((sq, index) => {
      prompt += `${index + 1}. Query: \`${sq.query.substring(0, 100)}${sq.query.length > 100 ? '...' : ''}\`\n`;
      prompt += `   Execution Time: ${sq.executionTime}ms\n`;
      prompt += `   Description: ${sq.description}\n\n`;
    });
  }

  prompt += `Please analyze this query and provide:\n`;
  prompt += `1. Performance issues identified\n`;
  prompt += `2. Specific optimization recommendations\n`;
  prompt += `3. Expected performance improvements\n`;
  prompt += `4. An optimized version of the query (if applicable)`;

  return prompt;
}

/**
 * Format context for LLM consumption
 */
export function formatContextForLLM(
  documentationContext: string,
  schemaContext?: string
): string {
  let formatted = `Documentation Context:\n\n${documentationContext}`;

  if (schemaContext) {
    formatted += `\n\n${'='.repeat(50)}\n\n`;
    formatted += `Schema Context:\n\n${schemaContext}`;
  }

  return formatted;
}

/**
 * Format live stats for LLM consumption
 */
export function formatLiveStatsForLLM(stats: {
  executionTime?: number;
  resourceUsage?: string;
  queryComplexity?: string;
  throughput?: number;
}): string {
  const parts: string[] = [];

  if (stats.executionTime !== undefined) {
    parts.push(`Execution Time: ${stats.executionTime}ms`);
  }

  if (stats.resourceUsage) {
    parts.push(`Resource Usage: ${stats.resourceUsage}`);
  }

  if (stats.queryComplexity) {
    parts.push(`Query Complexity: ${stats.queryComplexity}`);
  }

  if (stats.throughput !== undefined) {
    parts.push(`Throughput: ${stats.throughput} queries/sec`);
  }

  return parts.length > 0 ? parts.join(' | ') : 'No metrics available';
}

/**
 * Create a concise explanation prompt (for quick explanations)
 */
export function formatQuickExplanationPrompt(query: string, context?: string): string {
  let prompt = `Briefly explain this GraphQL query:\n\n\`\`\`graphql\n${query}\n\`\`\`\n\n`;
  
  if (context) {
    prompt += `Context: ${context}\n\n`;
  }

  prompt += `Provide a concise explanation (2-3 sentences).`;

  return prompt;
}

/**
 * Create a detailed explanation prompt (for comprehensive explanations)
 */
export function formatDetailedExplanationPrompt(
  query: string,
  documentationContext: string,
  schemaContext?: string
): string {
  let prompt = `Provide a detailed explanation of this GraphQL query:\n\n`;
  prompt += `\`\`\`graphql\n${query}\n\`\`\`\n\n`;

  prompt += `Documentation:\n${documentationContext}\n\n`;

  if (schemaContext) {
    prompt += `Schema:\n${schemaContext}\n\n`;
  }

  prompt += `Explain:\n`;
  prompt += `1. What the query does\n`;
  prompt += `2. How each field/operation works\n`;
  prompt += `3. Expected results\n`;
  prompt += `4. Common use cases\n`;
  prompt += `5. Potential issues or considerations`;

  return prompt;
}

