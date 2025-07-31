import chalk from "chalk";
import { BaseRetriever, NodeWithScore, PromptHelper, QueryBundle, QueryEngineTool, VectorStoreIndex, getResponseSynthesizer } from "llamaindex";
import { TITLE_MAPPINGS } from "../constants";
import { HuggingFaceReranker, createHuggingFaceReranker } from "../rerankers/huggingface-reranker";

export interface DocumentAgentConfig {
  documentPath: string;
  index: VectorStoreIndex;
  displayName?: string;
  description?: string;
  useReranking?: boolean;
  rerankingConfig?: {
    topK?: number;
    minScore?: number;
    verbose?: boolean;
  };
}

export interface DocumentAgentMetadata {
  name: string;
  description: string;
  documentPath: string;
  displayName: string;
}

/**
 * DocumentAgent wraps a single document's VectorStoreIndex as a QueryEngineTool
 * This allows individual documents to be used as tools in a multi-agent system
 */
export class DocumentAgent {
  private queryTool: QueryEngineTool;
  private metadata: DocumentAgentMetadata;
  private reranker?: HuggingFaceReranker;
  private useReranking: boolean;
  private config: DocumentAgentConfig;
  
  constructor(config: DocumentAgentConfig) {
    // Store config for later use
    this.config = config;
    
    // Settings should be configured at application startup
    
    // Extract display name from path if not provided
    const displayName = config.displayName || this.extractDisplayName(config.documentPath);
    
    // Set up reranking (disabled by default for better performance)
    this.useReranking = config.useReranking ?? false; // Default to disabled for speed
    if (this.useReranking) {
      this.reranker = createHuggingFaceReranker({
        topK: 10, // Increase to get more results
        minScore: 0.0, // Lower threshold to prevent filtering out everything  
        verbose: true, // Enable for debugging
        ...config.rerankingConfig,
      });
    }
    
    // Generate tool metadata with enhanced description for better routing
    const fileName = config.documentPath.split("/").pop() || config.documentPath;
    this.metadata = {
      name: `query_${this.sanitizeToolName(config.documentPath)}_tool`,
      description: config.description || `Search and answer questions about "${displayName}" (file: ${fileName}). Use this tool when the user asks about topics related to ${displayName}, mentions "${fileName}", or asks about this specific document.`,
      documentPath: config.documentPath,
      displayName,
    };
    
    // Create enhanced query engine with optimized retrieval settings
    const retriever = config.index.asRetriever({
      similarityTopK: this.useReranking ? 15 : 5, // Fewer results for faster processing
    });
    
    // Create custom retriever with reranking if enabled
    // For very large documents, consider disabling reranking to improve performance
    const enhancedRetriever = this.useReranking && this.reranker 
      ? this.createRerankingRetriever(retriever, this.reranker)
      : retriever;
      
    
    // Configure query engine to send ALL chunks in a SINGLE API call
    // Create PromptHelper with high token limits to match DeepSeek's 128k capacity
    const promptHelper = new PromptHelper({
      contextWindow: 120000, // High limit for DeepSeek (120k tokens)
      numOutput: 4000, // Allow longer responses
      chunkOverlapRatio: 0.1,
      chunkSizeLimit: undefined, // No chunk size limit
    });
    
    // Use tree_summarize with high token limits to fit all chunks in single call
    const responseSynthesizer = getResponseSynthesizer("tree_summarize", {
      promptHelper,
    });
    
    const queryEngine = config.index.asQueryEngine({
      retriever: enhancedRetriever,
      responseSynthesizer, // Single call with all chunks
      // Note: streaming is enabled via the call method, not configuration
    });
    
    // Create the QueryEngineTool with enhanced logging
    this.queryTool = new QueryEngineTool({
      queryEngine,
      metadata: {
        name: this.metadata.name,
        description: this.metadata.description,
      },
    });

    // Note: Tool call logging is handled at the OrchestratorAgent level
    
    console.log(chalk.blue(`[DocumentAgent] DocumentAgent created: ${displayName}${this.useReranking ? ' (reranking enabled)' : ''}`));
  }
  
  /**
   * Get the QueryEngineTool for use in agents
   */
  getTool(): QueryEngineTool {
    return this.queryTool;
  }
  
  /**
   * Get metadata about this document agent
   */
  getMetadata(): DocumentAgentMetadata {
    return { ...this.metadata };
  }
  
  /**
   * Get current configuration including reranking settings
   */
  getConfig(): DocumentAgentMetadata & { useReranking: boolean; rerankingConfig?: any } {
    return { 
      ...this.metadata,
      useReranking: this.useReranking,
      rerankingConfig: this.reranker?.getConfig()
    };
  }
  
  /**
   * Get the document path
   */
  getDocumentPath(): string {
    return this.metadata.documentPath;
  }
  
  /**
   * Get the display name
   */
  getDisplayName(): string {
    return this.metadata.displayName;
  }
  


  /**
   * Execute the actual query with retry logic
   */
  private async executeQuery(query: string): Promise<string> {
    const maxRetries = 2;
    let lastError: Error | null = null;
    
    for (let attempt = 1; attempt <= maxRetries; attempt++) {
      try {
        const startTime = Date.now();
        const response = await this.queryTool.call({ query });
        const duration = Date.now() - startTime;
        
        console.log(chalk.blue(`[DocumentAgent] Query completed (${duration}ms): ${this.metadata.displayName}`));
        
        // Handle different response types
        if (typeof response === 'string') {
          return response;
        }
        
        if (response && typeof response === 'object' && 'content' in response) {
          return (response as any).content || "No content in response";
        }
        
        if (response && typeof response === 'object' && 'message' in response) {
          return (response as any).message || "No message in response";
        }
        
        return JSON.stringify(response) || "No response generated";
        
      } catch (error) {
        lastError = error instanceof Error ? error : new Error(String(error));
        console.error(chalk.red(`[DocumentAgent] Query attempt ${attempt} failed: ${lastError.message}`));
        
        if (attempt === maxRetries) {
          break;
        }
        
        const waitTime = Math.pow(2, attempt - 1) * 1000;
        await new Promise(resolve => setTimeout(resolve, waitTime));
      }
    }
    
    console.error(chalk.red(`[DocumentAgent] All ${maxRetries} attempts failed: ${this.metadata.displayName}`));
    throw new Error(`Failed to query document after ${maxRetries} attempts: ${lastError?.message || "Unknown error"}`);
  }
  
  /**
   * Extract display name from document path
   */
  private extractDisplayName(documentPath: string): string {
    const filename = documentPath.split("/").pop() || documentPath;
    const lowerFilename = filename.toLowerCase();
    
    // Use title mappings if available
    if (TITLE_MAPPINGS[lowerFilename]) {
      return TITLE_MAPPINGS[lowerFilename];
    }
    
    // Clean up filename
    return filename.replace(/\.(pdf|txt|md|docx?)$/i, "").replace(/[-_]/g, " ");
  }
  
  /**
   * Sanitize document path for use as tool name
   */
  private sanitizeToolName(documentPath: string): string {
    return documentPath
      .replace(/[^a-zA-Z0-9]/g, '_') // Replace non-alphanumeric with underscores
      .replace(/_+/g, '_') // Replace multiple underscores with single
      .replace(/^_|_$/g, '') // Remove leading/trailing underscores
      .toLowerCase();
  }
  
  /**
   * Create a reranking-enhanced retriever
   */
  private createRerankingRetriever(baseRetriever: BaseRetriever, reranker: HuggingFaceReranker): BaseRetriever {
    // Create a wrapper class that extends BaseRetriever
    class RerankingRetriever extends BaseRetriever {
      constructor(private baseRetriever: BaseRetriever, private reranker: HuggingFaceReranker) {
        super();
      }
      
      async _retrieve(params: QueryBundle): Promise<NodeWithScore[]> {
        const retrievalStartTime = Date.now();
        
        // Get initial results from base retriever
        const initialResults = await this.baseRetriever.retrieve(params);
        
        if (!initialResults || initialResults.length === 0) {
          return initialResults;
        }
        
        // Apply reranking using the query string
        const queryStr = typeof params.query === 'string' ? params.query : params.toString();
        const rerankedResults = await this.reranker.rerank(queryStr, initialResults);
        
        const totalTime = Date.now() - retrievalStartTime;
        console.log(chalk.blue(`[DocumentAgent] Reranking completed (${totalTime}ms): ${initialResults.length} → ${rerankedResults.length} nodes`));
        
        // Convert back to NodeWithScore format
        return rerankedResults.map(result => result.node);
      }
    }
    
    return new RerankingRetriever(baseRetriever, reranker);
  }
}

/**
 * Factory function to create DocumentAgent from document path and index
 */
export async function createDocumentAgent(
  documentPath: string,
  index: VectorStoreIndex,
  options: {
    displayName?: string;
    description?: string;
    useReranking?: boolean;
    rerankingConfig?: {
      topK?: number;
      minScore?: number;
      verbose?: boolean;
    };
  } = {}
): Promise<DocumentAgent> {
  return new DocumentAgent({
    documentPath,
    index,
    displayName: options.displayName,
    description: options.description,
    useReranking: options.useReranking,
    rerankingConfig: options.rerankingConfig,
  });
}

/**
 * Create multiple DocumentAgents from a map of paths to indices
 */
export async function createMultipleDocumentAgents(
  documentsMap: Map<string, VectorStoreIndex>,
  options: {
    displayNames?: Map<string, string>;
    descriptions?: Map<string, string>;
  } = {}
): Promise<Map<string, DocumentAgent>> {
  const agents = new Map<string, DocumentAgent>();
  
  for (const [documentPath, index] of documentsMap) {
    const agent = await createDocumentAgent(documentPath, index, {
      displayName: options.displayNames?.get(documentPath),
      description: options.descriptions?.get(documentPath),
    });
    
    agents.set(documentPath, agent);
  }
  
  console.log(chalk.blue(`[DocumentAgent] Created ${agents.size} DocumentAgents`));
  return agents;
}