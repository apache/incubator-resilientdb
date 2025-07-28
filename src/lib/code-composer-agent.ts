/**
 * CodeComposerAgent - AI-powered context analysis and ranking for code generation
 * Replaces keyword-based CodeReranker with semantic understanding using AI SDK
 */

import { AgentOptions, AgentStats, AnalyzedChunk, RetrievedNode } from '@/app/research/types';
import { DeepSeekLLM } from '@llamaindex/deepseek';
import { TikTokenEstimator, TokenEstimator } from './token-estimator';


export class CodeComposerAgent {
  private llm: DeepSeekLLM;
  private tokenEstimator: TokenEstimator;
  private maxTokens: number;
  private stats: AgentStats;

  constructor(llm: DeepSeekLLM, options: AgentOptions = {}) {
    this.llm = llm;
    this.tokenEstimator = new TikTokenEstimator();
    this.maxTokens = options.maxTokens || 32000;
    this.stats = {
      totalChunks: 0,
      analyzedChunks: 0,
      selectedChunks: 0,
      totalTokens: 0,
      processingTime: 0
    };
  }

  async rerank(nodes: RetrievedNode[], query: string, options: AgentOptions = {}): Promise<RetrievedNode[]> {
    const startTime = Date.now();
    this.stats.totalChunks = nodes.length;

    console.log(`[CodeComposerAgent] Starting rerank process for ${nodes.length} chunks`);
    console.log(`[CodeComposerAgent] Query: "${query}"`);
    console.log(`[CodeComposerAgent] Options:`, {
      language: options.language,
      scope: options.scope,
      maxTokens: options.maxTokens
    });

    try {
      // Step 1: Analyze chunks for relevance
      console.log(`[CodeComposerAgent] Step 1: Starting chunk analysis...`);
      const analyzedChunks = await this.analyzeChunks(nodes, query, options);
      console.log(`[CodeComposerAgent] Step 1 Complete: Analyzed ${analyzedChunks.length} chunks`);

      // Step 2: Select and rank chunks based on analysis
      console.log(`[CodeComposerAgent] Step 2: Starting chunk ranking...`);
      const rankedChunks = await this.selectAndRankChunks(analyzedChunks, options);
      console.log(`[CodeComposerAgent] Step 2 Complete: Ranked ${rankedChunks.length} chunks`);

      // Step 3: Apply token budget constraints
      console.log(`[CodeComposerAgent] Step 3: Applying token budget constraints...`);
      const finalChunks = this.applyTokenBudget(rankedChunks);
      console.log(`[CodeComposerAgent] Step 3 Complete: Selected ${finalChunks.length} chunks within token budget`);

      this.stats.processingTime = Date.now() - startTime;
      this.stats.selectedChunks = finalChunks.length;

      console.log(`[CodeComposerAgent] Process completed in ${this.stats.processingTime / 1000} seconds`);
      console.log(`[CodeComposerAgent] Final stats:`, this.getStats());

      return finalChunks;
    } catch (error) {
      console.error('[CodeComposerAgent] Error during rerank process:', error);
      console.log('[CodeComposerAgent] Falling back to original scoring');
      // Fallback to original scoring
      return nodes.sort((a, b) => b.score - a.score);
    }
  }

  private async analyzeChunks(nodes: RetrievedNode[], query: string, options: AgentOptions): Promise<AnalyzedChunk[]> {
    const language = options.language || 'typescript';
    const scope = options.scope || [];

    console.log(`[CodeComposerAgent] Analyzing chunks with language: ${language}, scope: ${scope.join(', ')}`);

    const analysisPrompt = `You are an expert code analysis agent. Analyze document chunks for their relevance to implementing code based on a user query.

For each chunk, evaluate:
1. Implementation Relevance (0-10): How useful is this for actual code implementation?
2. Code Quality (0-10): Does it contain high-quality, complete code examples?
3. Query Alignment (0-10): How well does it match the specific user request?
4. Has Code Examples (true/false): Contains actual code snippets or implementations?

User Query: "${query}"
Target Language: ${language}
${scope.length > 0 ? `Scope Focus: ${scope.join(', ')}` : ''}

Analyze each chunk and provide scores with brief reasoning.`;

    const chunksToAnalyze = nodes;
    console.log(`[CodeComposerAgent] Processing ${chunksToAnalyze.length} chunks`);

    const analyzedChunks: AnalyzedChunk[] = [];

// Calculate optimal batch size to minimize batches while staying within token limits
    const batchSize = this.calculateOptimalBatchSize(analysisPrompt, chunksToAnalyze);
    const totalBatches = Math.ceil(chunksToAnalyze.length / batchSize);
    console.log(`[CodeComposerAgent] Using dynamic batch size: ${batchSize} chunks per batch`);
    console.log(`[CodeComposerAgent] Processing ${totalBatches} batches`);

    for (let i = 0; i < chunksToAnalyze.length; i += batchSize) {
      const batchNumber = Math.floor(i / batchSize) + 1;
      const batch = chunksToAnalyze.slice(i, i + batchSize);
      console.log(`[CodeComposerAgent] Processing batch ${batchNumber}/${totalBatches} (${batch.length} chunks)`);

      const batchAnalysis = await this.analyzeBatch(batch, analysisPrompt, query, options);
      analyzedChunks.push(...batchAnalysis);

      console.log(`[CodeComposerAgent] Batch ${batchNumber} complete: ${batchAnalysis.length} chunks analyzed`);
    }

    this.stats.analyzedChunks = analyzedChunks.length;
    console.log(`[CodeComposerAgent] Analysis complete: ${analyzedChunks.length} total chunks analyzed`);

    return analyzedChunks;
  }

  private async analyzeBatch(
    batch: RetrievedNode[],
    basePrompt: string,
    query: string,
    options: AgentOptions
  ): Promise<AnalyzedChunk[]> {
    const batchContent = batch.map((node, idx) =>
      `CHUNK ${idx + 1}:\n${node.node.getContent().substring(0, 1500)}...\n---`
    ).join('\n\n');

    const fullPrompt = `${basePrompt}

${batchContent}

For each chunk, respond in this JSON format:
{
  "analyses": [
    {
      "chunkIndex": 0,
      "implementationRelevance": 8,
      "codeQuality": 7,
      "queryAlignment": 9,
      "hasCodeExamples": true,
      "reasoning": "Brief explanation of scores"
    }
  ]
}`;

    try {
      console.log(`[CodeComposerAgent] Sending batch to LLM for analysis...`);
      const result = await this.llm.chat({
        messages: [{ role: "user", content: fullPrompt }],
        stream: false,
      });

      const responseContent = result.message.content as string;
      console.log(`[CodeComposerAgent] LLM Response received (${responseContent.length} chars)`);
      console.log(`[CodeComposerAgent] LLM Response preview:`, responseContent.substring(0, 200) + '...');

      const analysis = this.parseAnalysisResponse(responseContent);
      console.log(`[CodeComposerAgent] Parsed ${analysis.analyses?.length || 0} analysis results`);

      return batch.map((node, idx) => {
        const nodeAnalysis = analysis.analyses.find((a: any) => a.chunkIndex === idx) || {
          implementationRelevance: 5,
          codeQuality: 5,
          queryAlignment: 5,
          hasCodeExamples: false,
          reasoning: "Analysis failed, using default scores"
        };

        const finalScore = this.calculateFinalScore(node.score, nodeAnalysis, options);

        const analyzedChunk = {
          content: node.node.getContent(),
          originalScore: node.score,
          implementationRelevance: nodeAnalysis.implementationRelevance,
          codeQuality: nodeAnalysis.codeQuality,
          queryAlignment: nodeAnalysis.queryAlignment,
          hasCodeExamples: nodeAnalysis.hasCodeExamples,
          finalScore,
          reasoning: nodeAnalysis.reasoning
        };

        console.log(`[CodeComposerAgent] Chunk ${idx} analysis:`, analyzedChunk);

        return analyzedChunk;
      });
    } catch (error) {
      console.error('[CodeComposerAgent] Batch analysis failed:', error);
      console.log('[CodeComposerAgent] Using fallback scores for this batch');
      return batch.map(node => ({
        content: node.node.getContent(),
        originalScore: node.score,
        implementationRelevance: 5,
        codeQuality: 5,
        queryAlignment: 5,
        hasCodeExamples: false,
        finalScore: node.score,
        reasoning: "Analysis failed, using original score"
      }));
    }
  }

  private calculateOptimalBatchSize(basePrompt: string, chunks: RetrievedNode[]): number {
    const responseTokensPerChunk = 150;
    const safetyMargin = 500;

    const basePromptTokens = this.tokenEstimator.estimate(basePrompt);

    const sampleChunk = chunks.length > 0 ? chunks[0].node.getContent().substring(0, 1500) : '';
    const chunkFormatOverhead = 50;
    const avgChunkTokens = this.tokenEstimator.estimate(sampleChunk) + chunkFormatOverhead;

    const availableTokens = this.maxTokens - basePromptTokens - safetyMargin;

    const tokensPerChunkInBatch = avgChunkTokens + responseTokensPerChunk;
    const maxChunksInBatch = Math.floor(availableTokens / tokensPerChunkInBatch);

    const optimalBatchSize = Math.max(1, Math.min(maxChunksInBatch, 15));

    console.log(`[CodeComposerAgent] Batch size calculation:`, {
      maxTokens: this.maxTokens,
      basePromptTokens,
      avgChunkTokens,
      responseTokensPerChunk,
      safetyMargin,
      availableTokens,
      tokensPerChunkInBatch,
      calculatedBatchSize: maxChunksInBatch,
      finalBatchSize: optimalBatchSize
    });

    return optimalBatchSize;
  }

  private parseAnalysisResponse(response: string): any {
    try {
      // Try to extract JSON from response
      const jsonMatch = response.match(/\{[\s\S]*\}/);
      if (jsonMatch) {
        return JSON.parse(jsonMatch[0]);
      }

      // Fallback parsing
      return { analyses: [] };
    } catch (error) {
      console.error('Failed to parse analysis response:', error);
      return { analyses: [] };
    }
  }

  private calculateFinalScore(
    originalScore: number,
    analysis: any,
    options: AgentOptions
  ): number {
    const weights = {
      implementation: options.implementationWeight || 1.5,
      quality: options.qualityWeight || 2.0,
      alignment: 1.8,
      codeBonus: 1.3
    };

    const normalizedScores = {
      implementation: analysis.implementationRelevance / 10,
      quality: analysis.codeQuality / 10,
      alignment: analysis.queryAlignment / 10
    };

    const weightedScore =
      (normalizedScores.implementation * weights.implementation) +
      (normalizedScores.quality * weights.quality) +
      (normalizedScores.alignment * weights.alignment);

    const codeBonus = analysis.hasCodeExamples ? weights.codeBonus : 1.0;

    // Combine with original score
    const enhancedScore = (originalScore * 0.3) + (weightedScore * 0.7 * codeBonus);

    return Math.max(enhancedScore, originalScore * 0.1);
  }

  private async selectAndRankChunks(
    analyzedChunks: AnalyzedChunk[],
    options: AgentOptions
  ): Promise<RetrievedNode[]> {
    console.log(`[CodeComposerAgent] Ranking ${analyzedChunks.length} analyzed chunks`);

    const sortedChunks = analyzedChunks.sort((a, b) => b.finalScore - a.finalScore);

    console.log(`[CodeComposerAgent] Top 5 ranked chunks:`,
      sortedChunks.slice(0, 5).map((chunk, idx) => ({
        rank: idx + 1,
        finalScore: chunk.finalScore,
        originalScore: chunk.originalScore,
        implementationRelevance: chunk.implementationRelevance,
        codeQuality: chunk.codeQuality,
        reasoning: chunk.reasoning.substring(0, 100) + '...'
      }))
    );

    return sortedChunks.map(chunk => ({
      node: {
        getContent: () => chunk.content,
        metadata: {
          agentAnalysis: {
            implementationRelevance: chunk.implementationRelevance,
            codeQuality: chunk.codeQuality,
            queryAlignment: chunk.queryAlignment,
            reasoning: chunk.reasoning
          }
        }
      },
      score: chunk.finalScore
    }));
  }

  private applyTokenBudget(rankedChunks: RetrievedNode[]): RetrievedNode[] {
    console.log(`[CodeComposerAgent] Applying token budget to ${rankedChunks.length} ranked chunks (max: ${this.maxTokens} tokens)`);

    let totalTokens = 0;
    const selectedChunks: RetrievedNode[] = [];

    for (let i = 0; i < rankedChunks.length; i++) {
      const chunk = rankedChunks[i];
      const content = chunk.node.getContent();
      const chunkTokens = this.tokenEstimator.estimate(content);

      if (totalTokens + chunkTokens <= this.maxTokens) {
        selectedChunks.push(chunk);
        totalTokens += chunkTokens;
        console.log(`[CodeComposerAgent] Selected chunk ${i + 1}: ${chunkTokens} tokens (total: ${totalTokens}/${this.maxTokens})`);
      } else {
        console.log(`[CodeComposerAgent] Chunk ${i + 1} would exceed token limit (${chunkTokens} tokens, would total ${totalTokens + chunkTokens})`);
        break;
      }
    }

    this.stats.totalTokens = totalTokens;
    console.log(`[CodeComposerAgent] Token budget applied: ${selectedChunks.length}/${rankedChunks.length} chunks selected, ${totalTokens}/${this.maxTokens} tokens used`);

    return selectedChunks;
  }

  getStats(): AgentStats {
    return { ...this.stats };
  }

  resetStats(): void {
    this.stats = {
      totalChunks: 0,
      analyzedChunks: 0,
      selectedChunks: 0,
      totalTokens: 0,
      processingTime: 0
    };
  }
} 