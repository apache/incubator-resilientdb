import { config } from "@/config/environment";
import { deepseek } from "@llamaindex/deepseek";
import { PGVectorStore } from "@llamaindex/postgres";
import { agent, AgentWorkflow } from "@llamaindex/workflow";
import { BaseMemoryBlock, createMemory, Memory, tool, vectorBlock } from "llamaindex";
import z from "zod";
import { AGENT_RESEARCH_PROMPT, llamaService } from "./llama-service";
import { robustFactExtractionBlock } from "./robust-fact-memory";


interface AgentFactory {
  createAgent(documents: string[], sessionId: string): Promise<AgentWorkflow>;
}

export class NexusAgent implements AgentFactory {
  private readonly memoryVectorStore: PGVectorStore;
  private static instance?: NexusAgent;
  // Map to persist agent workflows per sessionId
  private readonly workflows: Map<string, AgentWorkflow> = new Map();
  // Map to track the Memory instance associated with each session
  private readonly memories: Map<string, Memory> = new Map();

  // Private constructor for singleton
  private constructor() {
    this.memoryVectorStore = this.initializeVectorStore();
  }

  // Factory method for creating singleton
  public static async create(): Promise<NexusAgent> {
    if (!NexusAgent.instance) {
      
      if (!config?.databaseUrl) {
        throw new Error("Database URL is required");
      }

      NexusAgent.instance = new NexusAgent();
    }
    return NexusAgent.instance;
  }

  public static getInstance(): NexusAgent {
    if (!NexusAgent.instance) {
      throw new Error("NexusAgent must be created with create() method first");
    }
    return NexusAgent.instance;
  }

  private initializeVectorStore(): PGVectorStore {
    return new PGVectorStore({
      clientConfig: {
        connectionString: config.databaseUrl
      },
      tableName: "llamaindex_memory_embeddings",
      performSetup: true,
      dimensions: config.embedDim,
    });
  }

  private createMemoryWithSession(sessionId: string): Memory {
    const memoryBlocks: BaseMemoryBlock[] = [
      robustFactExtractionBlock({
        id: 'retrieved-facts',
        priority: 1,
        llm: deepseek({ model: config.deepSeekModel }),
        maxFacts: 10,
        isLongTerm: true,
      }),
      vectorBlock({
        id: sessionId, // This automatically handles session isolation
        vectorStore: this.memoryVectorStore,
        priority: 2,
        retrievalContextWindow: 5,
        queryOptions: {
          similarityTopK: 3,
          mode: "hybrid" as any,

      },
      }),
    ];

    const memory = createMemory({
      tokenLimit: 30000,
      shortTermTokenLimitRatio: 0.7,
      memoryBlocks,
    });

    this.memories.set(sessionId, memory);
    return memory;
  }

  private createDocumentSearchTool() {
    return tool({
      name: "search_documents",
      description: "This tool can retrieve detailed information from the selected documents.",
      parameters: z.object({
        query: z.string().describe("The query to retrieve information from the document's vector embeddings."),
        documentPaths: z.array(z.string()).describe("The list of document paths to search in"),
      }),
      execute: llamaService.retrieve,
    });
  }

  private createWebSearchTool() {
    return tool({
      name: "search_web",
      description: "Search the web for information",
      parameters: z.object({
        query: z.string().describe("The search query optimized for web search"),
      }),
      execute: llamaService.searchWeb,
    });
  }


  public async createAgent(documents: string[], sessionId: string): Promise<AgentWorkflow> {
    if (!documents || documents.length === 0) {
      throw new Error("Documents array cannot be empty");
    }

    // Return existing workflow if present to preserve short-term memory
    const existingWorkflow = this.workflows.get(sessionId);
    if (existingWorkflow) {
      return existingWorkflow;
    }

    const documentSearchTool = this.createDocumentSearchTool();
    const webSearchTool = this.createWebSearchTool();

    const memory = this.createMemoryWithSession(sessionId);
    const workflow = agent({
      name: "Nexus",
      description: "Responsible for overseeing the entire research process.",
      tools: [documentSearchTool, webSearchTool],
      systemPrompt: AGENT_RESEARCH_PROMPT(documents),
      llm: deepseek({
        model: config.deepSeekModel,
      }),
      memory,
    });

    // Persist workflow for subsequent requests in same session
    this.workflows.set(sessionId, workflow);
    return workflow;
  }

  // Reset singleton for testing and clear cached workflows
  public static resetInstance(): void {
    if (NexusAgent.instance) {
      NexusAgent.instance.workflows.clear();
      NexusAgent.instance.memories.clear();
    }
    NexusAgent.instance = undefined;
  }

  public getMemory(sessionId: string): Memory | undefined {
    return this.memories.get(sessionId);
  }
}
