import { stat } from "fs/promises";
import {
  Document,
  LlamaParseReader,
  VectorStoreIndex
} from "llamaindex";
import { join } from "path";
import { config } from "../config/environment";

import chalk from "chalk";
import { databaseService, DocumentData } from "./database";

class DocumentIndexManager {
  private static instance: DocumentIndexManager;
  private documentIndices: Map<string, VectorStoreIndex> = new Map();
  private databaseInitialized: boolean = false;

  private constructor() {}

  static getInstance(): DocumentIndexManager {
    if (!DocumentIndexManager.instance) {
      DocumentIndexManager.instance = new DocumentIndexManager();
    }
    return DocumentIndexManager.instance;
  }

  private configureSettings(): void {
    // Settings should be configured at application startup
    // No need to configure here as it's already done globally
  }

  // Initialize database if not already done
  private async initializeDatabase(): Promise<void> {
    if (!this.databaseInitialized) {
      await databaseService.initializeSchema();
      this.databaseInitialized = true;
    }
  }

  // helper function to check if parsed documents exist and are newer than source
  private async shouldUseParsedDocuments(documentPath: string): Promise<boolean> {
    try {
      await this.initializeDatabase();

      const sourcePath = join(process.cwd(), documentPath);
      
      // get source file stats
      const sourceStats = await stat(sourcePath);

      // check if we have cached data that's newer than source
      return await databaseService.shouldUseParsedDocuments(documentPath, sourceStats.mtime);
    } catch (error) {
      // if any error occurs, we should re-parse
      console.error(chalk.red(`[DocumentIndexManager] Error checking parsed documents validity: ${error}`));
      return false;
    }
  }

  // helper function to load documents from database
  private async loadParsedDocuments(documentPath: string): Promise<Document[]> {
    try {
      await this.initializeDatabase();

      const documentsData = await databaseService.loadParsedDocuments(documentPath);

      // convert to LlamaIndex Document format
      return documentsData.map(
        (doc: DocumentData) =>
          new Document({
            id_: doc.id_,
            text: doc.text,
            metadata: doc.metadata || {},
          }),
      );
    } catch (error) {
      console.error(chalk.red(`[DocumentIndexManager] Error loading parsed documents: ${error}`));
      throw new Error("Failed to load pre-parsed documents");
    }
  }

  // helper function to save parsed documents to database
  private async saveParsedDocuments(
    documentPath: string,
    documents: Document[],
  ): Promise<void> {
    try {
      await this.initializeDatabase();

      // prepare documents data
      const documentsData: DocumentData[] = documents.map((doc) => ({
        id_: doc.id_,
        text: doc.text,
        metadata: doc.metadata,
      }));

      // get source file stats for metadata
      const sourcePath = join(process.cwd(), documentPath);
      const sourceStats = await stat(sourcePath);

      await databaseService.saveParsedDocuments(
        documentPath,
        documentsData,
        sourceStats.size,
        sourceStats.mtime
      );

      console.log(chalk.gray(`[DocumentIndexManager] Saved parsed documents to database for: ${documentPath}`));
    } catch (error) {
      console.error(chalk.red(`[DocumentIndexManager] Error saving parsed documents: ${error}`));
      // don't throw here as this is an optimization, not critical
    }
  }

  // helper function to parse document and save results
  private async parseAndSaveDocuments(
    documentPath: string,
  ): Promise<Document[]> {
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
    this.configureSettings();
    await this.initializeDatabase();

    // check if we already have an index for this document
    let documentIndex = this.documentIndices.get(documentPath);

    if (!documentIndex) {
      console.log(chalk.gray(`[DocumentIndexManager] Creating new index for document: ${documentPath}`));

      let documents: Document[];

      // check if we can use pre-parsed documents
      if (await this.shouldUseParsedDocuments(documentPath)) {
        console.log(chalk.gray(`[DocumentIndexManager] Loading pre-parsed documents for: ${documentPath}`));
        try {
          documents = await this.loadParsedDocuments(documentPath);
          console.log(
            chalk.gray(`[DocumentIndexManager] Successfully loaded ${documents.length} pre-parsed documents`),
          );
        } catch (error) {
          console.error(
            chalk.red(`[DocumentIndexManager] Failed to load pre-parsed documents, falling back to parsing: ${error}`),
            error,
          );
          documents = await this.parseAndSaveDocuments(documentPath);
        }
      } else {
        console.log(
          chalk.gray(`[DocumentIndexManager] No valid pre-parsed documents found, parsing document: ${documentPath}`),
        );
        documents = await this.parseAndSaveDocuments(documentPath);
      }

      if (!documents || documents.length === 0) {
        throw new Error(chalk.red("[DocumentIndexManager] No content could be extracted from the document"));
      }

      // create vector index from documents
      documentIndex = await VectorStoreIndex.fromDocuments(documents);
      this.documentIndices.set(documentPath, documentIndex);

      console.log(chalk.gray(`[DocumentIndexManager] Successfully created index for ${documentPath}`));
    } else {
      console.log(chalk.gray(`[DocumentIndexManager] Index already exists for ${documentPath}`));
    }
  }

  // public method to get an index for a document
  async getIndex(documentPath: string): Promise<VectorStoreIndex | undefined> {
    let documentIndex = this.documentIndices.get(documentPath);

    if (!documentIndex) {
      console.log(
        chalk.gray(`[DocumentIndexManager] Index not found in memory for ${documentPath}. Attempting to rebuild from cache.`),
      );
      if (await this.shouldUseParsedDocuments(documentPath)) {
        console.log(
          chalk.gray(`[DocumentIndexManager] Loading pre-parsed documents for rebuilding index: ${documentPath}`),
        );
        try {
          const documents = await this.loadParsedDocuments(documentPath);
          if (documents && documents.length > 0) {
            this.configureSettings();

            documentIndex = await VectorStoreIndex.fromDocuments(documents);
            this.documentIndices.set(documentPath, documentIndex);
            console.log(
              chalk.gray(`[DocumentIndexManager] Successfully rebuilt and cached index for ${documentPath} from parsed documents.`),
            );
          } else {
            console.log(
              chalk.gray(`[DocumentIndexManager] No documents found in parsed data for ${documentPath}. Cannot rebuild index.`),
            );
          }
        } catch (error) {
          console.error(
            chalk.red(`[DocumentIndexManager] Error rebuilding index from parsed documents for ${documentPath}:`),
            error,
          );
        }
      } else {
        console.log(
          chalk.gray(`[DocumentIndexManager] No valid pre-parsed documents found for ${documentPath}. Cannot rebuild index.`),
        );
      }
    }
    return documentIndex;
  }

  // public method to check if an index exists
  hasIndex(documentPath: string): boolean {
    return this.documentIndices.has(documentPath);
  }

  // Multi-document support methods

  // prepare indices for multiple documents
  async prepareMultipleIndices(documentPaths: string[]): Promise<void> {
    console.log(chalk.gray(`[DocumentIndexManager] Preparing indices for ${documentPaths.length} documents`));

    // prepare indices in parallel for better performance
    const preparePromises = documentPaths.map((documentPath) =>
      this.prepareIndex(documentPath),
    );

    await Promise.all(preparePromises);
    console.log(
      chalk.gray(`[DocumentIndexManager] Successfully prepared indices for all ${documentPaths.length} documents`),
    );
  }

  // get indices for multiple documents
  async getMultipleIndices(
    documentPaths: string[],
  ): Promise<Map<string, VectorStoreIndex>> {
    const indices = new Map<string, VectorStoreIndex>();

    for (const documentPath of documentPaths) {
      const index = await this.getIndex(documentPath);
      if (index) {
        indices.set(documentPath, index);
      }
    }

    return indices;
  }

  // get a combined index from multiple documents
  async getCombinedIndex(
    documentPaths: string[],
  ): Promise<VectorStoreIndex | undefined> {
    console.log(
      chalk.gray(`[DocumentIndexManager] Creating combined index from ${documentPaths.length} documents`),
    );

    // collect all documents from all indices by loading from database
    const allDocuments: Document[] = [];

    for (const documentPath of documentPaths) {
      try {
        // Load documents directly from database instead of trying to extract from index
        const documents = await this.loadParsedDocuments(documentPath);
        const documentsWithSource = documents.map(
          (doc) =>
            new Document({
              id_: doc.id_,
              text: doc.text,
              metadata: {
                ...doc.metadata,
                source_document: documentPath, // add source attribution
              },
            }),
        );
        allDocuments.push(...documentsWithSource);
        console.log(
          chalk.gray(`[DocumentIndexManager] Loaded ${documents.length} documents from ${documentPath}`),
        );
      } catch (error) {
        console.error(chalk.red(`[DocumentIndexManager] Failed to load documents for ${documentPath}:`), error);
        // If we can't load from database, we can't create a combined index
        continue;
      }
    }

    if (allDocuments.length === 0) {
      console.warn("No documents found for combined index");
      return undefined;
    }

    this.configureSettings();

    // create a new combined index
    const combinedIndex = await VectorStoreIndex.fromDocuments(allDocuments);
    console.log(
      chalk.gray(`[DocumentIndexManager] Successfully created combined index with ${allDocuments.length} documents`),
    );

    return combinedIndex;
  }

  // check if all indices exist for given document paths
  hasAllIndices(documentPaths: string[]): boolean {
    return documentPaths.every((path) => this.hasIndex(path));
  }

  // get all available indices
  getAllAvailableIndices(): Map<string, VectorStoreIndex> {
    return new Map(this.documentIndices);
  }

  // get all indexed document paths
  getIndexedDocumentPaths(): string[] {
    return Array.from(this.documentIndices.keys());
  }

  // get all document paths stored in database
  async getAllStoredDocumentPaths(): Promise<string[]> {
    try {
      await this.initializeDatabase();
      return await databaseService.getAllStoredDocumentPaths();
    } catch (error) {
      console.error("Error getting all stored document paths:", error);
      return [];
    }
  }

  // clear specific indices (useful for memory management)
  clearIndices(documentPaths: string[]): void {
    documentPaths.forEach((path) => {
      if (this.documentIndices.has(path)) {
        this.documentIndices.delete(path);
        console.log(chalk.gray(`[DocumentIndexManager] Cleared index for ${path}`));
      }
    });
  }

  // clear all indices
  clearAllIndices(): void {
    this.documentIndices.clear();
    console.log(chalk.gray("[DocumentIndexManager] Cleared all indices"));
  }

  // remove stored document index from database
  async removeStoredDocumentIndex(documentPath: string): Promise<void> {
    try {
      await this.initializeDatabase();
      await databaseService.deleteStoredDocumentIndex(documentPath);
      this.clearIndices([documentPath]);
      console.log(chalk.gray(`[DocumentIndexManager] Removed stored document index for ${documentPath}`));
    } catch (error) {
      console.error(chalk.red(`[DocumentIndexManager] Error removing stored document index for ${documentPath}:`), error);
      throw error;
    }
  }
}

// export singleton instance
export const documentIndexManager = DocumentIndexManager.getInstance();
