import { config } from "@/config/environment";
import { deepseek } from "@llamaindex/deepseek";
import { SupabaseVectorStore } from "@llamaindex/supabase";
import { agent, AgentWorkflow, multiAgent } from "@llamaindex/workflow";
import { BaseMemoryBlock, createMemory, Memory, Settings, tool, ToolCallLLM, vectorBlock } from "llamaindex";
import z from "zod";
import { AGENT_RESEARCH_PROMPT, llamaService } from "./llama-service";
import { robustFactExtractionBlock } from "./robust-fact-memory";


interface AgentFactory {
  createAgent(documents: string[], sessionId: string): Promise<AgentWorkflow>;
}

export class NexusAgent implements AgentFactory {
  private readonly memoryVectorStore: SupabaseVectorStore;
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

      if (!config?.supabaseUrl || !config?.supabaseKey) {
        throw new Error("Supabase URL and Key are required");
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

  private initializeVectorStore(): SupabaseVectorStore {
    return new SupabaseVectorStore({
      supabaseUrl: config.supabaseUrl,
      supabaseKey: config.supabaseKey,
      table: config.supabaseMemoryTable,
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

// CodeAgent to be selected when "code" mode is active in the UI.
export class CodeAgent implements AgentFactory {
  private readonly language: string;
  constructor(language: string = "ts") {
    this.language = language;
  }

  // eslint-disable-next-line @typescript-eslint/no-unused-vars
  public async createAgent(_documents: string[], _sessionId: string): Promise<AgentWorkflow> {
    const retrieveDocumentsTool = tool({
      name: "retrieve_documents",
      description: "CRITICAL: You MUST use this tool FIRST to retrieve document content before providing any analysis. This tool searches through the available documents and returns relevant information that you need to analyze.",
      parameters: z.object({
        query: z.string().describe("The search query to find relevant content in the documents. Be specific about what technical information you need."),
        documentPaths: z.array(z.string()).describe("The list of document paths to search in. Use the available documents: " + _documents.join(", ")),
      }),
      execute: async ({ query, documentPaths }) => {
        const results = await llamaService.retrieve({ query, documentPaths });
        return results
      },
    });

    const codeAgent = agent({
      name: "CodeAgent",
      description: "Generates final code implementation.",
      tools: [], // Include tools for message compatibility but don't use them
      systemPrompt: `You are implementing the final solution...

      Using the research and pseudocode from the previous workflow steps, your task is to generate clean, production-ready ${this.language} code.

      IMPORTANT: Do NOT use any tools. Focus only on code generation based on the research and pseudocode already provided in the conversation history.

      Generate the complete implementation immediately:
      - Write functional, well-structured code
      - Include only essential comments for complex logic  
      - Ensure the code follows best practices
      - Stream the output directly without any setup or context checking

      Start coding now:`,
      llm: Settings.llm as ToolCallLLM
    });

    const pseudoCodeAgent = agent({
      name: "PseudoCodeAgent",
      description: "Creates structured pseudocode.",
      tools: [], // Include tools for message compatibility but don't use them
      canHandoffTo: [codeAgent],
      systemPrompt: `You are creating structured pseudocode for implementation.

      You will receive research findings from the previous workflow step through the conversation history.

      IMPORTANT: Do NOT use any tools. Focus only on creating pseudocode based on the research already provided in the conversation history.

      Your task:
      - Start with: "Creating structured pseudocode..."
      - Develop detailed, step-by-step pseudocode
      - Make it comprehensive enough for code generation
      - Stream the pseudocode efficiently
      - MANDATORY: After completing pseudocode, you MUST use the handOff tool to transfer control to CodeAgent with reason "Pseudocode complete, moving to implementation"

      Begin immediately with pseudocode creation based on the research context, and finally use handOff tool to transfer to CodeAgent.`,
      llm: Settings.llm as ToolCallLLM

    });

    const plannerAgent = agent({
      name: "PlannerAgent",
      description: "Conducts research and planning.",
      tools: [retrieveDocumentsTool], // Only one fast tool
      canHandoffTo: [pseudoCodeAgent],
      systemPrompt: `You are conducting research and analysis for code generation.

      CRITICAL: You MUST use the retrieve_documents tool FIRST before providing any analysis.

      Your task sequence:
      1) ALWAYS start by calling retrieve_documents tool with a relevant query to get content from: [${_documents.join(", ")}]
      2) After receiving the tool results, analyze the retrieved content and provide your research findings
      3) MANDATORY: After analysis, you MUST use the handOff tool to transfer control to PseudoCodeAgent with reason "Research complete, moving to pseudocode design"

      Focus on:
      - Technical details for implementation
      - Key patterns from the documents
      - Specific requirements and constraints
      - Architecture considerations

      IMPORTANT: 
      - Do not skip the tool call step. You must retrieve documents first, then analyze.
      - You MUST call the handOff tool to transfer to PseudoCodeAgent after your analysis.`,
      llm: Settings.llm as ToolCallLLM
    });

    const workflow = multiAgent({
      agents: [plannerAgent, pseudoCodeAgent, codeAgent],
      rootAgent: plannerAgent,
      verbose: true,
      memory: createMemory({
        tokenLimit: 30000,
        shortTermTokenLimitRatio: 0.7,
      })
    });
    return workflow;
  }
}
