import { MetadataMode } from 'llamaindex';
import { documentIndexManager } from './document-index-manager';

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

    // Set maximum context length for responses
    setMaxContextLength(length: number): void {
        this.maxContextLength = length;
    }

    // Get display name for a document path
    private getDocumentDisplayName(documentPath: string): string {
        const filename = documentPath.split('/').pop() || documentPath;
        
        // Title mappings - extend as needed
        const titleMappings: Record<string, string> = {
            "resilientdb.pdf": "ResilientDB: Global Scale Resilient Blockchain Fabric",
            "rcc.pdf": "Resilient Concurrent Consensus for High-Throughput Secure Transaction Processing",
        };
        
        const lowerFilename = filename.toLowerCase();
        return titleMappings[lowerFilename] || filename.replace('.pdf', '');
    }

    // Query multiple documents and return organized results
    async queryMultipleDocuments(
        query: string, 
        documentPaths: string[], 
        options: {
            similarityTopK?: number;
            includeMetadata?: boolean;
            maxContextLength?: number;
        } = {}
    ): Promise<QueryResult> {
        const {
            similarityTopK = this.defaultSimilarityTopK,
            includeMetadata = true,
            maxContextLength = this.maxContextLength
        } = options;

        // Validate that all documents have indices
        const hasAllIndices = documentIndexManager.hasAllIndices(documentPaths);
        if (!hasAllIndices) {
            throw new Error("Some document indices are not prepared. Please prepare all documents first.");
        }

        // Get combined index
        const combinedIndex = await documentIndexManager.getCombinedIndex(documentPaths);
        if (!combinedIndex) {
            throw new Error("Failed to create combined index from documents");
        }

        // Retrieve relevant chunks
        const retriever = combinedIndex.asRetriever({ 
            similarityTopK: similarityTopK * documentPaths.length // More chunks for multiple docs
        });
        
        const retrievedNodes = await retriever.retrieve({ query });

        // Organize chunks by source document
        const chunksBySource: { [key: string]: ContextChunk[] } = {};
        let totalContextLength = 0;
        const processedChunks: ContextChunk[] = [];

        for (const node of retrievedNodes) {
            const sourceDoc = node.node.metadata?.source_document || 'Unknown';
            const content = node.node.getContent(includeMetadata ? MetadataMode.ALL : MetadataMode.NONE);
            
            // Check if adding this chunk would exceed context limit
            if (totalContextLength + content.length > maxContextLength) {
                console.log(`Context limit reached. Stopping at ${processedChunks.length} chunks.`);
                break;
            }

            const chunk: ContextChunk = {
                content,
                source: sourceDoc,
                metadata: includeMetadata ? node.node.metadata : undefined
            };

            if (!chunksBySource[sourceDoc]) {
                chunksBySource[sourceDoc] = [];
            }
            chunksBySource[sourceDoc].push(chunk);
            processedChunks.push(chunk);
            totalContextLength += content.length;
        }

        // Create formatted context with source attribution
        const contextParts = Object.entries(chunksBySource).map(([source, chunks]) => {
            const displayName = this.getDocumentDisplayName(source);
            const chunkContents = chunks.map(chunk => chunk.content).join('\n\n');
            return `**From ${displayName}:**\n${chunkContents}`;
        });

        const formattedContext = contextParts.join('\n\n---\n\n');

        // Create source information
        const sources: DocumentSource[] = documentPaths.map(path => ({
            path,
            name: path.split('/').pop() || path,
            displayTitle: this.getDocumentDisplayName(path)
        }));

        return {
            context: formattedContext,
            sources,
            chunks: processedChunks,
            totalChunks: retrievedNodes.length
        };
    }

    // Query a single document (for backward compatibility)
    async querySingleDocument(
        query: string, 
        documentPath: string, 
        options: {
            similarityTopK?: number;
            includeMetadata?: boolean;
        } = {}
    ): Promise<QueryResult> {
        return this.queryMultipleDocuments(query, [documentPath], options);
    }

    // Get context summary for selected documents
    async getDocumentsSummary(documentPaths: string[]): Promise<{
        totalDocuments: number;
        availableDocuments: number;
        sources: DocumentSource[];
        hasAllIndices: boolean;
    }> {
        const hasAllIndices = documentIndexManager.hasAllIndices(documentPaths);
        const sources: DocumentSource[] = documentPaths.map(path => ({
            path,
            name: path.split('/').pop() || path,
            displayTitle: this.getDocumentDisplayName(path)
        }));

        return {
            totalDocuments: documentPaths.length,
            availableDocuments: documentPaths.filter(path => documentIndexManager.hasIndex(path)).length,
            sources,
            hasAllIndices
        };
    }

    // Extract unique sources from query result
    getUniqueSources(chunks: ContextChunk[]): DocumentSource[] {
        const uniqueSources = new Set<string>();
        const sources: DocumentSource[] = [];

        chunks.forEach(chunk => {
            if (!uniqueSources.has(chunk.source)) {
                uniqueSources.add(chunk.source);
                sources.push({
                    path: chunk.source,
                    name: chunk.source.split('/').pop() || chunk.source,
                    displayTitle: this.getDocumentDisplayName(chunk.source)
                });
            }
        });

        return sources;
    }

    // Truncate context to fit within limits while preserving source attribution
    truncateContext(context: string, maxLength: number): string {
        if (context.length <= maxLength) {
            return context;
        }

        // Try to truncate at section boundaries (---)
        const sections = context.split('\n\n---\n\n');
        let truncated = '';
        
        for (const section of sections) {
            if (truncated.length + section.length + MultiDocumentQueryEngine.SEPARATOR_LENGTH <= maxLength) { // +10 for separator
                truncated += (truncated ? '\n\n---\n\n' : '') + section;
            } else {
                break;
            }
        }

        if (truncated.length === 0) {
            // If no complete sections fit, truncate the first section
            truncated = sections[0].substring(0, maxLength - 20) + '...\n\n[Context truncated]';
        } else {
            truncated += '\n\n[Additional context truncated]';
        }

        return truncated;
    }
}

// Export singleton instance
export const multiDocumentQueryEngine = MultiDocumentQueryEngine.getInstance(); 