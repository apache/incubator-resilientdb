import { GraphQLQuery, QueryAnalysis, QuerySuggestion } from '../types';
import env from '../config/environment';

/**
 * Nexus LLM Integration
 * 
 * Integrates with Nexus HTTP API to leverage its LLM capabilities for query analysis.
 * 
 * Nexus API Endpoint: POST /api/research/chat
 * - Uses DeepSeek LLM for reasoning
 * - Has document retrieval capabilities
 * - Supports session-based memory
 * 
 * Reference: https://github.com/ResilientApp/nexus
 */
export class NexusIntegration {
  private apiUrl: string;
  private enabled: boolean;

  constructor() {
    this.apiUrl = env.NEXUS_API_URL;
    // Enable if API URL is configured
    this.enabled = !!this.apiUrl && this.apiUrl !== '';
  }

  /**
   * Analyze a GraphQL query using Nexus LLM
   * 
   * Calls Nexus API to get LLM-powered analysis of the query.
   * Falls back to stub response if Nexus is not available.
   */
  async analyzeQuery(
    query: GraphQLQuery,
    context?: {
      schema?: string;
      metrics?: unknown;
    }
  ): Promise<QueryAnalysis> {
    if (!this.enabled) {
      return this.getStubAnalysis(query);
    }

    try {
      // Build prompt for Nexus to analyze the GraphQL query
      const prompt = this.buildAnalysisPrompt(query, context);
      
      // Call Nexus API: POST /api/research/chat
      // Nexus requires: query (required), documentPaths (required), sessionId (optional)
      // Nexus streams responses, so we need a longer timeout
      const controller = new AbortController();
      const timeoutId = setTimeout(() => controller.abort(), 60000); // 60 second timeout
      
      const response = await fetch(`${this.apiUrl}/api/research/chat`, {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
          ...(env.NEXUS_API_KEY ? { Authorization: `Bearer ${env.NEXUS_API_KEY}` } : {}),
        },
        body: JSON.stringify({
          query: prompt,
          documentPaths: ['documents/resilientdb.pdf'], // Default document for GraphQL queries
          sessionId: `graphq-llm-${Date.now()}`, // Generate unique session ID
        }),
        signal: controller.signal,
      });
      
      clearTimeout(timeoutId);

      if (!response.ok) {
        const errorText = await response.text().catch(() => 'Unknown error');
        throw new Error(`Nexus API error: ${response.status} ${response.statusText}. ${errorText.substring(0, 200)}`);
      }

      // Handle both streaming and non-streaming responses
      const contentType = response.headers.get('content-type') || '';
      let data: unknown;
      
      if (contentType.includes('application/json')) {
        // Non-streaming JSON response
        data = await response.json();
      } else if (contentType.includes('text/event-stream') || contentType.includes('text/plain')) {
        // Streaming response - read all chunks
        const reader = response.body?.getReader();
        const decoder = new TextDecoder();
        let fullText = '';
        
        if (reader) {
          while (true) {
            const { done, value } = await reader.read();
            if (done) break;
            fullText += decoder.decode(value, { stream: true });
          }
        }
        
        // Try to parse as JSON, otherwise use as text
        try {
          data = JSON.parse(fullText);
        } catch {
          data = { response: fullText, text: fullText };
        }
      } else {
        // Fallback: try to parse as JSON
        const text = await response.text();
        try {
          data = JSON.parse(text);
        } catch {
          data = { response: text, text: text };
        }
      }
      
      // Parse Nexus response and convert to QueryAnalysis format
      // Type assertion: Nexus returns { response?: string, text?: string } or similar
      const nexusResponse = data as { response?: string; text?: string; [key: string]: unknown };
      return this.parseNexusResponse(query, nexusResponse, context);
    } catch (error) {
      console.warn('Nexus integration failed, using fallback:', error);
      // Fallback to stub response if Nexus is unavailable
      return this.getStubAnalysis(query);
    }
  }

  /**
   * Build prompt for Nexus to analyze GraphQL query
   */
  private buildAnalysisPrompt(
    query: GraphQLQuery,
    context?: { schema?: string; metrics?: unknown }
  ): string {
    let prompt = `Analyze this GraphQL query for ResilientDB:\n\n`;
    prompt += `Query:\n\`\`\`graphql\n${query.query}\n\`\`\`\n\n`;
    
    if (query.operationName) {
      prompt += `Operation Name: ${query.operationName}\n\n`;
    }
    
    if (context?.schema) {
      prompt += `Schema Context:\n${context.schema.substring(0, 1000)}...\n\n`;
    }
    
    prompt += `Please provide:\n`;
    prompt += `1. An explanation of what this query does\n`;
    prompt += `2. The complexity level (low/medium/high)\n`;
    prompt += `3. Recommendations for optimization\n`;
    prompt += `4. Estimated efficiency (0-100)\n`;
    
    return prompt;
  }

  /**
   * Parse Nexus response and convert to QueryAnalysis format
   */
  private parseNexusResponse(
    query: GraphQLQuery,
    nexusResponse: { response?: string; text?: string },
    _context?: { schema?: string; metrics?: unknown }
  ): QueryAnalysis {
    const responseText = nexusResponse.response || nexusResponse.text || '';
    
    // Extract information from Nexus response
    // Nexus returns natural language text, so we parse it
    const explanation = this.extractExplanation(responseText);
    const complexity = this.extractComplexity(responseText);
    const recommendations = this.extractRecommendations(responseText);
    const efficiency = this.extractEfficiency(responseText);

    return {
      query: query.query,
      explanation: explanation || 'Query analysis from Nexus LLM',
      complexity: complexity || 'medium',
      recommendations: recommendations.length > 0 ? recommendations : [
        'Review query structure',
        'Check field selection',
        'Consider adding filters if needed',
      ],
      estimatedEfficiency: efficiency || 50,
    };
  }

  /**
   * Extract explanation from Nexus response
   */
  private extractExplanation(text: string): string {
    // Look for explanation section
    const explanationMatch = text.match(/explanation[:\s]+(.*?)(?:\n|$)/i);
    if (explanationMatch) {
      return explanationMatch[1].trim();
    }
    // If no structured format, return first paragraph
    const paragraphs = text.split('\n\n');
    return paragraphs[0] || text.substring(0, 200);
  }

  /**
   * Extract complexity from Nexus response
   */
  private extractComplexity(text: string): 'low' | 'medium' | 'high' {
    const lowerText = text.toLowerCase();
    if (lowerText.includes('low complexity') || lowerText.includes('simple')) {
      return 'low';
    }
    if (lowerText.includes('high complexity') || lowerText.includes('complex')) {
      return 'high';
    }
    return 'medium';
  }

  /**
   * Extract recommendations from Nexus response
   */
  private extractRecommendations(text: string): string[] {
    const recommendations: string[] = [];
    
    // Look for numbered or bulleted recommendations
    const lines = text.split('\n');
    for (const line of lines) {
      const trimmed = line.trim();
      // Match numbered items (1., 2., etc.) or bullets (-, *, etc.)
      if (/^[\d\-*•]\s+/.test(trimmed) || trimmed.startsWith('Recommendation')) {
        const rec = trimmed.replace(/^[\d\-*•]\s+/, '').replace(/^Recommendation[:\s]+/i, '');
        if (rec.length > 10) {
          recommendations.push(rec);
        }
      }
    }
    
    return recommendations;
  }

  /**
   * Extract efficiency score from Nexus response
   */
  private extractEfficiency(text: string): number {
    // Look for efficiency score (0-100)
    const efficiencyMatch = text.match(/efficiency[:\s]+(\d+)/i);
    if (efficiencyMatch) {
      const score = parseInt(efficiencyMatch[1], 10);
      if (score >= 0 && score <= 100) {
        return score;
      }
    }
    // Default based on complexity
    const complexity = this.extractComplexity(text);
    return complexity === 'low' ? 80 : complexity === 'high' ? 40 : 60;
  }

  /**
   * Get stub analysis when Nexus is not available
   */
  private getStubAnalysis(query: GraphQLQuery): QueryAnalysis {
    return {
      query: query.query,
      explanation: 'Query analysis will be available when Nexus is configured and running.',
      complexity: 'medium',
      recommendations: [
        'Configure NEXUS_API_URL to enable Nexus integration',
        'Ensure Nexus is running on the configured URL',
        'Check query syntax manually for now',
      ],
      estimatedEfficiency: 50,
    };
  }

  /**
   * Get query suggestions from Nexus LLM
   * 
   * Calls Nexus API to get optimized query suggestions.
   * Falls back to stub response if Nexus is not available.
   */
  async getSuggestions(
    query: GraphQLQuery,
    context?: {
      schema?: string;
      metrics?: unknown;
    }
  ): Promise<QuerySuggestion[]> {
    if (!this.enabled) {
      return this.getStubSuggestions(query);
    }

    try {
      // Build prompt for Nexus to suggest query optimizations
      const prompt = this.buildSuggestionPrompt(query, context);
      
      // Call Nexus API: POST /api/research/chat
      const response = await fetch(`${this.apiUrl}/api/research/chat`, {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
          'Accept': 'application/json',
          ...(env.NEXUS_API_KEY ? { Authorization: `Bearer ${env.NEXUS_API_KEY}` } : {}),
        },
        body: JSON.stringify({
          query: prompt,
          documentPaths: ['documents/resilientdb.pdf'], // Default document for GraphQL queries
          sessionId: `graphq-llm-suggestions-${Date.now()}`,
        }),
        signal: (() => {
          const controller = new AbortController();
          setTimeout(() => controller.abort(), 60000); // 60 second timeout
          return controller.signal;
        })(),
      });

      if (!response.ok) {
        const errorText = await response.text().catch(() => 'Unknown error');
        throw new Error(`Nexus API error: ${response.status} ${response.statusText}. ${errorText.substring(0, 200)}`);
      }

      // Handle both streaming and non-streaming responses
      const contentType = response.headers.get('content-type') || '';
      let data: unknown;
      
      if (contentType.includes('application/json')) {
        data = await response.json();
      } else if (contentType.includes('text/event-stream') || contentType.includes('text/plain')) {
        // Streaming response - read all chunks
        const reader = response.body?.getReader();
        const decoder = new TextDecoder();
        let fullText = '';
        
        if (reader) {
          while (true) {
            const { done, value } = await reader.read();
            if (done) break;
            fullText += decoder.decode(value, { stream: true });
          }
        }
        
        try {
          data = JSON.parse(fullText);
        } catch {
          data = { response: fullText, text: fullText };
        }
      } else {
        const text = await response.text();
        try {
          data = JSON.parse(text);
        } catch {
          data = { response: text, text: text };
        }
      }
      
      // Parse Nexus response and convert to QuerySuggestion format
      const nexusResponse = data as { response?: string; text?: string; [key: string]: unknown };
      return this.parseSuggestionResponse(query, nexusResponse);
    } catch (error) {
      console.warn('Nexus integration failed, using fallback:', error);
      return this.getStubSuggestions(query);
    }
  }

  /**
   * Build prompt for Nexus to suggest query optimizations
   */
  private buildSuggestionPrompt(
    query: GraphQLQuery,
    context?: { schema?: string; metrics?: unknown }
  ): string {
    let prompt = `Suggest optimizations for this ResilientDB GraphQL query:\n\n`;
    prompt += `Current Query:\n\`\`\`graphql\n${query.query}\n\`\`\`\n\n`;
    
    if (context?.schema) {
      prompt += `Schema Context:\n${context.schema.substring(0, 1000)}...\n\n`;
    }
    
    prompt += `Please provide:\n`;
    prompt += `1. An optimized version of this query\n`;
    prompt += `2. Explanation of the optimizations\n`;
    prompt += `3. Confidence level (0-1) for the suggestions\n`;
    
    return prompt;
  }

  /**
   * Parse Nexus response and convert to QuerySuggestion format
   */
  private parseSuggestionResponse(
    originalQuery: GraphQLQuery,
    nexusResponse: { response?: string; text?: string }
  ): QuerySuggestion[] {
    const responseText = nexusResponse.response || nexusResponse.text || '';
    
    // Extract optimized query from response
    const optimizedQueryMatch = responseText.match(/```graphql\n([\s\S]*?)\n```/);
    const optimizedQuery = optimizedQueryMatch 
      ? optimizedQueryMatch[1].trim() 
      : originalQuery.query; // Fallback to original if no optimized version found
    
    // Extract explanation
    const explanation = this.extractExplanation(responseText);
    
    // Extract confidence (look for confidence score)
    const confidenceMatch = responseText.match(/confidence[:\s]+([\d.]+)/i);
    const confidence = confidenceMatch 
      ? parseFloat(confidenceMatch[1]) 
      : 0.7; // Default confidence

    return [
      {
        query: optimizedQuery,
        explanation: explanation || 'Optimized query suggestion from Nexus',
        confidence: Math.min(Math.max(confidence, 0), 1), // Clamp between 0 and 1
      },
    ];
  }

  /**
   * Get stub suggestions when Nexus is not available
   */
  private getStubSuggestions(query: GraphQLQuery): QuerySuggestion[] {
    return [
      {
        query: query.query,
        explanation: 'Query suggestions will be available when Nexus is configured.',
        confidence: 0.5,
      },
    ];
  }

  // Note: Prompt building and parsing methods removed as they're not needed
  // until actual Nexus integration is implemented. They can be added back
  // when the integration method is determined.

  /**
   * Check if Nexus integration is enabled
   */
  isEnabled(): boolean {
    return this.enabled;
  }

  /**
   * Get Nexus API URL
   */
  getApiUrl(): string {
    return this.apiUrl;
  }

  /**
   * Check if Nexus API is accessible
   */
  async checkConnection(): Promise<{
    connected: boolean;
    apiUrl: string;
    error?: string;
  }> {
    if (!this.enabled) {
      return {
        connected: false,
        apiUrl: this.apiUrl,
        error: 'Nexus API URL not configured',
      };
    }

    try {
      // Try to call Nexus health endpoint or documents endpoint
      const response = await fetch(`${this.apiUrl}/api/research/documents`, {
        method: 'GET',
        headers: {
          ...(env.NEXUS_API_KEY ? { Authorization: `Bearer ${env.NEXUS_API_KEY}` } : {}),
        },
      });

      if (response.ok) {
        return {
          connected: true,
          apiUrl: this.apiUrl,
        };
      } else {
        return {
          connected: false,
          apiUrl: this.apiUrl,
          error: `HTTP ${response.status}: ${response.statusText}`,
        };
      }
    } catch (error) {
      return {
        connected: false,
        apiUrl: this.apiUrl,
        error: error instanceof Error ? error.message : String(error),
      };
    }
  }
}

