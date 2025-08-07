import { deepseek } from "@llamaindex/deepseek";
import {
  PGVectorStore,
  PostgresDocumentStore,
  PostgresIndexStore,
} from "@llamaindex/postgres";
import { agent, AgentWorkflow, multiAgent } from "@llamaindex/workflow";
import dotenv from "dotenv";
import fs from "fs/promises";
import {
  ChatMessage,
  ContextChatEngine,
  Document,
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
import { ClientConfig } from "pg";
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

- ALWAYS state to the user that you are going to use a tool before you call it..
- Example: "Let me look through the documents..." 
- Then proceed to use the appropriate tools to find information

### Tool Selection
- Prioritize the **retriever tool** for answering questions about ResilientDB and related blockchain topics
  - Refer to documents by their **title**, not by their file name 
- Use the **search tool** for web searches when needed

### Document Handling

AVAILABLE DOCUMENTS (documentPath - Title):
{
${
    documentPaths.map(docPath =>   `${docPath}": "${TITLE_MAPPINGS[docPath.replace("documents/", "")]}`).join(",\n")
}
}

#### Response Requirements
- **Always** base responses on information provided in the document when asked about document content, and from the web when needed.
- **Cite specific sections, pages, or other information** from the document/web results when possible
- **Clearly state** if specific information cannot be found in the document/web
- If asked about content NOT related to ResilientDB or related blockchain topics:
  - Give a brief answer
  - Guide the user to ask about something that IS related to ResilientDB or related blockchain topics.

### [IMPORTANT -- APPLIES ONLY FOR DOCUMENT RETRIEVALS] Citation Format
- Use [^id] format where id is the 1-based index of the source node
- Include **only** the citation markers - no additional citation explanations

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
  private vectorStore: PGVectorStore;
  private indexStore: PostgresIndexStore;
  private documentStore: PostgresDocumentStore;
  private pipeline: IngestionPipeline;
  private clientConfig: ClientConfig;
  private static instance: LlamaService;

  private constructor() {
    this.clientConfig = {
      connectionString: config.databaseUrl,
    };
    configureLlamaSettings();

    this.vectorStore = new PGVectorStore({
      clientConfig: this.clientConfig,
      performSetup: true,
      dimensions: config.embedDim,
    });

    this.indexStore = new PostgresIndexStore({
      clientConfig: this.clientConfig,
    });

    this.documentStore = new PostgresDocumentStore({
      clientConfig: this.clientConfig,
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
      // docStore: this.documentStore,
    });
  }

  public static getInstance(): LlamaService {
    if (!LlamaService.instance) {
      LlamaService.instance = new LlamaService();
    }
    return LlamaService.instance;
  }

  private getVectorStore() {
    if (!this.vectorStore) {
      this.vectorStore = new PGVectorStore({
        clientConfig: this.clientConfig,
        performSetup: true,
        dimensions: config.embedDim,
      });
    }
    return this.vectorStore;
  }

  private getIndexStore() {
    if (!this.indexStore) {
      this.indexStore = new PostgresIndexStore({
        clientConfig: this.clientConfig,
      });
    }
    return this.indexStore;
  }

  private getDocumentStore() {
    if (!this.documentStore) {
      this.documentStore = new PostgresDocumentStore({
        clientConfig: this.clientConfig,
      });
    }
    return this.documentStore;
  }
  private async getStorageContext(): Promise<StorageContext> {
    return await storageContextFromDefaults({
      vectorStore: this.getVectorStore(),
      indexStore: this.getIndexStore(),
      docStore: this.getDocumentStore(),
    });
  }

  /**
   * Ingests docs content using the pipeline (fire and forget)
   */
  async ingestDocs(filePaths: string[]): Promise<void> {
    try {
      console.info(`Starting docs ingestion for ${filePaths.join(", ")}`);
      let cache: Record<string, boolean> = {};
      let cacheExists = false;
      try {
        await fs.access(PARSING_CACHE, fs.constants.F_OK);
        cacheExists = true;
      } catch (e) {
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

      let documents: Document[] = [];
      for (let file of filePaths) {
        if (!cache[file]) {
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
          } catch (parseError) {
            const errorMessage =
              parseError instanceof Error
                ? parseError.message
                : String(parseError);
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
        `[LlamaService] Successfully loaded ${
          documents.length
        } document chunks from ${filePaths.join(", ")}`
      );

      if (documents.length > 0) {
        // console.log("Pipeline: ", this.pipeline);
        console.info(`Ingesting ${documents.length} docs files`);
        await this.pipeline.run({ documents });

        console.info("Awaiting VectorStoreIndex.fromDocuments");
        // await VectorStoreIndex.fromDocuments(documents);
        console.info(`Successfully ingested ${documents.length} docs files`);
      } else {
        console.info(`All docs files were already ingested`);
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
            operator: "in",
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

  // async createQueryEngine(
  //   documents: string[],
  //   tools?: any[]
  // ): Promise<RetrieverQueryEngine> {
  //   const storageContext = await this.getStorageContext();
  //   const index = await VectorStoreIndex.fromVectorStore(this.getVectorStore());

  //   const retriever = index.asRetriever({
  //     similarityTopK: 5,
  //     filters: {
  //       filters: [
  //         {
  //           key: "source_document",
  //           operator: "in",
  //           value: documents,
  //         },
  //       ],
  //     },
  //   });

  //   const queryEngine = index.asQueryEngine({
  //     retriever: retriever,
  //   });

  //   return queryEngine;
  // }

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
        model: "deepseek-chat",
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
    const tavily = new TavilyClient();
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
    const index = await VectorStoreIndex.fromVectorStore(this.getVectorStore());
    const retriever = index.retriever({
      similarityTopK: 3,
      filters: {
        filters: [
          {
            key: "source_document",
            operator: "in",
            value: parameters.documentPaths,
          },
        ],
      },
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

  async createNexusAgent(documents: string[]): Promise<AgentWorkflow> {
    const index = await VectorStoreIndex.fromVectorStore(this.getVectorStore());

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
        model: "deepseek-chat",
        temperature: 0.1,

      })
    });


    return NexusAgent;
  }


}

export const llamaService = LlamaService.getInstance();
