import { LoadedDocument } from './document-loader';

/**
 * Chunking Service
 * 
 * Splits documents into chunks for embedding:
 * - Size-based chunking (max tokens/chars)
 * - Semantic chunking (by sections/headers)
 * - Overlap strategy (to preserve context)
 */
export interface DocumentChunk {
  id: string;
  chunkText: string;
  source: string;
  chunkIndex: number;
  metadata: {
    documentId: string;
    section?: string;
    type: LoadedDocument['type'];
    startChar?: number;
    endChar?: number;
  };
}

export class ChunkingService {
  private maxChunkSize: number; // in characters (approximate)
  private chunkOverlap: number; // in characters
  private tokensPerChar: number = 0.25; // approximate: 4 chars per token

  constructor(options: {
    maxTokens?: number;
    chunkOverlapTokens?: number;
  } = {}) {
    const { maxTokens = 512, chunkOverlapTokens = 50 } = options;
    
    // Convert tokens to characters (approximate)
    this.maxChunkSize = maxTokens / this.tokensPerChar;
    this.chunkOverlap = chunkOverlapTokens / this.tokensPerChar;
  }

  /**
   * Chunk a document using size-based strategy with overlap
   */
  chunkBySize(document: LoadedDocument): DocumentChunk[] {
    const chunks: DocumentChunk[] = [];
    const content = document.content;
    
    if (!content || content.trim().length === 0) {
      return [];
    }

    let startIndex = 0;
    let chunkIndex = 0;
    const maxIterations = 10000; // Safety limit
    let iterations = 0;

    while (startIndex < content.length && iterations < maxIterations) {
      iterations++;
      
      // Calculate end index
      let endIndex = Math.min(startIndex + this.maxChunkSize, content.length);

      // Try to break at a word boundary if possible (but not on first iteration)
      if (endIndex < content.length && chunkIndex > 0) {
        // Look for a good break point (newline, period, space)
        const searchEnd = Math.min(endIndex + 50, content.length); // Look ahead a bit
        const breakPoints = [
          content.lastIndexOf('\n\n', searchEnd), // Paragraph break
          content.lastIndexOf('\n', searchEnd),   // Line break
          content.lastIndexOf('. ', searchEnd),   // Sentence end
          content.lastIndexOf(' ', searchEnd),     // Word break
        ].filter(bp => bp > startIndex);

        if (breakPoints.length > 0) {
          const bestBreak = Math.max(...breakPoints);
          if (bestBreak > startIndex + this.maxChunkSize * 0.5) {
            endIndex = bestBreak + 1;
          }
        }
      }

      const chunkText = content.slice(startIndex, endIndex).trim();
      
      if (chunkText.length > 0) {
        chunks.push({
          id: `${document.id}_chunk_${chunkIndex}`,
          chunkText,
          source: document.source,
          chunkIndex,
          metadata: {
            documentId: document.id,
            type: document.type,
            startChar: startIndex,
            endChar: endIndex,
            section: document.metadata.section,
          },
        });

        chunkIndex++;
      }

      // Move start index forward with overlap
      const nextStartIndex = endIndex - this.chunkOverlap;
      
      // Ensure we always make progress
      if (nextStartIndex <= startIndex) {
        startIndex = endIndex; // Move past current chunk
      } else {
        startIndex = nextStartIndex;
      }
      
      // Final safety: break if we're at the end
      if (startIndex >= content.length) {
        break;
      }
    }

    if (iterations >= maxIterations) {
      console.warn(`Chunking stopped after ${maxIterations} iterations to prevent infinite loop`);
    }

    return chunks;
  }

  /**
   * Chunk a markdown document by sections/headers
   */
  chunkBySections(document: LoadedDocument): DocumentChunk[] {
    if (document.type !== 'markdown') {
      // Fall back to size-based chunking for non-markdown
      return this.chunkBySize(document);
    }

    const chunks: DocumentChunk[] = [];
    const content = document.content;
    
    // Split by markdown headers (# ## ###)
    const headerRegex = /^(#{1,6}\s+.+)$/gm;
    const sections: Array<{ title: string; start: number; end?: number }> = [];
    
    let match;
    const headerMatches: Array<{ title: string; index: number }> = [];
    
    while ((match = headerRegex.exec(content)) !== null) {
      headerMatches.push({
        title: match[1].replace(/^#+\s+/, '').trim(),
        index: match.index,
      });
    }

    // Create sections
    for (let i = 0; i < headerMatches.length; i++) {
      const header = headerMatches[i];
      const nextHeader = headerMatches[i + 1];
      
      sections.push({
        title: header.title,
        start: header.index,
        end: nextHeader ? nextHeader.index : content.length,
      });
    }

    // If no headers found, fall back to size-based chunking
    if (sections.length === 0) {
      return this.chunkBySize(document);
    }

    // Create chunks for each section
    sections.forEach((section, index) => {
      const sectionText = content.slice(section.start, section.end).trim();
      
      // If section is too large, split it further
      if (sectionText.length > this.maxChunkSize) {
        // Split large section using size-based chunking
        const subDocument: LoadedDocument = {
          ...document,
          id: `${document.id}_section_${index}`,
          content: sectionText,
          metadata: {
            ...document.metadata,
            section: section.title,
          },
        };
        
        const subChunks = this.chunkBySize(subDocument);
        chunks.push(...subChunks);
      } else {
        // Use entire section as one chunk
        chunks.push({
          id: `${document.id}_section_${index}`,
          chunkText: sectionText,
          source: document.source,
          chunkIndex: index,
          metadata: {
            documentId: document.id,
            type: document.type,
            section: section.title,
            startChar: section.start,
            endChar: section.end,
          },
        });
      }
    });

    return chunks;
  }

  /**
   * Chunk a document using the best strategy for its type
   */
  chunkDocument(document: LoadedDocument): DocumentChunk[] {
    if (document.type === 'markdown') {
      // Try semantic chunking first for markdown
      const semanticChunks = this.chunkBySections(document);
      if (semanticChunks.length > 0) {
        return semanticChunks;
      }
    }
    
    // Fall back to size-based chunking
    return this.chunkBySize(document);
  }

  /**
   * Chunk multiple documents
   */
  chunkDocuments(documents: LoadedDocument[]): DocumentChunk[] {
    const allChunks: DocumentChunk[] = [];
    
    for (const document of documents) {
      try {
        const chunks = this.chunkDocument(document);
        allChunks.push(...chunks);
      } catch (error) {
        console.warn(
          `Failed to chunk document ${document.id}:`,
          error instanceof Error ? error.message : String(error)
        );
        // Continue with other documents
      }
    }

    return allChunks;
  }
}

