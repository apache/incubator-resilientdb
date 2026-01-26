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

import fs from 'fs';
import path from 'path';
import matter from 'gray-matter';
import { gzipSync, gunzipSync } from 'zlib';

export interface DocumentSection {
  id: string;
  title: string;
  level: number;
  content: string;
  url: string;
}

export interface DocumentChunk {
  id: string;
  title: string;
  url: string;
  content: string;
  excerpt: string;
  sections: DocumentSection[];
  keywords: string[];
  metadata: {
    description?: string;
    parent?: string;
    nav_order?: number;
    tags?: string[];
  };
  chunkIndex: number;
  totalChunks: number;
}

export interface DocumentIndex {
  documents: DocumentChunk[];
  lastUpdated: string;
  totalDocuments: number;
}

class DocumentIndexer {
  private contentDir: string;
  private indexFile: string;

  constructor() {
    this.contentDir = path.join(process.cwd(), 'content');
    this.indexFile = path.join(process.cwd(), 'public', 'document-index.json');
  }

  // Extract keywords from title, content, and URL
  private extractKeywords(title: string, content: string, url: string, metadata: any): string[] {
    const keywords = new Set<string>();
    
    // Extract from title
    const titleWords = title.toLowerCase()
      .replace(/[^\w\s-]/g, ' ')
      .split(/\s+/)
      .filter(word => word.length > 2);
    titleWords.forEach(word => keywords.add(word));
    
    // Extract from URL path
    const urlParts = url.toLowerCase()
      .replace(/[^\w\s-]/g, ' ')
      .split(/\s+/)
      .filter(word => word.length > 2);
    urlParts.forEach(word => keywords.add(word));
    
    // Extract from content (first 500 chars for performance)
    const contentWords = content.toLowerCase()
      .substring(0, 500)
      .replace(/[^\w\s-]/g, ' ')
      .split(/\s+/)
      .filter(word => word.length > 3)
      .filter(word => !this.isStopWord(word));
    
    // Add most frequent content words
    const wordCounts = new Map<string, number>();
    contentWords.forEach(word => {
      wordCounts.set(word, (wordCounts.get(word) || 0) + 1);
    });
    
    // Add top 10 most frequent words
    Array.from(wordCounts.entries())
      .sort((a, b) => b[1] - a[1])
      .slice(0, 10)
      .forEach(([word]) => keywords.add(word));
    
    // Add metadata tags
    if (metadata.tags) {
      metadata.tags.forEach((tag: string) => keywords.add(tag.toLowerCase()));
    }
    
    // Add parent category
    if (metadata.parent) {
      keywords.add(metadata.parent.toLowerCase());
    }
    
    return Array.from(keywords);
  }

  // Common stop words to filter out
  private isStopWord(word: string): boolean {
    const stopWords = new Set([
      'the', 'and', 'or', 'but', 'in', 'on', 'at', 'to', 'for', 'of', 'with', 'by',
      'from', 'up', 'about', 'into', 'through', 'during', 'before', 'after',
      'above', 'below', 'between', 'among', 'this', 'that', 'these', 'those',
      'i', 'you', 'he', 'she', 'it', 'we', 'they', 'me', 'him', 'her', 'us', 'them',
      'my', 'your', 'his', 'her', 'its', 'our', 'their', 'mine', 'yours', 'ours', 'theirs',
      'am', 'is', 'are', 'was', 'were', 'be', 'been', 'being', 'have', 'has', 'had',
      'do', 'does', 'did', 'will', 'would', 'could', 'should', 'may', 'might', 'must',
      'can', 'shall', 'a', 'an', 'some', 'any', 'all', 'both', 'each', 'every', 'other',
      'another', 'such', 'no', 'nor', 'not', 'only', 'own', 'same', 'so', 'than', 'too',
      'very', 'just', 'now', 'here', 'there', 'where', 'when', 'why', 'how', 'what',
      'which', 'who', 'whom', 'whose', 'whether', 'if', 'because', 'as', 'until',
      'while', 'whereas', 'although', 'though', 'unless', 'since', 'once', 'whenever',
      'wherever', 'however', 'therefore', 'thus', 'hence', 'moreover', 'furthermore',
      'nevertheless', 'nonetheless', 'meanwhile', 'consequently', 'accordingly'
    ]);
    return stopWords.has(word);
  }

  // Extract headings and sections from content
  private extractSections(content: string, baseUrl: string): DocumentSection[] {
    const sections: DocumentSection[] = [];
    const lines = content.split('\n');
    
    // First pass: find all headings with their positions
    const headings: Array<{
      lineIndex: number;
      level: number;
      title: string;
      id: string;
    }> = [];
    
    for (let i = 0; i < lines.length; i++) {
      const line = lines[i];
      const headerMatch = line.match(/^(#{1,6})\s+(.+)$/);
      if (headerMatch) {
        const level = headerMatch[1].length;
        const title = headerMatch[2].trim();
        
        // Generate ID that matches Next.js/MDX heading ID generation
        const id = title
          .toLowerCase()
          .replace(/[^\w\s-]/g, '') // Remove special characters except word chars, spaces, and hyphens
          .replace(/\s+/g, '-') // Replace spaces with hyphens
          .replace(/-+/g, '-') // Replace multiple hyphens with single hyphen
          .replace(/^-|-$/g, ''); // Remove leading/trailing hyphens
        
        headings.push({
          lineIndex: i,
          level,
          title,
          id
        });
      }
    }
    
    // Second pass: extract content for each heading
    for (let i = 0; i < headings.length; i++) {
      const heading = headings[i];
      const nextHeading = headings[i + 1];
      
      // Determine content range
      const startLine = heading.lineIndex + 1; // Start after the heading line
      const endLine = nextHeading ? nextHeading.lineIndex : lines.length;
      
      // Extract content lines
      const contentLines: string[] = [];
      for (let j = startLine; j < endLine; j++) {
        const line = lines[j];
        // Stop if we hit another heading at the same or higher level
        const lineHeaderMatch = line.match(/^(#{1,6})\s+/);
        if (lineHeaderMatch && lineHeaderMatch[1].length <= heading.level) {
          break;
        }
        if (line.trim()) {
          contentLines.push(line);
        }
      }
      
      // For H1 headings, limit content to first paragraph or reasonable length
      if (heading.level === 1 && contentLines.length > 0) {
        // Find the first paragraph break (empty line) or limit to first few sentences
        const firstParagraph: string[] = [];
        let foundBreak = false;
        
        for (const line of contentLines) {
          if (line.trim() === '') {
            foundBreak = true;
            break;
          }
          firstParagraph.push(line);
          
          // Also limit by length to prevent overly long H1 content
          if (firstParagraph.join(' ').length > 500) {
            break;
          }
        }
        
        // Use first paragraph if it's reasonable, otherwise use limited content
        if (firstParagraph.length > 0 && firstParagraph.join(' ').length <= 500) {
          contentLines.splice(0, contentLines.length, ...firstParagraph);
        } else {
          // Fallback: limit to first 500 characters
          const limitedContent = contentLines.join(' ').substring(0, 500);
          contentLines.splice(0, contentLines.length, limitedContent);
        }
      }
      
      const sectionContent = contentLines.join(' ').trim();
      
      // Only add section if it has meaningful content
      if (sectionContent && sectionContent.length > 10) {
        sections.push({
          id: heading.id,
          title: heading.title,
          level: heading.level,
          content: sectionContent,
          url: `${baseUrl}#${heading.id}`
        });
      }
    }
    
    return sections;
  }

  // Extract text content from MDX files
  private extractContent(filePath: string): string {
    try {
      const fileContent = fs.readFileSync(filePath, 'utf-8');
      const { content } = matter(fileContent);
      
      // Remove frontmatter and clean up content
      return content
        .replace(/^---[\s\S]*?---\n/, '') // Remove frontmatter
        .replace(/```[\s\S]*?```/g, '') // Remove code blocks
        .replace(/`[^`]+`/g, '') // Remove inline code
        .replace(/#{1,6}\s+/g, '') // Remove headers
        .replace(/\*\*([^*]+)\*\*/g, '$1') // Remove bold
        .replace(/\*([^*]+)\*/g, '$1') // Remove italic
        .replace(/\[([^\]]+)\]\([^)]+\)/g, '$1') // Remove links, keep text
        .replace(/\n+/g, ' ') // Replace newlines with spaces
        .trim();
    } catch (error) {
      console.error(`Error reading file ${filePath}:`, error);
      return '';
    }
  }

  // Extract frontmatter metadata
  private extractMetadata(filePath: string): any {
    try {
      const fileContent = fs.readFileSync(filePath, 'utf-8');
      const { data } = matter(fileContent);
      return data;
    } catch (error) {
      console.error(`Error reading metadata from ${filePath}:`, error);
      return {};
    }
  }

  // Generate URL from file path
  private generateUrl(filePath: string): string {
    const relativePath = path.relative(this.contentDir, filePath);
    const url = relativePath
      .replace(/\.(mdx|md)$/, '') // Remove extension
      .replace(/\/index$/, '') // Remove index suffix
      .replace(/\\/g, '/'); // Normalize path separators
    
    return `/docs/${url}`;
  }

  // Split content into chunks for better search
  private chunkContent(content: string, maxChunkSize: number = 1000): string[] {
    if (content.length <= maxChunkSize) {
      return [content];
    }

    const chunks: string[] = [];
    const sentences = content.split(/[.!?]+/);
    let currentChunk = '';

    for (const sentence of sentences) {
      if (currentChunk.length + sentence.length > maxChunkSize && currentChunk.length > 0) {
        chunks.push(currentChunk.trim());
        currentChunk = sentence;
      } else {
        currentChunk += sentence + '.';
      }
    }

    if (currentChunk.trim()) {
      chunks.push(currentChunk.trim());
    }

    return chunks;
  }

  // Recursively find all MDX/MD files
  private findMarkdownFiles(dir: string): string[] {
    const files: string[] = [];
    
    try {
      const items = fs.readdirSync(dir);
      
      for (const item of items) {
        const fullPath = path.join(dir, item);
        const stat = fs.statSync(fullPath);
        
        if (stat.isDirectory()) {
          files.push(...this.findMarkdownFiles(fullPath));
        } else if (item.match(/\.(mdx|md)$/)) {
          files.push(fullPath);
        }
      }
    } catch (error) {
      console.error(`Error reading directory ${dir}:`, error);
    }
    
    return files;
  }

  // Index all documents
  public async indexDocuments(): Promise<DocumentIndex> {
    console.log('Starting document indexing...');
    
    const files = this.findMarkdownFiles(this.contentDir);
    const documents: DocumentChunk[] = [];
    
    for (const filePath of files) {
      try {
        const fileContent = fs.readFileSync(filePath, 'utf-8');
        const { content: rawContent } = matter(fileContent);
        const metadata = this.extractMetadata(filePath);
        const url = this.generateUrl(filePath);
        
        if (!rawContent.trim()) continue;
        
        // Extract sections first
        const sections = this.extractSections(rawContent, url);
        
        // Extract clean content for search
        const content = this.extractContent(filePath);
        const chunks = this.chunkContent(content);
        
        // Extract keywords from the full content and metadata
        const fullContent = content; // Use the full extracted content
        const keywords = this.extractKeywords(
          metadata.title || path.basename(filePath, path.extname(filePath)),
          fullContent,
          url,
          metadata
        );
        
        chunks.forEach((chunk, index) => {
          const document: DocumentChunk = {
            id: `${path.basename(filePath, path.extname(filePath))}-${index}`,
            title: metadata.title || path.basename(filePath, path.extname(filePath)),
            url,
            content: chunk,
            excerpt: chunk.substring(0, 200) + (chunk.length > 200 ? '...' : ''),
            sections,
            keywords, // Add extracted keywords
            metadata: {
              description: metadata.description,
              parent: metadata.parent,
              nav_order: metadata.nav_order,
              tags: metadata.tags || []
            },
            chunkIndex: index,
            totalChunks: chunks.length
          };
          
          documents.push(document);
        });
        
        console.log(`Indexed: ${url} (${chunks.length} chunks, ${sections.length} sections)`);
      } catch (error) {
        console.error(`Error indexing ${filePath}:`, error);
      }
    }
    
    const index: DocumentIndex = {
      documents,
      lastUpdated: new Date().toISOString(),
      totalDocuments: documents.length
    };
    
    // Save index to file
    this.saveIndex(index);
    
    console.log(`Document indexing complete. Indexed ${documents.length} chunks from ${files.length} files.`);
    return index;
  }

  // Save index to compressed JSON file
  private saveIndex(index: DocumentIndex): void {
    try {
      // Ensure public directory exists
      const publicDir = path.dirname(this.indexFile);
      if (!fs.existsSync(publicDir)) {
        fs.mkdirSync(publicDir, { recursive: true });
      }
      
      // Compress the JSON data
      const jsonData = JSON.stringify(index);
      const compressed = gzipSync(jsonData);
      
      fs.writeFileSync(this.indexFile, compressed);
      
      const originalSize = Buffer.byteLength(jsonData, 'utf8');
      const compressedSize = compressed.length;
      const compressionRatio = ((originalSize - compressedSize) / originalSize * 100).toFixed(1);
      
      console.log(`Index saved to ${this.indexFile}`);
      console.log(`Compression: ${(originalSize / 1024).toFixed(1)}KB → ${(compressedSize / 1024).toFixed(1)}KB (${compressionRatio}% reduction)`);
    } catch (error) {
      console.error('Error saving index:', error);
    }
  }

  // Load index from compressed file
  public loadIndex(): DocumentIndex | null {
    try {
      if (!fs.existsSync(this.indexFile)) {
        return null;
      }
      
      const compressedData = fs.readFileSync(this.indexFile);
      
      // Try to decompress first (new format)
      try {
        const decompressed = gunzipSync(compressedData);
        return JSON.parse(decompressed.toString('utf-8'));
      } catch (decompressionError) {
        // Fallback to plain JSON (old format)
        const indexData = compressedData.toString('utf-8');
        return JSON.parse(indexData);
      }
    } catch (error) {
      console.error('Error loading index:', error);
      return null;
    }
  }

  // Check if index needs updating
  public needsUpdate(): boolean {
    try {
      const index = this.loadIndex();
      if (!index) return true;
      
      const indexTime = new Date(index.lastUpdated).getTime();
      const now = Date.now();
      const oneHour = 60 * 60 * 1000; // 1 hour
      
      return (now - indexTime) > oneHour;
    } catch (error) {
      return true;
    }
  }
}

export const documentIndexer = new DocumentIndexer();
