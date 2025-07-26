/**
 * Code-weighting reranker for academic chunks
 * Boosts chunks that contain code-related keywords for better code generation
 */

interface RetrievedNode {
  node: {
    getContent: (mode?: any) => string;
    metadata?: any;
  };
  score: number;
}

interface CodeRerankerOptions {
  codeKeywords: string[];
  boostFactor: number;
  maxTokens: number;
}

const DEFAULT_CODE_KEYWORDS = [
  "algorithm",
  "proof",
  "pseudocode",
  "figure",
  "implementation",
  "function",
  "method",
  "class",
  "struct",
  "procedure",
  "protocol",
  "schema",
  "data structure",
  "complexity",
  "time complexity",
  "space complexity",
  "big o",
  "optimization",
  "performance",
  "benchmark",
  "evaluation",
  "experiment",
  "simulation",
  "model",
  "framework",
  "architecture",
  "design pattern",
  "api",
  "interface",
  "specification",
  "formal verification",
  "correctness",
  "invariant",
  "precondition",
  "postcondition"
];

export class CodeReranker {
  private options: CodeRerankerOptions;

  constructor(options: Partial<CodeRerankerOptions> = {}) {
    this.options = {
      codeKeywords: options.codeKeywords || DEFAULT_CODE_KEYWORDS,
      boostFactor: options.boostFactor || 1.5,
      maxTokens: options.maxTokens || 3000,
    };
  }

  /**
   * Rerank nodes by boosting those with code-related content
   */
  rerank(nodes: RetrievedNode[]): RetrievedNode[] {
    const rankedNodes = nodes.map(node => {
      const content = node.node.getContent().toLowerCase();
      const codeScore = this.calculateCodeScore(content);
      
      return {
        ...node,
        // boost the original score by a factor based on code relevance and the boost factor.
        // formula: enhanced_score = base_score * (1 + code_relevance * boost_factor)
        score: node.score * (1 + codeScore * this.options.boostFactor),
        codeRelevance: codeScore
      };
    });

    // Sort by enhanced score
    rankedNodes.sort((a, b) => b.score - a.score);

    // Apply token budget limit
    return this.applyTokenBudget(rankedNodes);
  }

  /**
   * Calculate code relevance score based on keyword presence
   */
  private calculateCodeScore(content: string): number {
    const words = content.split(/\s+/);
    const totalWords = words.length;
    
    if (totalWords === 0) return 0;

    let codeKeywordCount = 0;
    const foundKeywords = new Set<string>();

    // Count unique code keywords
    for (const keyword of this.options.codeKeywords) {
      if (content.includes(keyword.toLowerCase())) {
        foundKeywords.add(keyword);
        // Count occurrences for frequency weighting
        const regex = new RegExp(keyword.toLowerCase().replace(/[.*+?^${}()|[\]\\]/g, '\\$&'), 'g');
        const matches = content.match(regex);
        codeKeywordCount += matches ? matches.length : 0;
      }
    }

    // Normalize by content length and add diversity bonus
    const densityScore = Math.min(codeKeywordCount / totalWords * 100, 1.0);
    const diversityBonus = Math.min(foundKeywords.size / 10, 0.5); // Bonus for keyword diversity
    
    return densityScore + diversityBonus;
  }

  /**
   * Apply token budget by truncating context to stay within limits
   */
  private applyTokenBudget(nodes: RetrievedNode[]): RetrievedNode[] {
    const maxTokens = this.options.maxTokens;
    let currentTokens = 0;
    const selectedNodes: RetrievedNode[] = [];

    for (const node of nodes) {
      const content = node.node.getContent();
      const estimatedTokens = this.estimateTokens(content);
      
      if (currentTokens + estimatedTokens <= maxTokens) {
        selectedNodes.push(node);
        currentTokens += estimatedTokens;
      } else {
        // Try to fit a truncated version
        const remainingTokens = maxTokens - currentTokens;
        if (remainingTokens > 100) { // Only if we have meaningful space left
          const truncatedContent = this.truncateToTokens(content, remainingTokens);
          if (truncatedContent.length > 0) {
            selectedNodes.push({
              ...node,
              node: {
                ...node.node,
                getContent: () => truncatedContent
              }
            });
          }
        }
        break;
      }
    }

    return selectedNodes;
  }

  /**
   * Rough token estimation (1 token ≈ 4 characters for English)
   */
  private estimateTokens(text: string): number {
    return Math.ceil(text.length / 4);
  }

  /**
   * Truncate text to approximate token limit
   */
  private truncateToTokens(text: string, maxTokens: number): string {
    const maxChars = maxTokens * 4;
    if (text.length <= maxChars) return text;
    
    // Try to truncate at sentence boundary
    const truncated = text.substring(0, maxChars);
    const lastSentence = truncated.lastIndexOf('.');
    
    if (lastSentence > maxChars * 0.7) {
      return truncated.substring(0, lastSentence + 1);
    }
    
    return truncated + "...";
  }

  /**
   * Get reranking statistics for debugging
   */
  getStats(originalNodes: RetrievedNode[], rerankedNodes: RetrievedNode[]): any {
    return {
      originalCount: originalNodes.length,
      rerankedCount: rerankedNodes.length,
      totalTokens: rerankedNodes.reduce((sum, node) => 
        sum + this.estimateTokens(node.node.getContent()), 0),
      averageCodeRelevance: rerankedNodes.reduce((sum, node: any) => 
        sum + (node.codeRelevance || 0), 0) / rerankedNodes.length,
      topCodeScores: rerankedNodes.slice(0, 5).map((node: any) => ({
        score: node.score,
        codeRelevance: node.codeRelevance || 0
      }))
    };
  }
} 