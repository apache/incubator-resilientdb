import * as fs from 'fs';
import * as path from 'path';

/**
 * Document Loader
 * 
 * Loads documents from various sources:
 * - Markdown files (.md)
 * - Text files (.txt)
 * - JSON files (for structured data)
 * - GraphQL schema (via introspection)
 */
export interface LoadedDocument {
  id: string;
  content: string;
  source: string;
  type: 'markdown' | 'text' | 'json' | 'schema' | 'api';
  metadata: {
    filePath?: string;
    title?: string;
    section?: string;
    timestamp?: string;
  };
}

export class DocumentLoader {
  /**
   * Load a single file
   */
  async loadFile(filePath: string): Promise<LoadedDocument> {
    const fullPath = path.resolve(filePath);
    
    if (!fs.existsSync(fullPath)) {
      throw new Error(`File not found: ${fullPath}`);
    }

    const stats = fs.statSync(fullPath);
    if (!stats.isFile()) {
      throw new Error(`Path is not a file: ${fullPath}`);
    }

    const content = fs.readFileSync(fullPath, 'utf-8');
    const ext = path.extname(fullPath).toLowerCase();
    
    let type: LoadedDocument['type'] = 'text';
    if (ext === '.md' || ext === '.markdown') {
      type = 'markdown';
    } else if (ext === '.json') {
      type = 'json';
    }

    const fileName = path.basename(fullPath, ext);
    const id = `doc_${Date.now()}_${fileName.replace(/[^a-zA-Z0-9]/g, '_')}`;

    return {
      id,
      content,
      source: fullPath,
      type,
      metadata: {
        filePath: fullPath,
        title: fileName,
        timestamp: new Date().toISOString(),
      },
    };
  }

  /**
   * Load all files from a directory
   */
  async loadDirectory(
    dirPath: string,
    options: {
      recursive?: boolean;
      extensions?: string[];
      exclude?: string[];
    } = {}
  ): Promise<LoadedDocument[]> {
    const {
      recursive = true,
      extensions = ['.md', '.txt', '.json'],
      exclude = ['node_modules', '.git', 'dist', 'build'],
    } = options;

    const fullPath = path.resolve(dirPath);
    
    if (!fs.existsSync(fullPath)) {
      throw new Error(`Directory not found: ${fullPath}`);
    }

    const stats = fs.statSync(fullPath);
    if (!stats.isDirectory()) {
      throw new Error(`Path is not a directory: ${fullPath}`);
    }

    const documents: LoadedDocument[] = [];
    const files = this.getAllFiles(fullPath, recursive, extensions, exclude);

    for (const file of files) {
      try {
        const doc = await this.loadFile(file);
        documents.push(doc);
      } catch (error) {
        console.warn(`Failed to load file ${file}:`, error instanceof Error ? error.message : String(error));
        // Continue loading other files
      }
    }

    return documents;
  }

  /**
   * Recursively get all files matching extensions
   */
  private getAllFiles(
    dirPath: string,
    recursive: boolean,
    extensions: string[],
    exclude: string[]
  ): string[] {
    const files: string[] = [];

    try {
      const entries = fs.readdirSync(dirPath, { withFileTypes: true });

      for (const entry of entries) {
        const fullPath = path.join(dirPath, entry.name);

        // Skip excluded directories/files
        if (exclude.some(excluded => fullPath.includes(excluded))) {
          continue;
        }

        if (entry.isDirectory() && recursive) {
          // Recursively get files from subdirectories
          files.push(...this.getAllFiles(fullPath, recursive, extensions, exclude));
        } else if (entry.isFile()) {
          const ext = path.extname(entry.name).toLowerCase();
          if (extensions.includes(ext)) {
            files.push(fullPath);
          }
        }
      }
    } catch (error) {
      console.warn(`Failed to read directory ${dirPath}:`, error instanceof Error ? error.message : String(error));
    }

    return files;
  }

  /**
   * Load GraphQL schema as a document
   */
  async loadSchema(schemaContent: string, schemaId: string = 'graphql_schema'): Promise<LoadedDocument> {
    return {
      id: schemaId,
      content: schemaContent,
      source: 'graphql_introspection',
      type: 'schema',
      metadata: {
        title: 'GraphQL Schema',
        timestamp: new Date().toISOString(),
      },
    };
  }

  /**
   * Load API documentation as a document
   */
  async loadAPIDocumentation(
    apiContent: string,
    apiId: string,
    title?: string
  ): Promise<LoadedDocument> {
    return {
      id: apiId,
      content: apiContent,
      source: 'api_documentation',
      type: 'api',
      metadata: {
        title: title || apiId,
        timestamp: new Date().toISOString(),
      },
    };
  }
}

