import { deepseek } from "@llamaindex/deepseek";
import { SupabaseVectorStore } from "@llamaindex/supabase";
import { agent, AgentWorkflow, multiAgent } from "@llamaindex/workflow";
import dotenv from "dotenv";
import fs from "fs/promises";
import {
  ChatMessage,
  ContextChatEngine,
  Document,
  FilterOperator,
  IngestionPipeline,
  LlamaParseReader,
  MarkdownNodeParser,
  NodeWithScore,
  SentenceSplitter,
  Settings,
  StorageContext,
  storageContextFromDefaults,
  SummaryExtractor,
  TextNode,
  tool,
  ToolCallLLM,
  VectorStoreIndex
} from "llamaindex";
import { TavilyClient } from "tavily";
import z from "zod";
import { config } from "../config/environment";
import { configureLlamaSettings } from "./config/llama-settings";
import { TITLE_MAPPINGS } from "./constants";

dotenv.config();

const RESEARCH_SYSTEM_PROMPT = `
You are Nexus, an AI research assistant specialized in Apache ResilientDB, blockchain technology, distributed systems, and fault-tolerant consensus protocols. Your primary role is to help students, researchers, and practitioners understand complex technical concepts related to Apache ResilientDB and blockchain systems, who can answer questions about documents. 
You have access to the content of a document and can provide accurate, detailed answers based on that content.
- When asked about the document, always base your responses on the information provided in the document. When possible, cite sections, pages, or other specific information from the document.
- If you cannot find specific information in the document, say so clearly.
- If asked about something that is not in the document, give a brief answer and try to guide the user to ask about something that is in the document.
- Please favor referring to the document by its title, instead of the file name.
- If referring to a source, do not use metadata terms like "node" or "Header_1". 
\n\n
Citation Instructions: 
    - When referencing information from documents, use the format [^id] where id is the 1-based index of the source node
    - Only include the citation markers. Do not include any other citation explanations in your response
    - When consecutive statements reference the same source document AND page, only include the citation marker once at the end of that section
    - Always include citations for each distinct source, even if from the same document but different pages
`;

export const AGENT_RESEARCH_PROMPT = (documentPaths: string[]) =>
  `
## Core Identity
You are **Nexus**, an AI research assistant specialized in Apache ResilientDB and its related blockchain technology, distributed systems, and fault-tolerant consensus protocols. Your primary role is to help students, researchers, and practitioners understand complex technical concepts related to Apache ResilientDB and blockchain systems.

## Capabilities
- Answer questions about documents with access to document content
- Provide accurate, detailed answers based on document content
- Explain complex technical concepts in blockchain and distributed systems
- Assist with Apache ResilientDB research and implementation

## Operational Guidelines

- For most questions, you should use the **search_documents tool** to answer questions, even if the question is not about ResilientDB or related blockchain topics.
- If uncertain about which document to search, search all available documents.
- ALWAYS state to the user that you are going to use a tool before you call it..
- Example: "Let me look through the documents..." 
- Then proceed to use the appropriate tools to find information

### Tool Selection
- Prioritize the **search_documents tool** for answering questions.
  - Refer to documents by their **title**, not by their file name 
- Use the **search_web tool** for web searches when needed

### Document Handling

AVAILABLE DOCUMENTS (documentPath - Title):
{
${documentPaths.map((docPath) => `"${docPath}": "${TITLE_MAPPINGS[docPath.replace("documents/", "")]}"`).join(",\n")}
}

#### Response Requirements
- **Always** base responses on information provided in the document when asked about document content, and from the web when needed.
- **Cite specific sections, pages, or other information** from the document/web results when possible
- **Clearly state** if specific information cannot be found in the document/web
- If asked about content NOT related to ResilientDB or related blockchain topics:
  - Give a brief answer
  - Guide the user to ask about something that IS related to ResilientDB or related blockchain topics.


#### Reference Standards
- Favor referring to documents by their **title** instead of file name
- When referencing sources, avoid metadata terms like "node" or "Header_1"

## Communication Style
- Professional, friendly and technical when discussing complex concepts
- Clear and educational for students and researchers
- Helpful and guided when users ask about topics outside available documents
`;

const PARSING_CACHE = "parsing-cache.json";

export class LlamaService {
  private vectorStore: SupabaseVectorStore;
  private pipeline: IngestionPipeline;
  private static instance: LlamaService;

  private constructor() {
    configureLlamaSettings();

    console.info(`[LlamaService] Initializing SupabaseVectorStore with table: ${config.supabaseVectorTable}`);

    if (!config.supabaseUrl || !config.supabaseKey) {
      throw new Error("Supabase URL and key are required for vector store initialization");
    }

    this.vectorStore = new SupabaseVectorStore({
      supabaseUrl: config.supabaseUrl,
      supabaseKey: config.supabaseKey,
      table: config.supabaseVectorTable,
    });

    this.pipeline = new IngestionPipeline({
      transformations: [
        new MarkdownNodeParser(),
        new SentenceSplitter({
          chunkSize: 768,
          chunkOverlap: 20,
        }),
        new SummaryExtractor(),
        Settings.embedModel,
      ],
      vectorStore: this.vectorStore,
      // docStore: optional Supabase doc store not used
    });
  }

  public static getInstance(): LlamaService {
    if (!LlamaService.instance) {
      LlamaService.instance = new LlamaService();
    }
    return LlamaService.instance;
  }

  /**
   * Clear the parsing cache to force re-ingestion of all documents
   */
  async clearCache(): Promise<void> {
    try {
      await fs.unlink(PARSING_CACHE);
      console.info("[LlamaService] Parsing cache cleared");
    } catch {
      console.info("[LlamaService] No cache file to clear");
    }
  }

  private getVectorStore() {
    if (!this.vectorStore) {
      this.vectorStore = new SupabaseVectorStore({
        supabaseUrl: config.supabaseUrl,
        supabaseKey: config.supabaseKey,
        table: config.supabaseVectorTable,
      });
    }
    return this.vectorStore;
  }
  private async getStorageContext(): Promise<StorageContext> {
    return await storageContextFromDefaults({
      vectorStore: this.getVectorStore(),
    });
  }

  /**
   * Ingests docs content using the pipeline (fire and forget)
   */
  async ingestDocs(filePaths: string[], forceReingestion = false): Promise<void> {
    try {
      console.info(`Starting docs ingestion for ${filePaths.join(", ")}`);
      let cache: Record<string, boolean> = {};
      let cacheExists = false;
      try {
        await fs.access(PARSING_CACHE, fs.constants.F_OK);
        cacheExists = true;
      } catch {
        console.log("No cache found");
      }
      if (cacheExists) {
        cache = JSON.parse(await fs.readFile(PARSING_CACHE, "utf-8"));
      }

      const reader = new LlamaParseReader({
        apiKey: config.llamaCloudApiKey,
        resultType: "json",
        verbose: true
      });

      const documents: Document[] = [];
      for (const file of filePaths) {
        if (!cache[file] || forceReingestion) {
          if (forceReingestion && cache[file]) {
            console.log(`Force re-ingesting cached file: ${file}`);
          }
          console.log(`Processing uncached file: ${file}`);
          try {
            const jsonObjects = await reader.loadJson(file);

            for (const jsonObj of jsonObjects) {
              if (jsonObj.pages && Array.isArray(jsonObj.pages)) {
                for (const page of jsonObj.pages) {
                  if (page && page.md && page.page !== undefined) {
                    documents.push(
                      new Document({
                        text: page.md,
                        id_: `${file}_page_${page.page}`,
                        metadata: {
                          source_document: file,
                          page: page.page,
                        },
                      })
                    );
                  }
                }
              }
            }
            cache[file] = true;
          } catch (error) {
            const errorMessage =
              error instanceof Error
                ? error.message
                : String(error);
            console.error(`Error while parsing the file with: ${errorMessage}`);
            // Continue with next file
            continue;
          }
        } else {
          console.log(`Skipping cached file: ${file}`);
        }
      }

      await fs.writeFile(PARSING_CACHE, JSON.stringify(cache));
      console.info(
        `[LlamaService] Successfully loaded ${documents.length
        } document chunks from ${filePaths.join(", ")}`
      );

      if (documents.length > 0) {
        console.info(`Ingesting ${documents.length} document chunks`);

        try {
          // Run pipeline to process and embed documents
          const nodes = await this.pipeline.run({ documents });
          console.info(`Pipeline processed ${nodes.length} nodes`);

          // Explicitly create index to ensure vector store insertion
          const storageContext = await this.getStorageContext();
          await VectorStoreIndex.fromDocuments(documents, {
            storageContext,
          });
          console.info(`Successfully created index and inserted ${documents.length} documents into vector store`);
        } catch (error) {
          console.error("Error during ingestion pipeline:", error);
          throw error;
        }
      } else {
        console.info(`All docs files were already cached - skipping ingestion`);
      }

      console.info(
        `[LlamaService] Ingestion complete for ${filePaths.join(", ")}.`
      );
    } catch (error) {
      console.error(`Error ingesting docs for ${filePaths.join(", ")}:`, error);
    }
  }

  async createChatEngine(
    documents: string[],
    ctx: ChatMessage[]
  ): Promise<ContextChatEngine> {
    const index = await VectorStoreIndex.fromVectorStore(this.getVectorStore());

    const retriever = index.asRetriever({
      similarityTopK: 5,
      filters: {
        filters: [
          {
            key: "source_document",
            operator: FilterOperator.IN,
            value: documents,
          },
        ],
      },
    });
    const chatEngine = new ContextChatEngine({
      retriever,
      chatHistory: ctx || [],
      systemPrompt: RESEARCH_SYSTEM_PROMPT,
    });
    return chatEngine;
  }

  // tbd
  async createCodingAgent(): Promise<AgentWorkflow> {
    const index = await VectorStoreIndex.fromVectorStore(this.getVectorStore());
    const tools = [
      index.queryTool({
        metadata: {
          name: "rag_tool",
          description: `This tool can answer detailed questions about the paper.`,
        },
        options: { similarityTopK: 10 },
      }),
    ];
    const codingAgent = agent({
      name: "CodingAgent",
      description:
        "Responsible for implementing code based on research findings and technical requirements.",
      systemPrompt: `You are a coding agent. Your role is to implement code for technical concepts and algorithms based on detailed research findings, requirements, and instructions provided by the research agent. Always write correct, best practice, DRY, bug-free, and fully functional code.`,
      tools: [],
      llm: deepseek({
        model: config.deepSeekModel,
      })
    });

    const planningAgent = agent({
      name: "ResearchAgent",
      description:
        "Responsible for finding all information necessary for implementing technical concepts from research papers to code.",
      systemPrompt: `You are a research agent. Your role is to find and extract ALL information necessary for implementing technical concepts, algorithms, and methods from research papers and technical documents. Gather detailed, precise, and actionable insights required for accurate code implementation. Present your findings clearly for the coding agent to use directly in code generation.`,
      tools: tools,
      canHandoffTo: [codingAgent],
      llm: Settings.llm as ToolCallLLM,
    });

    const workflow = multiAgent({
      agents: [planningAgent, codingAgent],
      rootAgent: planningAgent, // The PlanningAgent starts the process
    });

    return workflow;
  }

  searchWeb = async (parameters: {
    query: string;
  }): Promise<{
    results: {
      url: string;
      title: string;
      content: string;
      raw_content?: string | undefined;
      score: string;
    }[];
  }> => {
    const tavily = new TavilyClient({ apiKey: config.tavilyApiKey });
    const response = await tavily.search({
      query: parameters.query,
      search_depth: "advanced",
      include_answer: true,
      include_images: true,
      max_results: 10,
    });
    return response;
  };

  retrieve = async (parameters: {
    documentPaths: string[];
    query: string;
  }): Promise<any> => {
    console.log("retrieve", parameters);
    const index = await VectorStoreIndex.fromVectorStore(this.getVectorStore());
    const retriever = index.asRetriever({
      similarityTopK: 3,
      // filters: {
      //   filters: [
      //     {
      //       key: "source_document",
      //       operator: "in",
      //       value: parameters.documentPaths,
      //     },
      //   ],
      // },
    });
    const nodes = await retriever.retrieve({ query: parameters.query });
    return nodes.map((node: NodeWithScore) => {
      return {
        content: (node.node as TextNode).text,
        metadata: node.node.metadata,
      };
    });
    // return results;
  };

  // deprecated, see agent.ts
  async createNexusAgent(documents: string[]): Promise<AgentWorkflow> {


    // using a queryTool(slower)
    // const retrieverTool = index.queryTool({
    //   metadata: {
    //     name: "retriever_tool",
    //     description: `This tool can retrieve information about the selected documents.`,
    //   },
    //   includeSourceNodes: true,
    //   options: { similarityTopK: 3,
    //     filters: {
    //     filters: [
    //       {
    //         key: "source_document",
    //         operator: "in",
    //         value: documents,
    //       },
    //     ],
    //   },
    //   },
    // });

    ////////////////////////////////////////////////////
    // using a bound retriever function to create a tool
    // const boundRetriever = (documents: string[]) => {
    //   return async (parameters: { query: string }) => {
    //     const nodes = await this.retrieve({
    //       query: parameters.query,
    //       documentPaths: documents,
    //     });
    //     return nodes.map((node: NodeWithScore) => {
    //       return {
    //         content: (node.node as TextNode).text,
    //         metadata: node.node.metadata,
    //       };
    //     });
    //   };
    // };
    const retrieverTool = tool({
      name: "retriever_tool",
      description: `This tool can retrieve detailed information from the selected documents.`,
      parameters: z.object({
        query: z.string().describe("The query to retrieve information from the document's vector embeddings."),
        documentPaths: z.array(z.string()).describe("The list of document paths to search in"),
      }),
      execute: this.retrieve,
    });
    ////////////////////////////////////////////////////

    const searchTool = tool({
      name: "search_web",
      description: "Search the web for information",
      parameters: z.object({
        query: z.string().describe("The search query optimized for web search"),
      }),
      execute: this.searchWeb,
    });

    const systemPrompt = AGENT_RESEARCH_PROMPT(documents);
    const NexusAgent = agent({
      name: "Nexus",
      description: "Responsible for overseeing the entire research process.",
      systemPrompt,
      tools: [retrieverTool, searchTool],
      llm: deepseek({
        model: config.deepSeekModel,
        temperature: 0.1,

      })
    });


    return NexusAgent;
  }


}

export const llamaService = LlamaService.getInstance();
