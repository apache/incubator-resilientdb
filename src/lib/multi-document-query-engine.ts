import chalk from "chalk";
import { DocumentAgent, createDocumentAgent } from "./agents/document-agent";
import { OrchestratorAgent } from "./agents/orchestrator-agent";
import { TITLE_MAPPINGS } from "./constants";
import { documentIndexManager } from "./document-index-manager";

export interface DocumentSource {
  path: string;
  name: string;
  displayTitle?: string;
}

export interface ContextChunk {
  content: string;
  source: string;
  metadata?: Record<string, any>;
}

export interface QueryResult {
  context: string;
  sources: DocumentSource[];
  chunks: ContextChunk[];
  totalChunks: number;
}

export interface StreamingQueryOptions {
  useReranking?: boolean;
  rerankingConfig?: {
    topK?: number;
    minScore?: number;
    verbose?: boolean;
  };
  enableStreaming?: boolean;
}

export class MultiDocumentQueryEngine {
  private static instance: MultiDocumentQueryEngine;
  private orchestratorAgent: OrchestratorAgent;
  private documentAgents: Map<string, DocumentAgent> = new Map();

  private constructor() {
    // Initialize orchestrator agent
    this.orchestratorAgent = new OrchestratorAgent({
      systemPrompt: "You are a research assistant that helps users find information across multiple documents. Use the available document tools to answer questions accurately.",
      verbose: true
    });
  }

  static getInstance(): MultiDocumentQueryEngine {
    if (!MultiDocumentQueryEngine.instance) {
      MultiDocumentQueryEngine.instance = new MultiDocumentQueryEngine();
    }
    return MultiDocumentQueryEngine.instance;
  }

  // get display name for a document path
  private getDocumentDisplayName(documentPath: string): string {
    const filename = documentPath.split("/").pop() || documentPath;
    const lowerFilename = filename.toLowerCase();
    return TITLE_MAPPINGS[lowerFilename] || filename.replace(".pdf", "");
  }

  // query multiple documents using agent architecture
  async queryMultipleDocuments(
    query: string,
    documentPaths: string[],
    options: StreamingQueryOptions = {},
  ): Promise<QueryResult> {
    const {
      useReranking = false,
      rerankingConfig = {
        topK: 5,
        minScore: 0.1,
        verbose: false
      }
    } = options;

    try {
      // validate that all documents have indices
      const hasAllIndices = documentIndexManager.hasAllIndices(documentPaths);
      if (!hasAllIndices) {
        throw new Error(
          "Some document indices are not prepared. Please prepare all documents first.",
        );
      }

      // Ensure document agents exist for all paths
      await this.ensureDocumentAgents(documentPaths, useReranking, rerankingConfig);

      // For single document, use DocumentAgent directly
      if (documentPaths.length === 1) {
        console.log(chalk.yellow(`🎯 [MultiDocumentQueryEngine] SINGLE DOCUMENT ROUTE: ${this.getDocumentDisplayName(documentPaths[0])}`));
        console.log(chalk.yellow(`📄 [MultiDocumentQueryEngine] File: ${documentPaths[0].split("/").pop()}`));
        console.log(chalk.yellow(`⚡ [MultiDocumentQueryEngine] Bypassing orchestrator for efficiency`));
        
        const documentPath = documentPaths[0];
        const agent = this.documentAgents.get(documentPath);
        if (!agent) {
          throw new Error(`DocumentAgent not found for ${documentPath}`);
        }

        // Call DocumentAgent tool directly - bypasses orchestrator for efficiency
        const tool = agent.getTool();
        const response = await tool.call({ query });
        
        // Format response as string
        const responseText = typeof response === 'string' ? response : 
          (response && typeof response === 'object' && 'content' in response) ? (response as any).content :
          JSON.stringify(response) || "No response generated";
        
        return this.formatAgentResponse(responseText, documentPaths, [documentPath]);
      }

      // For multiple documents, use OrchestratorAgent
      console.log(chalk.yellow(`🎭 [MultiDocumentQueryEngine] MULTI-DOCUMENT ROUTE: ${documentPaths.length} documents`));
      console.log(chalk.yellow(`📚 [MultiDocumentQueryEngine] Documents: ${documentPaths.map(p => p.split("/").pop()).join(", ")}`));
      console.log(chalk.yellow(`🧠 [MultiDocumentQueryEngine] Using OrchestratorAgent for intelligent routing`));
      
      const result = await this.queryWithOrchestrator(query, documentPaths);
      return this.formatAgentResponse(result.response, documentPaths, result.sources);

    } catch (error) {
      throw error;
    }
  }

  // query a single document (convenience method)
  async querySingleDocument(
    query: string,
    documentPath: string,
    options: {
      useReranking?: boolean;
      rerankingConfig?: {
        topK?: number;
        minScore?: number;
        verbose?: boolean;
      };
    } = {},
  ): Promise<QueryResult> {
    return this.queryMultipleDocuments(query, [documentPath], options);
  }

  // get summary of available documents
  async getDocumentsSummary(documentPaths: string[]): Promise<{
    availableDocuments: number;
    totalDocuments: number;
    documentTitles: string[];
  }> {
    const hasAllIndices = documentIndexManager.hasAllIndices(documentPaths);
    const availableCount = hasAllIndices ? documentPaths.length : 0;
    
    return {
      availableDocuments: availableCount,
      totalDocuments: documentPaths.length,
      documentTitles: documentPaths.map(path => this.getDocumentDisplayName(path))
    };
  }

  // Ensure DocumentAgents exist for all document paths
  private async ensureDocumentAgents(
    documentPaths: string[], 
    useReranking: boolean = true,
    rerankingConfig: {
      topK?: number;
      minScore?: number;
      verbose?: boolean;
    } = {}
  ): Promise<void> {
    for (const documentPath of documentPaths) {
      if (!this.documentAgents.has(documentPath)) {
        const index = await documentIndexManager.getIndex(documentPath);
        if (!index) {
          throw new Error(`Index not found for document: ${documentPath}`);
        }

        const agent = await createDocumentAgent(documentPath, index, {
          displayName: this.getDocumentDisplayName(documentPath),
          useReranking,
          rerankingConfig: {
            topK: 10, // Increase to get more results
            minScore: 0.0, // Lower threshold to prevent filtering out everything
            verbose: true, // Enable for debugging
            ...rerankingConfig
          }
        });

        this.documentAgents.set(documentPath, agent);

        // Add existing agent to orchestrator (no duplication)
        this.orchestratorAgent.addDocumentAgent(agent);
      }
    }
  }

  // Query with OrchestratorAgent
  private async queryWithOrchestrator(
    query: string,
    documentPaths: string[]
  ): Promise<{ response: string; sources: string[] }> {
    const result = await this.orchestratorAgent.query(query);
    return result;
  }

  // Format agent response to match QueryResult interface
  private formatAgentResponse(
    response: string, 
    documentPaths: string[], 
    sourcePaths?: string[]
  ): QueryResult {
    const sources: DocumentSource[] = (sourcePaths || documentPaths).map(path => ({
      path,
      name: path.split("/").pop() || path,
      displayTitle: this.getDocumentDisplayName(path)
    }));

    // Create chunks from the response (simplified for agent responses)
    const chunks: ContextChunk[] = [{
      content: response,
      source: documentPaths[0] || "agent-response",
      metadata: { 
        generatedBy: "agent",
        timestamp: new Date().toISOString(),
        queryType: documentPaths.length === 1 ? "single-document" : "multi-document"
      }
    }];

    return {
      context: response,
      sources,
      chunks,
      totalChunks: chunks.length
    };
  }

  // Get information about loaded agents
  getAgentInfo(): {
    totalAgents: number;
    documentPaths: string[];
    orchestratorInitialized: boolean;
  } {
    return {
      totalAgents: this.documentAgents.size,
      documentPaths: Array.from(this.documentAgents.keys()),
      orchestratorInitialized: !!this.orchestratorAgent
    };
  }

  // Clear all agents (useful for testing or reset)
  clearAllAgents(): void {
    this.documentAgents.clear();
    // Reinitialize orchestrator with no documents
    this.orchestratorAgent = new OrchestratorAgent({
      systemPrompt: "You are a research assistant that helps users find information across multiple documents. Use the available document tools to answer questions accurately.",
      verbose: false
    });
  }
}

// export singleton instance
export const multiDocumentQueryEngine = MultiDocumentQueryEngine.getInstance();