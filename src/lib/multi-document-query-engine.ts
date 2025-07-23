import { MetadataMode } from "llamaindex";
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

export class MultiDocumentQueryEngine {
  private static instance: MultiDocumentQueryEngine;
  private maxContextLength: number = 220000; // deepseek max context length
  private defaultSimilarityTopK: number = 10;
  private static readonly SEPARATOR_LENGTH: number = 10;

  private constructor() {}

  static getInstance(): MultiDocumentQueryEngine {
    if (!MultiDocumentQueryEngine.instance) {
      MultiDocumentQueryEngine.instance = new MultiDocumentQueryEngine();
    }
    return MultiDocumentQueryEngine.instance;
  }

  // set maximum context length for responses
  setMaxContextLength(length: number): void {
    this.maxContextLength = length;
  }

  // get display name for a document path
  private getDocumentDisplayName(documentPath: string): string {
    const filename = documentPath.split("/").pop() || documentPath;

    const lowerFilename = filename.toLowerCase();
    return TITLE_MAPPINGS[lowerFilename] || filename.replace(".pdf", "");
  }

  // query multiple documents and return organized results
  async queryMultipleDocuments(
    query: string,
    documentPaths: string[],
    options: {
      similarityTopK?: number;
      includeMetadata?: boolean;
      maxContextLength?: number;
    } = {},
  ): Promise<QueryResult> {
    const {
      similarityTopK = this.defaultSimilarityTopK,
      includeMetadata = true,
      maxContextLength = this.maxContextLength,
    } = options;

    // validate that all documents have indices
    const hasAllIndices = documentIndexManager.hasAllIndices(documentPaths);
    if (!hasAllIndices) {
      throw new Error(
        "Some document indices are not prepared. Please prepare all documents first.",
      );
    }

    // get combined index
    const combinedIndex =
      await documentIndexManager.getCombinedIndex(documentPaths);
    if (!combinedIndex) {
      throw new Error("Failed to create combined index from documents");
    }

    // retrieve relevant chunks
    const retriever = combinedIndex.asRetriever({
      similarityTopK: similarityTopK * documentPaths.length, // more chunks for multiple docs
    });

    const retrievedNodes = await retriever.retrieve({ query });

    // organize chunks by source document
    const chunksBySource: { [key: string]: ContextChunk[] } = {};
    let totalContextLength = 0;
    const processedChunks: ContextChunk[] = [];

    for (const node of retrievedNodes) {
      const sourceDoc = node.node.metadata?.source_document || "Unknown";
      const content = node.node.getContent(
        includeMetadata ? MetadataMode.ALL : MetadataMode.NONE,
      );

      // check if adding this chunk would exceed context limit
      if (totalContextLength + content.length > maxContextLength) {
        console.log(
          `Context limit reached. Stopping at ${processedChunks.length} chunks.`,
        );
        break;
      }

      const chunk: ContextChunk = {
        content,
        source: sourceDoc,
        metadata: includeMetadata ? node.node.metadata : undefined,
      };

      if (!chunksBySource[sourceDoc]) {
        chunksBySource[sourceDoc] = [];
      }
      chunksBySource[sourceDoc].push(chunk);
      processedChunks.push(chunk);
      totalContextLength += content.length;
    }

    // create formatted context with source attribution
    const contextParts = Object.entries(chunksBySource).map(
      ([source, chunks]) => {
        const displayName = this.getDocumentDisplayName(source);
        const chunkContents = chunks.map((chunk) => chunk.content).join("\n\n");
        return `**From ${displayName}:**\n${chunkContents}`;
      },
    );

    const formattedContext = contextParts.join("\n\n---\n\n");

    // create source information
    const sources: DocumentSource[] = documentPaths.map((path) => ({
      path,
      name: path.split("/").pop() || path,
      displayTitle: this.getDocumentDisplayName(path),
    }));

    return {
      context: formattedContext,
      sources,
      chunks: processedChunks,
      totalChunks: retrievedNodes.length,
    };
  }

  // query a single document (for backward compatibility)
  async querySingleDocument(
    query: string,
    documentPath: string,
    options: {
      similarityTopK?: number;
      includeMetadata?: boolean;
    } = {},
  ): Promise<QueryResult> {
    return this.queryMultipleDocuments(query, [documentPath], options);
  }

  // get context summary for selected documents
  async getDocumentsSummary(documentPaths: string[]): Promise<{
    totalDocuments: number;
    availableDocuments: number;
    sources: DocumentSource[];
    hasAllIndices: boolean;
  }> {
    const hasAllIndices = documentIndexManager.hasAllIndices(documentPaths);
    const sources: DocumentSource[] = documentPaths.map((path) => ({
      path,
      name: path.split("/").pop() || path,
      displayTitle: this.getDocumentDisplayName(path),
    }));

    return {
      totalDocuments: documentPaths.length,
      availableDocuments: documentPaths.filter((path) =>
        documentIndexManager.hasIndex(path),
      ).length,
      sources,
      hasAllIndices,
    };
  }

  // extract unique sources from query result
  getUniqueSources(chunks: ContextChunk[]): DocumentSource[] {
    const uniqueSources = new Set<string>();
    const sources: DocumentSource[] = [];

    chunks.forEach((chunk) => {
      if (!uniqueSources.has(chunk.source)) {
        uniqueSources.add(chunk.source);
        sources.push({
          path: chunk.source,
          name: chunk.source.split("/").pop() || chunk.source,
          displayTitle: this.getDocumentDisplayName(chunk.source),
        });
      }
    });

    return sources;
  }

  // truncate context to fit within limits while preserving source attribution
  truncateContext(context: string, maxLength: number): string {
    if (context.length <= maxLength) {
      return context;
    }

    // try to truncate at section boundaries (---)
    const sections = context.split("\n\n---\n\n");
    let truncated = "";

    for (const section of sections) {
      if (
        truncated.length +
          section.length +
          MultiDocumentQueryEngine.SEPARATOR_LENGTH <=
        maxLength
      ) {
        // +10 for separator
        truncated += (truncated ? "\n\n---\n\n" : "") + section;
      } else {
        break;
      }
    }

    if (truncated.length === 0) {
      // if no complete sections fit, truncate the first section
      truncated =
        sections[0].substring(0, maxLength - 20) + "...\n\n[Context truncated]";
    } else {
      truncated += "\n\n[Additional context truncated]";
    }

    return truncated;
  }
}

// export singleton instance
export const multiDocumentQueryEngine = MultiDocumentQueryEngine.getInstance();
