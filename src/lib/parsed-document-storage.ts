import chalk from "chalk";
import { Document } from "llamaindex";
import { databaseConnection } from "./database-connection";

export interface ParsedDocumentStorage {
  hasDocument(filePath: string): Promise<boolean>;
  storeDocument(filePath: string, chunks: Document[]): Promise<void>;
  getDocument(filePath: string): Promise<Document[] | null>;
  removeDocument(filePath: string): Promise<void>;
  clearAll(): Promise<void>;
  getStoredDocumentCount(): Promise<number>;
}

/**
 * Simple storage service for parsed document chunks
 * Prevents re-parsing documents that have already been processed by LlamaParse
 */
export class SimpleParsedDocumentStorage implements ParsedDocumentStorage {
  private static instance: SimpleParsedDocumentStorage;
  private tableName = "parsed_document_chunks";

  private constructor() {}

  static getInstance(): SimpleParsedDocumentStorage {
    if (!SimpleParsedDocumentStorage.instance) {
      SimpleParsedDocumentStorage.instance = new SimpleParsedDocumentStorage();
    }
    return SimpleParsedDocumentStorage.instance;
  }

  /**
   * Check if a document has already been parsed and stored
   */
  async hasDocument(filePath: string): Promise<boolean> {
    try {
      const result = await databaseConnection.query(
        `SELECT 1 FROM ${this.tableName} WHERE file_path = $1 LIMIT 1`,
        [filePath]
      );
      return result.rows.length > 0;
    } catch (error) {
      console.error(chalk.red(`[ParsedDocumentStorage] Error checking document ${filePath}:`), error);
      return false;
    }
  }

  /**
   * Store parsed document chunks in the database
   */
  async storeDocument(filePath: string, chunks: Document[]): Promise<void> {
    if (!chunks || chunks.length === 0) {
      console.warn(chalk.yellow(`[ParsedDocumentStorage] No chunks to store for ${filePath}`));
      return;
    }

    const client = await databaseConnection.getClient();
    
    try {
      await client.query('BEGIN');

      // First, remove any existing chunks for this document
      await client.query(
        `DELETE FROM ${this.tableName} WHERE file_path = $1`,
        [filePath]
      );

      // Insert new chunks
      for (let i = 0; i < chunks.length; i++) {
        const chunk = chunks[i];
        await client.query(
          `INSERT INTO ${this.tableName} (file_path, chunk_index, chunk_content, chunk_metadata) 
           VALUES ($1, $2, $3, $4)`,
          [
            filePath,
            i,
            chunk.text,
            JSON.stringify(chunk.metadata || {})
          ]
        );
      }

      await client.query('COMMIT');
      console.log(chalk.green(`[ParsedDocumentStorage] Stored ${chunks.length} chunks for ${filePath}`));

    } catch (error) {
      await client.query('ROLLBACK');
      console.error(chalk.red(`[ParsedDocumentStorage] Error storing document ${filePath}:`), error);
      throw error;
    } finally {
      client.release();
    }
  }

  /**
   * Retrieve stored document chunks from the database
   */
  async getDocument(filePath: string): Promise<Document[] | null> {
    try {
      const result = await databaseConnection.query(
        `SELECT chunk_content, chunk_metadata 
         FROM ${this.tableName} 
         WHERE file_path = $1 
         ORDER BY chunk_index`,
        [filePath]
      );

      if (result.rows.length === 0) {
        return null;
      }

      const documents = result.rows.map((row: any) => new Document({
        text: row.chunk_content,
        metadata: typeof row.chunk_metadata === 'string' 
          ? JSON.parse(row.chunk_metadata) 
          : row.chunk_metadata
      }));

      console.log(chalk.gray(`[ParsedDocumentStorage] Retrieved ${documents.length} chunks for ${filePath}`));
      return documents;

    } catch (error) {
      console.error(chalk.red(`[ParsedDocumentStorage] Error retrieving document ${filePath}:`), error);
      return null;
    }
  }

  /**
   * Remove a specific document from storage
   */
  async removeDocument(filePath: string): Promise<void> {
    try {
      const result = await databaseConnection.query(
        `DELETE FROM ${this.tableName} WHERE file_path = $1`,
        [filePath]
      );

      const deletedCount = result.rowCount || 0;
      if (deletedCount > 0) {
        console.log(chalk.yellow(`[ParsedDocumentStorage] Removed ${deletedCount} chunks for ${filePath}`));
      } else {
        console.log(chalk.gray(`[ParsedDocumentStorage] No chunks found to remove for ${filePath}`));
      }

    } catch (error) {
      console.error(chalk.red(`[ParsedDocumentStorage] Error removing document ${filePath}:`), error);
      throw error;
    }
  }

  /**
   * Clear all stored documents (useful for testing or cleanup)
   */
  async clearAll(): Promise<void> {
    try {
      const result = await databaseConnection.query(`DELETE FROM ${this.tableName}`);
      const deletedCount = result.rowCount || 0;
      console.log(chalk.yellow(`[ParsedDocumentStorage] Cleared ${deletedCount} stored document chunks`));

    } catch (error) {
      console.error(chalk.red("[ParsedDocumentStorage] Error clearing all documents:"), error);
      throw error;
    }
  }

  /**
   * Get count of stored documents for monitoring
   */
  async getStoredDocumentCount(): Promise<number> {
    try {
      const result = await databaseConnection.query(
        `SELECT COUNT(DISTINCT file_path) as count FROM ${this.tableName}`
      );
      return parseInt(result.rows[0].count) || 0;

    } catch (error) {
      console.error(chalk.red("[ParsedDocumentStorage] Error getting document count:"), error);
      return 0;
    }
  }

  /**
   * Get storage statistics for monitoring
   */
  async getStorageStats(): Promise<{
    documentCount: number;
    totalChunks: number;
    oldestDocument: string | null;
    newestDocument: string | null;
  }> {
    try {
      const result = await databaseConnection.query(`
        SELECT 
          COUNT(DISTINCT file_path) as document_count,
          COUNT(*) as total_chunks,
          MIN(created_at) as oldest_created,
          MAX(created_at) as newest_created
        FROM ${this.tableName}
      `);

      const row = result.rows[0];
      return {
        documentCount: parseInt(row.document_count) || 0,
        totalChunks: parseInt(row.total_chunks) || 0,
        oldestDocument: row.oldest_created,
        newestDocument: row.newest_created
      };

    } catch (error) {
      console.error(chalk.red("[ParsedDocumentStorage] Error getting storage stats:"), error);
      return {
        documentCount: 0,
        totalChunks: 0,
        oldestDocument: null,
        newestDocument: null
      };
    }
  }
}

// Export singleton instance
export const parsedDocumentStorage = SimpleParsedDocumentStorage.getInstance();