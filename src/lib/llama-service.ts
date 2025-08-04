import { configureLlamaSettings } from "@/lib/config/llama-settings";
import { HuggingFaceEmbedding } from "@llamaindex/huggingface";
import { PGVectorStore, PostgresDocumentStore, PostgresIndexStore } from "@llamaindex/postgres";
import dotenv from "dotenv";
import fs from "fs/promises";
import { ContextChatEngine, Document, IngestionPipeline, LlamaParseReader, MarkdownNodeParser, SentenceSplitter, StorageContext, storageContextFromDefaults, SummaryExtractor, VectorStoreIndex } from "llamaindex";
import { ClientConfig } from "pg";
import { config } from "../config/environment";

dotenv.config();

const RESEARCH_SYSTEM_PROMPT = `
You are Nexus, an AI research assistant specialized in Apache ResilientDB and its related blockchain technology, distributed systems, and fault-tolerant consensus protocols. Your primary role is to help students, researchers, and practitioners understand complex technical concepts related to Apache ResilientDB and blockchain systems, who can answer questions about documents. 
You have access to the content of a document and can provide accurate, detailed answers based on that content.
When asked about the document, always base your responses on the information provided in the document. When possible, cite sections, pages, or other specific information from the document.
If you cannot find specific information in the document, say so clearly.
Please favor referring to the document by its title, instead of the file name.
If asked about something that is not in the document, give a brief answer and try to guide the user to ask about something that is in the document.
`;
const PARSING_CACHE = "parsing-cache.json";



export class LlamaService {
    private vectorStore: PGVectorStore;
    private indexStore: PostgresIndexStore;
    private documentStore: PostgresDocumentStore;
    private pipeline: IngestionPipeline;
    private selectedDocuments: string[] = [];
    private clientConfig: ClientConfig;
    private static instance: LlamaService;

    private constructor() {

        this.clientConfig = {
            connectionString: process.env.DATABASE_URL,
        };
        configureLlamaSettings();

        this.vectorStore = new PGVectorStore({
            clientConfig: this.clientConfig,
            performSetup: true,
            dimensions: config.vectorStore.embedDim,
        });

        this.indexStore = new PostgresIndexStore({
            clientConfig: this.clientConfig,
        });

        this.documentStore = new PostgresDocumentStore({
            clientConfig: this.clientConfig,
        });
        const huggingFaceEmbedding = new HuggingFaceEmbedding({
            modelType: "BAAI/bge-large-en-v1.5",
            modelOptions: {
                dtype: "auto",
              },
        })
        this.pipeline = new IngestionPipeline({
            transformations: [
                new MarkdownNodeParser(),   
                // look into using MarkdownElementNodeParser for getting table data
                new SentenceSplitter({    
                    chunkSize: 1024,
                    chunkOverlap: 20
                }),
                new SummaryExtractor(),
                huggingFaceEmbedding,
            ],
            vectorStore: this.vectorStore,
            docStore: this.documentStore,
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
              dimensions: config.vectorStore.embedDim,
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
        })

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
                                    documents.push(new Document({ 
                                        text: page.md, 
                                        id_: `${file}_page_${page.page}`,
                                        metadata: { 
                                            source_document: file,
                                            page: page.page 
                                        }
                                    }));
                                }
                            }
                        }
                    }
                    cache[file] = true;
                } catch (parseError) {
                    const errorMessage = parseError instanceof Error ? parseError.message : String(parseError);
                    console.error(`Error while parsing the file with: ${errorMessage}`);
                    // Continue with next file
                    continue;
                }
            } else {
                console.log(`Skipping cached file: ${file}`);
            }
        }
        
        await fs.writeFile(PARSING_CACHE, JSON.stringify(cache));
        console.info(`[LlamaService] Successfully loaded ${documents.length} document chunks from ${filePaths.join(", ")}`);


        if (documents.length > 0) {
            // console.log("Pipeline: ", this.pipeline);
            console.info(`Ingesting ${documents.length} docs files`);
            await this.pipeline.run({ documents });

            console.info('Awaiting VectorStoreIndex.fromDocuments');
            // await VectorStoreIndex.fromDocuments(documents);
            console.info(`Successfully ingested ${documents.length} docs files`);
          } else {
            console.info(`All docs files were already ingested`);
          }

        console.info(`[LlamaService] Ingestion complete for ${filePaths.join(", ")}.`);
    } catch (error) {
        console.error(`Error ingesting docs for ${filePaths.join(", ")}:`, error);
    }
  }

  async createChatEngine(documents: string[]): Promise<ContextChatEngine> {
    const storageContext = await this.getStorageContext();
    // const chatHistory = createMemory([
    //     {
    //       content: "You are a helpful assistant named Nexus.",
    //       role: "system",
    //     },
    //   ]);
      const index = await VectorStoreIndex.fromVectorStore(this.getVectorStore());

    const retriever = index.asRetriever({
        similarityTopK: 5,
        
        filters: {
            filters: [{
                key: "source_document",
                operator: "in",
                value: documents,
            }]
        }
    });
      const chatEngine = new ContextChatEngine({
        retriever: retriever,
        systemPrompt: RESEARCH_SYSTEM_PROMPT,
      });
      return chatEngine;
  } 
  
// async createQueryEngine(documents: string[]): Promise<any> {
//     const storageContext = await this.getStorageContext();
//     const index = await VectorStoreIndex.fromVectorStore(this.getVectorStore());

//     const retriever = index.asRetriever({
//         similarityTopK: 5,
//         filters: {
//             filters: [{
//                 key: "source_document",
//                 operator: "in",
//                 value: documents,
//             }]
//         }
//     });

//     const queryEngine = index.asQueryEngine({
//         retriever: retriever,
//     });
    
//     return queryEngine;
//   }

}

export const llamaService = LlamaService.getInstance();