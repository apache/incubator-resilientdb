import { config } from "@/config/environment";
import { tool } from "@llamaindex/core/tools";
import { deepseek } from "@llamaindex/deepseek";
import { PGVectorStore } from "@llamaindex/postgres";
import { agent, AgentWorkflow } from "@llamaindex/workflow";
import {
    BaseMemoryBlock,
    createMemory,
    Memory,
    vectorBlock
} from "llamaindex";
import { AGENT_RESEARCH_PROMPT, llamaService } from "./llama-service";


interface AgentFactory {
  createAgent(documents: string[]): Promise<AgentWorkflow>;
}

export class NexusAgent implements AgentFactory {
  private readonly memoryVectorStore: PGVectorStore;
  private readonly memory: Memory;
  private static instance?: NexusAgent;

  // Private constructor for singleton
  private constructor() {
    this.memoryVectorStore = this.initializeVectorStore();
    this.memory = this.createMemorySystem();
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

  private createMemorySystem(): Memory {
    const memoryBlocks: BaseMemoryBlock[] = [
    //   staticBlock({
    //     content: `My name is Logan, and I live in Saskatoon. I work at LlamaIndex.`,
    //   }),
    //   factExtractionBlock({
    //     priority: 1,
    //     llm: deepseek({
    //       model: "deepseek-chat",
    //       temperature: 0.1,
    //     }),
    //     maxFacts: 10,
    //   }),
      vectorBlock({
        vectorStore: this.memoryVectorStore,
        priority: 2,
        queryOptions: {
            similarityTopK: 3,
        }
      }),
    ];

    return createMemory({
    tokenLimit: 20,
      memoryBlocks,
    });
  }

  private createDocumentSearchTool(documents: string[]) {
    return tool(llamaService.retrieve, {
      name: "search_documents",
      description: "This tool can retrieve detailed information from the selected documents.",
      parameters: {
        type: "object",
        properties: {
          query: {
            type: "string",
            description: "The query to retrieve information from the document's vector embeddings.",
          },
          documentPaths: {
            type: "array",
            items: {
              type: "string",
            },
            description: "The list of document paths to search in",
          },
        },
        required: ["query", "documentPaths"],
      },
    });
  }

  private createWebSearchTool() {
    return tool(llamaService.searchWeb, {
      name: "search_web",
      description: "Search the web for information",
      parameters: {
        type: "object",
        properties: {
          query: {
            type: "string",
            description: "The search query optimized for web search",
          },
        },
        required: ["query"],
      },
    });
  }


  public async createAgent(documents: string[]): Promise<AgentWorkflow> {
    if (!documents || documents.length === 0) {
      throw new Error("Documents array cannot be empty");
    }

    const documentSearchTool = this.createDocumentSearchTool(documents);
    const webSearchTool = this.createWebSearchTool();

    return agent({
      name: "Nexus",
      description: "Responsible for overseeing the entire research process.",
      tools: [documentSearchTool, webSearchTool],
      systemPrompt: AGENT_RESEARCH_PROMPT(documents),
      llm: deepseek({
        model: "deepseek-chat",
      }),
      memory: this.memory,
    });
  }



  // Reset singleton for testing
  public static resetInstance(): void {
    NexusAgent.instance = undefined;
  }
}
