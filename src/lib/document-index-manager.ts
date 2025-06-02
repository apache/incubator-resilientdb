import { DeepSeekLLM } from '@llamaindex/deepseek';
import { mkdir, readFile, stat, writeFile } from "fs/promises";
import {
    Document,
    LlamaParseReader,
    Settings,
    VectorStoreIndex
} from 'llamaindex';
import { join } from "path";
import { config } from "../config/environment";

class DocumentIndexManager {
    private static instance: DocumentIndexManager;
    private documentIndices: Map<string, VectorStoreIndex> = new Map();

    private constructor() { }

    static getInstance(): DocumentIndexManager {
        if (!DocumentIndexManager.instance) {
            DocumentIndexManager.instance = new DocumentIndexManager();
        }
        return DocumentIndexManager.instance;
    }

    // helper function to get parsed file paths
    private getParsedPaths(documentPath: string) {
        const fileName = documentPath.split('/').pop()?.replace(/\.[^/.]+$/, "") || "document";
        const parsedDir = join(process.cwd(), "documents", "parsed", fileName);
        return {
            documentsPath: join(parsedDir, "documents.json"),
            metadataPath: join(parsedDir, "metadata.json"),
            parsedDir,
        };
    }

    // helper function to check if parsed files exist and are newer than source
    private async shouldUseParsedFiles(documentPath: string): Promise<boolean> {
        try {
            const sourcePath = join(process.cwd(), documentPath);
            const { documentsPath, metadataPath } = this.getParsedPaths(documentPath);

            // check if parsed files exist
            const [sourceStats, documentsStats, metadataStats] = await Promise.all([
                stat(sourcePath),
                stat(documentsPath),
                stat(metadataPath)
            ]);

            // check if parsed files are newer than source
            return documentsStats.mtime >= sourceStats.mtime && metadataStats.mtime >= sourceStats.mtime;
        } catch (error) {
            // if any file doesn't exist or we can't stat it, we should parse
            return false;
        }
    }

    // helper function to load documents from parsed files
    private async loadParsedDocuments(documentPath: string): Promise<Document[]> {
        const { documentsPath } = this.getParsedPaths(documentPath);

        try {
            const documentsData = await readFile(documentsPath, 'utf-8');
            const parsedDocuments = JSON.parse(documentsData);

            // convert to LlamaIndex Document format
            return parsedDocuments.map((doc: any) => new Document({
                id_: doc.id_,
                text: doc.text,
                metadata: doc.metadata || {}
            }));
        } catch (error) {
            console.error("Error loading parsed documents:", error);
            throw new Error("Failed to load pre-parsed documents");
        }
    }

    // helper function to save parsed documents
    private async saveParsedDocuments(documentPath: string, documents: Document[]): Promise<void> {
        try {
            const { parsedDir, documentsPath, metadataPath } = this.getParsedPaths(documentPath);

            // create directory if it doesn't exist
            await mkdir(parsedDir, { recursive: true });

            // prepare documents data
            const documentsData = documents.map(doc => ({
                id_: doc.id_,
                text: doc.text,
                metadata: doc.metadata
            }));

            // get source file stats for metadata
            const sourceStats = await stat(join(process.cwd(), documentPath));
            const metadata = {
                documentPath,
                originalFileSize: sourceStats.size,
                originalModifiedTime: sourceStats.mtime.toISOString(),
                cachedAt: new Date().toISOString(),
                documentsCount: documents.length
            };

            // save both files
            await Promise.all([
                writeFile(documentsPath, JSON.stringify(documentsData, null, 2)),
                writeFile(metadataPath, JSON.stringify(metadata, null, 2))
            ]);

            console.log(`Saved parsed documents to ${parsedDir}`);
        } catch (error) {
            console.error("Error saving parsed documents:", error);
            // don't throw here as this is a optimization, not critical
        }
    }

    // helper function to parse document and save results
    private async parseAndSaveDocuments(documentPath: string): Promise<Document[]> {
        // parse the document using LlamaParse
        const parser = new LlamaParseReader({
            apiKey: config.llamaCloudApiKey,
            resultType: "text", // simple text extraction
        });

        const filePath = join(process.cwd(), documentPath);
        const documents = await parser.loadData(filePath);

        // save parsed documents for future use
        await this.saveParsedDocuments(documentPath, documents);

        return documents;
    }

    // public method to prepare an index for a document
    async prepareIndex(documentPath: string): Promise<void> {
        // configure DeepSeek LLM
        Settings.llm = new DeepSeekLLM({
            apiKey: config.deepSeekApiKey,
            model: config.deepSeekModel,
        });

        // check if we already have an index for this document
        let documentIndex = this.documentIndices.get(documentPath);

        if (!documentIndex) {
            console.log(`Creating new index for document: ${documentPath}`);

            let documents: Document[];

            // check if we can use pre-parsed files
            if (await this.shouldUseParsedFiles(documentPath)) {
                console.log(`Loading pre-parsed documents for: ${documentPath}`);
                try {
                    documents = await this.loadParsedDocuments(documentPath);
                    console.log(`Successfully loaded ${documents.length} pre-parsed documents`);
                } catch (error) {
                    console.error("Failed to load pre-parsed documents, falling back to parsing:", error);
                    documents = await this.parseAndSaveDocuments(documentPath);
                }
            } else {
                console.log(`No valid pre-parsed files found, parsing document: ${documentPath}`);
                documents = await this.parseAndSaveDocuments(documentPath);
            }

            if (!documents || documents.length === 0) {
                throw new Error("No content could be extracted from the document");
            }

            // create vector index from documents
            documentIndex = await VectorStoreIndex.fromDocuments(documents);
            this.documentIndices.set(documentPath, documentIndex);

            console.log(`Successfully created index for ${documentPath}`);
        } else {
            console.log(`Index already exists for ${documentPath}`);
        }
    }

    // public method to get an index for a document
    async getIndex(documentPath: string): Promise<VectorStoreIndex | undefined> {
        let documentIndex = this.documentIndices.get(documentPath);

        if (!documentIndex) {
            console.log(`Index not found in memory for ${documentPath}. Attempting to rebuild from cache.`);
            // Attempt to load from parsed files if not in memory
            if (await this.shouldUseParsedFiles(documentPath)) {
                console.log(`Loading pre-parsed documents for rebuilding index: ${documentPath}`);
                try {
                    const documents = await this.loadParsedDocuments(documentPath);
                    if (documents && documents.length > 0) {
                        // Re-initialize LLM settings as they might be needed for index creation
                        Settings.llm = new DeepSeekLLM({
                            apiKey: config.deepSeekApiKey,
                            model: config.deepSeekModel,
                        });
                        documentIndex = await VectorStoreIndex.fromDocuments(documents);
                        this.documentIndices.set(documentPath, documentIndex);
                        console.log(`Successfully rebuilt and cached index for ${documentPath} from parsed files.`);
                    } else {
                        console.log(`No documents found in parsed files for ${documentPath}. Cannot rebuild index.`);
                    }
                } catch (error) {
                    console.error(`Error rebuilding index from parsed files for ${documentPath}:`, error);
                    // If rebuilding fails, we still return undefined, or handle error as appropriate
                }
            } else {
                console.log(`No valid pre-parsed files found for ${documentPath}. Cannot rebuild index.`);
            }
        }
        return documentIndex;
    }

    // public method to check if an index exists
    hasIndex(documentPath: string): boolean {
        return this.documentIndices.has(documentPath);
    }
}

// export singleton instance
export const documentIndexManager = DocumentIndexManager.getInstance(); 