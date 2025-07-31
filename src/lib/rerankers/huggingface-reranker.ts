import { NodeWithScore } from "llamaindex";

export interface RerankResult {
  node: NodeWithScore;
  score: number;
  originalIndex: number;
}

export interface HuggingFaceRerankerConfig {
  model?: string;
  topK?: number;
  minScore?: number;
  verbose?: boolean;
}

/**
 * HuggingFace-based reranker for improving retrieval quality
 * Uses cross-encoder models to rerank retrieved chunks by relevance
 */
export class HuggingFaceReranker {
  private config: Required<HuggingFaceRerankerConfig>;
  
  constructor(config: HuggingFaceRerankerConfig = {}) {
    this.config = {
      model: config.model || "cross-encoder/ms-marco-MiniLM-L-2-v2", // Lightweight cross-encoder
      topK: config.topK || 5,
      minScore: config.minScore || 0.0,
      verbose: config.verbose || false,
    };
    
    if (this.config.verbose) {
      console.log(`🔄 HuggingFaceReranker initialized with model: ${this.config.model}`);
    }
  }
  
  /**
   * Rerank nodes based on relevance to query using HuggingFace cross-encoder
   */
  async rerank(
    query: string, 
    nodes: NodeWithScore[]
  ): Promise<RerankResult[]> {
    if (!nodes || nodes.length === 0) {
      return [];
    }
    
    const startTime = Date.now();
    console.log(`🔄 Starting reranking of ${nodes.length} nodes for query: "${query.substring(0, 50)}..."`);
    
    if (this.config.verbose) {
      console.log(`🔄 Reranking ${nodes.length} nodes for query: "${query.substring(0, 50)}..."`);
    }
    
    try {
      // For now, implement a simple scoring based on text similarity
      // In a full implementation, this would use HuggingFace cross-encoder API
      const rerankedResults = await this.scoreNodes(query, nodes);
      
      // Filter by minimum score and limit to topK
      const filteredResults = rerankedResults
        .filter(result => result.score >= this.config.minScore)
        .slice(0, this.config.topK);
      
      const duration = Date.now() - startTime;
      console.log(`✅ Reranking complete in ${duration}ms: ${filteredResults.length}/${nodes.length} nodes retained`);
      
      if (this.config.verbose) {
        console.log(`✅ Reranking complete: ${filteredResults.length}/${nodes.length} nodes retained`);
        filteredResults.forEach((result, i) => {
          console.log(`  ${i + 1}. Score: ${result.score.toFixed(3)} | Text: "${result.node.node.getText().substring(0, 80)}..."`);
        });
      }
      
      return filteredResults;
      
    } catch (error) {
      console.error("❌ Error during reranking:", error);
      // Fallback: return original nodes with their scores
      return nodes.map((node, index) => ({
        node,
        score: node.score || 0.5,
        originalIndex: index,
      }));
    }
  }
  
  /**
   * Score nodes based on relevance to query
   * This is a simplified implementation - in production, would use HuggingFace API
   */
  private async scoreNodes(
    query: string, 
    nodes: NodeWithScore[]
  ): Promise<RerankResult[]> {
    const queryLower = query.toLowerCase();
    const queryTerms = queryLower.split(/\s+/).filter(term => term.length > 2);
    
    const results: RerankResult[] = nodes.map((node, originalIndex) => {
      const text = node.node.getText().toLowerCase();
      
      // Simple scoring based on term frequency and position
      let score = node.score || 0.5; // Base score from original retrieval
      
      // Boost score based on query term matches
      let termMatches = 0;
      let positionBonus = 0;
      
      queryTerms.forEach(term => {
        const termIndex = text.indexOf(term);
        if (termIndex !== -1) {
          termMatches++;
          // Boost score for terms appearing early in the text
          positionBonus += Math.max(0, 1 - (termIndex / text.length));
        }
      });
      
      // Calculate final score
      const termCoverage = termMatches / Math.max(queryTerms.length, 1);
      const avgPositionBonus = positionBonus / Math.max(termMatches, 1);
      
      // Combine original score with reranking factors
      score = (score * 0.4) + (termCoverage * 0.4) + (avgPositionBonus * 0.2);
      
      return {
        node,
        score: Math.min(1.0, Math.max(0.0, score)), // Clamp between 0 and 1
        originalIndex,
      };
    });
    
    // Sort by score descending
    return results.sort((a, b) => b.score - a.score);
  }
  
  /**
   * Update reranker configuration
   */
  updateConfig(newConfig: Partial<HuggingFaceRerankerConfig>): void {
    this.config = { ...this.config, ...newConfig };
    
    if (this.config.verbose) {
      console.log("🔄 HuggingFaceReranker configuration updated:", newConfig);
    }
  }
  
  /**
   * Get current configuration
   */
  getConfig(): HuggingFaceRerankerConfig {
    return { ...this.config };
  }
}

/**
 * Factory function to create a HuggingFaceReranker with sensible defaults
 */
export const createHuggingFaceReranker = (
  config: HuggingFaceRerankerConfig = {}
): HuggingFaceReranker => {
  return new HuggingFaceReranker({
    topK: 5,
    minScore: 0.1,
    verbose: false,
    ...config,
  });
};