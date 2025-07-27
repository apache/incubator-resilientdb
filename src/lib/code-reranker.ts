/*
 * inspired by https://www.staff.city.ac.uk/~sbrp622/papers/foundations_bm25_review.pdf
 * treats implementation/theoretical/quality signals as independent evidence sources,
 * combined additively like BM25 term weights. includes saturation functions (Math.min caps)
 * and density normalization to handle document length bias, plus non-textual code structure
 * detection as query-independent relevance signals.
 */


import { DEFAULT_IMPLEMENTATION_KEYWORDS, DEFAULT_QUALITY_INDICATORS, DEFAULT_THEORETICAL_KEYWORDS } from "./constants";
import { TikTokenEstimator, TokenEstimator } from "./token-estimator";

interface RetrievedNode {
  node: {
    getContent: (mode?: any) => string;
    metadata?: any;
  };
  score: number;
}

interface CodeRerankerOptions {
  implementationKeywords: string[];
  theoreticalKeywords: string[];
  qualityIndicators: string[];
  implementationWeight: number;
  theoreticalWeight: number;
  qualityWeight: number;
  codeBlockBonus: number;
  maxTokens: number;
}

export class CodeReranker {
  private options: CodeRerankerOptions;
  private maxTokens: number;
  private tokenEstimator: TokenEstimator;

  constructor(options: Partial<CodeRerankerOptions> = {}) {
    this.maxTokens = options.maxTokens || 4000; 
    this.tokenEstimator = new TikTokenEstimator();

    this.options = {
      implementationKeywords: options.implementationKeywords || DEFAULT_IMPLEMENTATION_KEYWORDS,
      theoreticalKeywords: options.theoreticalKeywords || DEFAULT_THEORETICAL_KEYWORDS,
      qualityIndicators: options.qualityIndicators || DEFAULT_QUALITY_INDICATORS,
      implementationWeight: options.implementationWeight || 1.0,
      theoreticalWeight: options.theoreticalWeight || 1.0,
      qualityWeight: options.qualityWeight || 1.0,
      codeBlockBonus: options.codeBlockBonus || 1.0,
      maxTokens: this.maxTokens,
    };
  }

 
  rerank(nodes: RetrievedNode[]): RetrievedNode[] {
    const rankedNodes = nodes.map(node => {
      const content = node.node.getContent().toLowerCase();
      const enhancedScore = this.calculateEnhancedScore(content, node.score);
      
      return {
        ...node,
        score: enhancedScore.finalScore,
        implementationScore: enhancedScore.implementationScore,
        theoreticalScore: enhancedScore.theoreticalScore,
        qualityScore: enhancedScore.qualityScore,
        codeBlockScore: enhancedScore.codeBlockScore,
        originalScore: node.score
      };
    });

    rankedNodes.sort((a, b) => b.score - a.score);

    return this.applyTokenBudgetWithDiversity(rankedNodes);
  }


  private calculateEnhancedScore(content: string, originalScore: number): {
    finalScore: number;
    implementationScore: number;
    theoreticalScore: number;
    qualityScore: number;
    codeBlockScore: number;
  } {
    const words = content.split(/\s+/);
    const totalWords = words.length;
    
    if (totalWords === 0) {
      return {
        finalScore: originalScore,
        implementationScore: 0,
        theoreticalScore: 0,
        qualityScore: 0,
        codeBlockScore: 0
      };
    }

    // calculate individual scores
    const implementationScore = this.calculateKeywordScore(content, this.options.implementationKeywords, totalWords);
    const theoreticalScore = this.calculateKeywordScore(content, this.options.theoreticalKeywords, totalWords);
    const qualityScore = this.calculateKeywordScore(content, this.options.qualityIndicators, totalWords);
    const codeBlockScore = this.detectCodeBlocks(content);

    // calculate weighted enhancement factor (additive approach)
    const enhancementFactor = 
      (implementationScore * this.options.implementationWeight) +
      (theoreticalScore * this.options.theoreticalWeight) +
      (qualityScore * this.options.qualityWeight) +
      (codeBlockScore * this.options.codeBlockBonus);

    // apply enhancement as additive boost to avoid over-amplification
    const finalScore = originalScore + (enhancementFactor * 0.1); 
    return {
      finalScore: Math.max(finalScore, originalScore * 0.1),
      implementationScore,
      theoreticalScore,
      qualityScore,
      codeBlockScore
    };
  }

  /**
   * Calculate keyword-based score with frequency and diversity consideration
   */
  private calculateKeywordScore(content: string, keywords: string[], totalWords: number): number {
    let keywordCount = 0;
    const foundKeywords = new Set<string>();

    for (const keyword of keywords) {
      const regex = new RegExp(keyword.toLowerCase().replace(/[.*+?^${}()|[\]\\]/g, '\\$&'), 'g');
      const matches = content.match(regex);
      if (matches) {
        foundKeywords.add(keyword);
        keywordCount += matches.length;
      }
    }

    // normalize by content length and add diversity bonus
    const densityScore = Math.min(keywordCount / totalWords * 100, 1.0);
    const diversityBonus = Math.min(foundKeywords.size / keywords.length, 0.3);
    
    return densityScore + diversityBonus;
  }


  private detectCodeBlocks(content: string): number {
    let codeBlockScore = 0;
    
    // indented code blocks (4+ spaces or tab)
    const indentedLines = content.split('\n').filter(line => 
      /^[\s]{4,}/.test(line) || /^\t/.test(line)
    );
    codeBlockScore += Math.min(indentedLines.length / 10, 0.5);

    // code fence markers
    const codeFences = content.match(/```[\s\S]*?```/g);
    if (codeFences) {
      codeBlockScore += Math.min(codeFences.length * 0.3, 0.8);
    }

    // function signatures and implementations
    const functionPatterns = [
      /function\s+\w+\s*\(/g,
      /event\s+\w+\s*\(/g,
      /def\s+\w+\s*\(/g,
      /class\s+\w+/g,
      /\w+\s*=\s*function/g,
      /const\s+\w+\s*=\s*\(/g,
      /\w+\s*\(\s*\w*\s*\)\s*{/g
    ];

    functionPatterns.forEach(pattern => {
      const matches = content.match(pattern);
      if (matches) {
        codeBlockScore += Math.min(matches.length * 0.2, 0.4);
      }
    });

    // variable declarations and assignments
    const variablePatterns = [
      /\w+\s*=\s*[^=]/g,
      /var\s+\w+/g,
      /let\s+\w+/g,
      /const\s+\w+/g
    ];

    variablePatterns.forEach(pattern => {
      const matches = content.match(pattern);
      if (matches) {
        codeBlockScore += Math.min(matches.length * 0.1, 0.3);
      }
    });

    return Math.min(codeBlockScore, 2.0); // cap the code block score
  }


  private applyTokenBudgetWithDiversity(nodes: RetrievedNode[]): RetrievedNode[] {
    const maxTokens = this.maxTokens;
    let currentTokens = 0;
    const selectedNodes: RetrievedNode[] = [];
    
    // ensure we get a good mix of implementation and theoretical content
    const implementationNodes = nodes.filter((node: any) => 
      node.implementationScore > node.theoreticalScore
    );
    const theoreticalNodes = nodes.filter((node: any) => 
      node.theoreticalScore >= node.implementationScore
    );

    // prioritize implementation nodes
    const prioritizedNodes = [
      ...implementationNodes.slice(0, Math.ceil(nodes.length * 0.7)),
      ...theoreticalNodes.slice(0, Math.ceil(nodes.length * 0.3))
    ].sort((a, b) => b.score - a.score);

    for (const node of prioritizedNodes) {
      const content = node.node.getContent();
      const estimatedTokens = this.tokenEstimator.estimate(content);
      
      if (currentTokens + estimatedTokens <= maxTokens) {
        selectedNodes.push(node);
        currentTokens += estimatedTokens;
      } else {
        // try to fit a truncated version
        const remainingTokens = maxTokens - currentTokens;
        if (remainingTokens > 100) {
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



  private truncateToTokens(text: string, maxTokens: number): string {
    const maxChars = maxTokens * 4;
    if (text.length <= maxChars) return text;
    
    // try to truncate at sentence boundary
    const truncated = text.substring(0, maxChars);
    const lastSentence = truncated.lastIndexOf('.');
    
    if (lastSentence > maxChars * 0.7) {
      return truncated.substring(0, lastSentence + 1);
    }
    
    return truncated + "...";
  }

 
  getStats(originalNodes: RetrievedNode[], rerankedNodes: RetrievedNode[]): any {
    const enhancedNodes = rerankedNodes as any[];
    
    return {
      originalCount: originalNodes.length,
      rerankedCount: rerankedNodes.length,
      totalTokens: rerankedNodes.reduce((sum, node) => 
        sum + this.tokenEstimator.estimate(node.node.getContent()), 0),
      
      // scoring statistics 
      averageImplementationScore: enhancedNodes.reduce((sum, node) => 
        sum + (node.implementationScore || 0), 0) / enhancedNodes.length,
      averageTheoreticalScore: enhancedNodes.reduce((sum, node) => 
        sum + (node.theoreticalScore || 0), 0) / enhancedNodes.length,
      averageQualityScore: enhancedNodes.reduce((sum, node) => 
        sum + (node.qualityScore || 0), 0) / enhancedNodes.length,
      averageCodeBlockScore: enhancedNodes.reduce((sum, node) => 
        sum + (node.codeBlockScore || 0), 0) / enhancedNodes.length,
      
      nodesWithCodeBlocks: enhancedNodes.filter(node => 
        node.codeBlockScore > 0.1).length,
      
      // top scoring nodes breakdown
      topNodes: enhancedNodes.slice(0, 5).map(node => ({
        finalScore: node.score,
        implementationScore: node.implementationScore || 0,
        theoreticalScore: node.theoreticalScore || 0,
        qualityScore: node.qualityScore || 0,
        codeBlockScore: node.codeBlockScore || 0,
      })),
      
    };
  }
} 