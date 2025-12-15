/*
* Licensed to the Apache Software Foundation (ASF) under one
* or more contributor license agreements.  See the NOTICE file
* distributed with this work for additional information
* regarding copyright ownership.  The ASF licenses this file
* to you under the Apache License, Version 2.0 (the
* "License"); you may not use this file except in compliance
* with the License.  You may obtain a copy of the License at
* 
*   http://www.apache.org/licenses/LICENSE-2.0
* 
* Unless required by applicable law or agreed to in writing,
* software distributed under the License is distributed on an
* "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
* KIND, either express or implied.  See the License for the
* specific language governing permissions and limitations
* under the License.
*/

import { SupabaseVectorStore } from "@llamaindex/supabase";
import { createClient, SupabaseClient } from "@supabase/supabase-js";
import dotenv from "dotenv";
import {
  Document,
  IngestionPipeline,
  LlamaParseReader,
  MarkdownNodeParser,
  NodeWithScore,
  SentenceSplitter,
  Settings,
  SimilarityPostprocessor,
  StorageContext,
  storageContextFromDefaults,
  SummaryExtractor,
  TextNode,
  VectorStoreIndex
} from "llamaindex";
import { TavilyClient } from "tavily";
import { config } from "../config/environment";
import { configureLlamaSettings } from "./config/llama-settings";

dotenv.config();

export const AGENT_RESEARCH_PROMPT = `
## Core Identity
You are **Nexus**, an AI research assistant specialized in Apache ResilientDB and its related blockchain technology, distributed systems, and fault-tolerant consensus protocols. Your primary role is to help students, researchers, and practitioners understand complex technical concepts related to Apache ResilientDB and blockchain systems.

## Capabilities
- Answer questions about documents with access to document content
- Provide accurate, detailed answers based on document content
- Explain complex technical concepts in blockchain and distributed systems
- Assist with Apache ResilientDB research and implementation

## Operational Guidelines

- For most questions, you should use the **search_documents tool** to answer questions, even if the question is not about ResilientDB or related blockchain topics.
- The user has selected specific documents to search through. Use the search_documents tool with the document paths provided to you.
- If uncertain about which document to search, search all available documents that were passed to you.
- ALWAYS state to the user that you are going to use a tool before you call it.
- Example: "Let me look through the documents..." 
- Then proceed to use the appropriate tools to find information

### Tool Selection
- **Web Search Priority**: If the user's query contains "[Web Search Priority]" at the start, prioritize using the **search_web tool** first to find current information from the internet, then use **search_documents** if needed for additional context from selected documents.
- **Default Behavior**: Prioritize the **search_documents tool** for answering questions.
  - Refer to documents by their **title**, not by their file name 
- Use the **search_web tool** for web searches when documents don't contain the needed information or for current/recent information

### Document Handling

- The search_documents tool will be called with specific document paths selected by the user
- You can search one or multiple documents depending on the query
- The tool parameters will specify which documents to search


### CRITICAL TOOL USAGE REQUIREMENT
**YOU MUST MAKE A SEPARATE TOOL CALL FOR EACH DOCUMENT YOU EXAMINE.**

- **DO NOT** pass multiple documents in a single tool call
- **ALWAYS** make individual tool calls, one per document
- When searching multiple documents, make sequential separate tool calls for each document path
- Example of CORRECT behavior:
  - Tool call 1: search_documents with documentPaths: ["documents/document1.pdf"]
  - Tool call 2: search_documents with documentPaths: ["documents/document2.pdf"]
  - Tool call 3: search_documents with documentPaths: ["documents/document3.pdf"]
- Example of INCORRECT behavior:
  - Tool call 1: search_documents with documentPaths: ["documents/document1.pdf", "documents/document2.pdf", "documents/document3.pdf"]

This ensures thorough examination of each document and proper tracking of information sources.

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

export class LlamaService {
  private vectorStore: SupabaseVectorStore;
  private pipeline: IngestionPipeline;
  private supabase: SupabaseClient;
  private static instance: LlamaService;

  private constructor() {
    configureLlamaSettings();

    console.info(`[LlamaService] Initializing SupabaseVectorStore with table: ${config.supabaseVectorTable}`);

    if (!config.supabaseUrl || !config.supabaseKey) {
      throw new Error("Supabase URL and key are required for vector store initialization");
    }

    this.supabase = createClient(config.supabaseUrl, config.supabaseKey);

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
      const { error } = await this.supabase
        .from('parsed_documents')
        .delete()
        .neq('id', '00000000-0000-0000-0000-000000000000'); // Delete all records
      
      if (error) {
        console.error("[LlamaService] Error clearing cache:", error);
        throw error;
      }
      
      console.info("[LlamaService] Parsing cache cleared from database");
    } catch (error) {
      console.error("[LlamaService] Failed to clear cache:", error);
    }
  }

  /**
   * Check if a file has been successfully parsed
   */
  private async isFileParsed(filePath: string): Promise<boolean> {
    try {
      const { data, error } = await this.supabase
        .from('parsed_documents')
        .select('parsing_status')
        .eq('file_path', filePath)
        .eq('parsing_status', 'success')
        .single();
      
      if (error && error.code !== 'PGRST116') { // PGRST116 is "not found" error
        console.error(`[LlamaService] Error checking cache for ${filePath}:`, error);
        return false;
      }
      
      return data !== null;
    } catch (error) {
      console.error(`[LlamaService] Error checking cache for ${filePath}:`, error);
      return false;
    }
  }

  /**
   * Mark a file as successfully parsed
   */
  private async markFileParsed(filePath: string, pagesCount: number, chunksCount: number): Promise<void> {
    try {
      const { error } = await this.supabase
        .from('parsed_documents')
        .upsert({
          file_path: filePath,
          parsing_status: 'success',
          pages_count: pagesCount,
          chunks_count: chunksCount,
          parsed_at: new Date().toISOString(),
        });
      
      if (error) {
        console.error(`[LlamaService] Error marking file as parsed ${filePath}:`, error);
        throw error;
      }
    } catch (error) {
      console.error(`[LlamaService] Error marking file as parsed ${filePath}:`, error);
    }
  }

  /**
   * Mark a file as failed to parse
   */
  private async markFileFailed(filePath: string, errorMessage: string): Promise<void> {
    try {
      const { error } = await this.supabase
        .from('parsed_documents')
        .upsert({
          file_path: filePath,
          parsing_status: 'failed',
          parsing_error: errorMessage,
          parsed_at: new Date().toISOString(),
        });
      
      if (error) {
        console.error(`[LlamaService] Error marking file as failed ${filePath}:`, error);
      }
    } catch (error) {
      console.error(`[LlamaService] Error marking file as failed ${filePath}:`, error);
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
   * Create a similarity postprocessor with 0.5 cutoff
   */
  private createSimilarityPostprocessor(): SimilarityPostprocessor {
    return new SimilarityPostprocessor({
      similarityCutoff: 0.6,
    });
  }

  /**
   * Ingests docs content using the pipeline (fire and forget)
   */
  async ingestDocs(filePaths: string[], forceReingestion = false): Promise<void> {
    try {
      console.info(`Starting docs ingestion for ${filePaths.join(", ")}`);

      const reader = new LlamaParseReader({
        apiKey: config.llamaCloudApiKey,
        resultType: "json",
        verbose: true
      });

      const documents: Document[] = [];
      
      for (const file of filePaths) {
        console.log(`Processing file: ${file}`);

        // Check if file is already parsed (skip in Vercel environment)
        const isAlreadyParsed = await this.isFileParsed(file);
        
        if (!process.env.VERCEL && (!isAlreadyParsed || forceReingestion)) {
          if (forceReingestion && isAlreadyParsed) {
            console.log(`Force re-ingesting cached file: ${file}`);
          }
          console.log(`Processing uncached file: ${file}`);
          
          let pagesCount = 0;
          let chunksCount = 0;
          
          try {
            const jsonObjects = await reader.loadJson(file);

            for (const jsonObj of jsonObjects) {
              if (jsonObj.pages && Array.isArray(jsonObj.pages)) {
                pagesCount = jsonObj.pages.length;
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
                    chunksCount++;
                  }
                }
              }
            }
            
            // Mark file as successfully parsed
            await this.markFileParsed(file, pagesCount, chunksCount);
            
          } catch (error) {
            const errorMessage =
              error instanceof Error
                ? error.message
                : String(error);
            console.error(`Error while parsing the file with: ${errorMessage}`);
            
            // Mark file as failed
            await this.markFileFailed(file, errorMessage);
            
            // Continue with next file
            continue;
          }
        } else {
          console.log(`Skipping cached file: ${file}`);
        }
      }

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

  // async createChatEngine(
  //   documents: string[],
  //   ctx: ChatMessage[]
  // ): Promise<ContextChatEngine> {
  //   const index = await VectorStoreIndex.fromVectorStore(this.getVectorStore());

  //   // TODO: IN operator doesn't work with Supabase, so we'll use EQ operator for now
  //   // const retriever = index.asRetriever({
  //   //   similarityTopK: 5,
  //   //   filters: {
  //   //     filters: [
  //   //       {
  //   //         key: "source_document",
  //   //         operator: FilterOperator.IN,
  //   //         value: documents,
  //   //       },
  //   //     ],
  //   //   },
  //   // });
  //   //
  //   // Create a custom retriever that handles multiple documents with EQ operator
  //   const customRetriever = {
  //     retrieve: async ({ query }: { query: string }) => {
  //       const allNodes: NodeWithScore[] = [];
  //       const postprocessor = this.createSimilarityPostprocessor();
        
  //       for (const docPath of documents) {
  //         const retriever = index.asRetriever({
  //           similarityTopK: 3, // Get results per document
  //           filters: {
  //             filters: [
  //               {
  //                 key: "source_document",
  //                 operator: "==",
  //                 value: docPath,
  //               },
  //             ],
  //           },
  //         });
          
  //         const nodes = await retriever.retrieve({ query });
  //         allNodes.push(...nodes);
  //       }
        
  //       // Apply similarity postprocessor to filter low-quality results
  //       return await postprocessor.postprocessNodes(allNodes);
  //     }
  //   };

  //   const chatEngine = new ContextChatEngine({
  //     retriever: customRetriever as any,
  //     chatHistory: ctx || [],
  //     systemPrompt: RESEARCH_SYSTEM_PROMPT,
  //   });
  //   return chatEngine;
  // }

  // // tbd
  // async createCodingAgent(): Promise<AgentWorkflow> {
  //   const index = await VectorStoreIndex.fromVectorStore(this.getVectorStore());
  //   const tools = [
  //     index.queryTool({
  //       metadata: {
  //         name: "rag_tool",
  //         description: `This tool can answer detailed questions about the paper.`,
  //       },
  //       options: { similarityTopK: 10 },
  //     }),
  //   ];
  //   const codingAgent = agent({
  //     name: "CodingAgent",
  //     description:
  //       "Responsible for implementing code based on research findings and technical requirements.",
  //     systemPrompt: `You are a coding agent. Your role is to implement code for technical concepts and algorithms based on detailed research findings, requirements, and instructions provided by the research agent. Always write correct, best practice, DRY, bug-free, and fully functional code.`,
  //     tools: [],
  //     llm: deepseek({
  //       model: config.deepSeekModel,
  //     })
  //   });

  //   const planningAgent = agent({
  //     name: "ResearchAgent",
  //     description:
  //       "Responsible for finding all information necessary for implementing technical concepts from research papers to code.",
  //     systemPrompt: `You are a research agent. Your role is to find and extract ALL information necessary for implementing technical concepts, algorithms, and methods from research papers and technical documents. Gather detailed, precise, and actionable insights required for accurate code implementation. Present your findings clearly for the coding agent to use directly in code generation.`,
  //     tools: tools,
  //     canHandoffTo: [codingAgent],
  //     llm: Settings.llm as ToolCallLLM,
  //   });

  //   const workflow = multiAgent({
  //     agents: [planningAgent, codingAgent],
  //     rootAgent: planningAgent, // The PlanningAgent starts the process
  //   });

  //   return workflow;
  // }

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
    
    // Since IN operator doesn't work with Supabase, retrieve from each document separately
    const allNodes: NodeWithScore[] = [];
    const postprocessor = this.createSimilarityPostprocessor();
    
    for (const docPath of parameters.documentPaths) {
      const retriever = index.asRetriever({
        similarityTopK: 5, // Get more results per document
        filters: {
          filters: [
            {
              key: "source_document",
              operator: "==",
              value: docPath,
            },
          ],
        },
      });
      
      const nodes = await retriever.retrieve({ query: parameters.query });
      allNodes.push(...nodes);
    }
    
    // Apply similarity postprocessor to filter low-quality results
    const filteredNodes = await postprocessor.postprocessNodes(allNodes);
    
    console.log("nodes", filteredNodes.map((node: NodeWithScore) => node.node.metadata));
    return filteredNodes.map((node: NodeWithScore) => {
      return {
        content: (node.node as TextNode).text,
        metadata: node.node.metadata,
      };
    });
  };

  // // deprecated, see agent.ts
  // async createNexusAgent(documents: string[]): Promise<AgentWorkflow> {


  //   // using a queryTool(slower)
  //   // const retrieverTool = index.queryTool({
  //   //   metadata: {
  //   //     name: "retriever_tool",
  //   //     description: `This tool can retrieve information about the selected documents.`,
  //   //   },
  //   //   includeSourceNodes: true,
  //   //   options: { similarityTopK: 3,
  //   //     filters: {
  //   //     filters: [
  //   //       {
  //   //         key: "source_document",
  //   //         operator: "in",
  //   //         value: documents,
  //   //       },
  //   //     ],
  //   //   },
  //   //   },
  //   // });

  //   ////////////////////////////////////////////////////
  //   // using a bound retriever function to create a tool
  //   // const boundRetriever = (documents: string[]) => {
  //   //   return async (parameters: { query: string }) => {
  //   //     const nodes = await this.retrieve({
  //   //       query: parameters.query,
  //   //       documentPaths: documents,
  //   //     });
  //   //     return nodes.map((node: NodeWithScore) => {
  //   //       return {
  //   //         content: (node.node as TextNode).text,
  //   //         metadata: node.node.metadata,
  //   //       };
  //   //     });
  //   //   };
  //   // };
  //   const retrieverTool = tool({
  //     name: "retriever_tool",
  //     description: `This tool can retrieve detailed information from the selected documents.`,
  //     parameters: z.object({
  //       query: z.string().describe("The query to retrieve information from the document's vector embeddings."),
  //       documentPaths: z.array(z.string()).describe("The list of document paths to search in"),
  //     }),
  //     execute: this.retrieve,
  //   });
  //   ////////////////////////////////////////////////////

  //   const searchTool = tool({
  //     name: "search_web",
  //     description: "Search the web for information",
  //     parameters: z.object({
  //       query: z.string().describe("The search query optimized for web search"),
  //     }),
  //     execute: this.searchWeb,
  //   });

  //   const systemPrompt = AGENT_RESEARCH_PROMPT(documents);
  //   const NexusAgent = agent({
  //     name: "Nexus",
  //     description: "Responsible for overseeing the entire research process.",
  //     systemPrompt,
  //     tools: [retrieverTool, searchTool],
  //     llm: deepseek({
  //       model: config.deepSeekModel,
  //       temperature: 0.1,

  //     })
  //   });


  //   return NexusAgent;
  // }


}

export const llamaService = LlamaService.getInstance();
