import { Pool } from "pg";
import { config } from "../config/environment";

// Types for document data
export interface DocumentData {
  id_: string;
  text: string;
  metadata: Record<string, any>;
}

export interface DocumentMetadata {
  documentPath: string;
  originalFileSize: number;
  originalModifiedTime: string;
  cachedAt: string;
  documentsCount: number;
}

export interface StoredDocumentIndex {
  document_path: string;
  parsed_data: DocumentData[];
  metadata: DocumentMetadata;
  original_file_size: number;
  original_modified_time: Date;
  created_at: Date;
  updated_at: Date;
}

class DatabaseService {
  private static instance: DatabaseService;
  private pool: Pool | null = null;
  private schemaInitialized: boolean = false;
  private schemaInitializationPromise: Promise<void> | null = null;

  private constructor() {}

  static getInstance(): DatabaseService {
    if (!DatabaseService.instance) {
      DatabaseService.instance = new DatabaseService();
    }
    return DatabaseService.instance;
  }

  // initialize database connection
  private getPool(): Pool {
    if (!this.pool) {
      if (!config.databaseUrl) {
        throw new Error("DATABASE_URL environment variable is required");
      }

      this.pool = new Pool({
        connectionString: config.databaseUrl,
        ssl: process.env.NODE_ENV === "production" ? { rejectUnauthorized: false } : false,
        max: 20,
        idleTimeoutMillis: 30000,
        connectionTimeoutMillis: 2000,
      });

      this.pool.on('error', (err) => {
        console.error('Unexpected error on idle client', err);
      });
    }
    return this.pool;
  }

  // initialize database schema
  async initializeSchema(): Promise<void> {
    // Fast path: schema already initialized
    if (this.schemaInitialized) {
      return;
    }

    // If an initialization is already in progress, wait for it
    if (this.schemaInitializationPromise) {
      return this.schemaInitializationPromise;
    }

    // Otherwise, start initialization and store the promise so other calls can wait
    this.schemaInitializationPromise = (async () => {
      const pool = this.getPool();
      const client = await pool.connect();

      try {
        const schemaSQL = `
        -- Document indices table: stores parsed document data and metadata
        CREATE TABLE IF NOT EXISTS document_indices (
            document_path VARCHAR(500) PRIMARY KEY,
            parsed_data JSONB NOT NULL,
            metadata JSONB NOT NULL,
            original_file_size BIGINT NOT NULL,
            original_modified_time TIMESTAMP WITH TIME ZONE NOT NULL,
            created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
            updated_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()
        );

        -- Index for faster queries on document paths
        CREATE INDEX IF NOT EXISTS idx_document_indices_path ON document_indices(document_path);

        -- Index for faster queries on modification times (for cache validation)
        CREATE INDEX IF NOT EXISTS idx_document_indices_modified_time ON document_indices(original_modified_time);

        -- Index for faster queries on parsed data content (GIN index for JSONB)
        CREATE INDEX IF NOT EXISTS idx_document_indices_parsed_data ON document_indices USING GIN (parsed_data);

        -- Function to automatically update the updated_at timestamp
        CREATE OR REPLACE FUNCTION update_updated_at_column()
        RETURNS TRIGGER AS $$
        BEGIN
            NEW.updated_at = NOW();
            RETURN NEW;
        END;
        $$ language 'plpgsql';

        -- Trigger to automatically update the updated_at column
        DO $$
        BEGIN
          IF NOT EXISTS (
            SELECT 1 FROM pg_trigger WHERE tgname = 'update_document_indices_updated_at'
          ) THEN
            CREATE TRIGGER update_document_indices_updated_at 
            BEFORE UPDATE ON document_indices 
            FOR EACH ROW 
            EXECUTE FUNCTION update_updated_at_column();
          END IF;
        END $$;
      `;

        await client.query(schemaSQL);
        console.log("Database schema initialized successfully");
        this.schemaInitialized = true;
      } catch (error) {
        console.error("Error initializing database schema:", error);
        throw error;
      } finally {
        client.release();
      }
    })();

    // Wait for initialization to finish (or propagate errors)
    return this.schemaInitializationPromise;
  }

  // save parsed documents to database
  async saveParsedDocuments(
    documentPath: string,
    documents: DocumentData[],
    originalFileSize: number,
    originalModifiedTime: Date
  ): Promise<void> {
    const pool = this.getPool();
    const client = await pool.connect();

    try {
      const metadata: DocumentMetadata = {
        documentPath,
        originalFileSize,
        originalModifiedTime: originalModifiedTime.toISOString(),
        cachedAt: new Date().toISOString(),
        documentsCount: documents.length,
      };

      const query = `
        INSERT INTO document_indices (
          document_path, 
          parsed_data, 
          metadata, 
          original_file_size, 
          original_modified_time
        )
        VALUES ($1, $2, $3, $4, $5)
        ON CONFLICT (document_path) 
        DO UPDATE SET
          parsed_data = EXCLUDED.parsed_data,
          metadata = EXCLUDED.metadata,
          original_file_size = EXCLUDED.original_file_size,
          original_modified_time = EXCLUDED.original_modified_time,
          updated_at = NOW()
      `;

      await client.query(query, [
        documentPath,
        JSON.stringify(documents),
        JSON.stringify(metadata),
        originalFileSize,
        originalModifiedTime,
      ]);

      console.log(`Saved parsed documents to database for: ${documentPath}`);
    } catch (error) {
      console.error("Error saving parsed documents:", error);
      throw error;
    } finally {
      client.release();
    }
  }

  // load parsed documents from database
  async loadParsedDocuments(documentPath: string): Promise<DocumentData[]> {
    const pool = this.getPool();
    const client = await pool.connect();

    try {
      const query = "SELECT parsed_data FROM document_indices WHERE document_path = $1";
      const result = await client.query(query, [documentPath]);

      if (result.rows.length === 0) {
        throw new Error(`No parsed documents found for: ${documentPath}`);
      }

      return result.rows[0].parsed_data;
    } catch (error) {
      console.error("Error loading parsed documents:", error);
      throw error;
    } finally {
      client.release();
    }
  }

  // check if parsed documents exist and are newer than source
  async shouldUseParsedDocuments(
    documentPath: string,
    sourceModifiedTime: Date
  ): Promise<boolean> {
    const pool = this.getPool();
    const client = await pool.connect();

    try {
      const query = `
        SELECT original_modified_time 
        FROM document_indices 
        WHERE document_path = $1
      `;
      const result = await client.query(query, [documentPath]);

      if (result.rows.length === 0) {
        return false; // No cached data exists
      }

      const cachedModifiedTime = new Date(result.rows[0].original_modified_time);
      return cachedModifiedTime >= sourceModifiedTime;
    } catch (error) {
      console.error("Error checking parsed documents validity:", error);
      return false; // If error, assume we should re-parse
    } finally {
      client.release();
    }
  }

  // Get stored document index with all metadata
  async getStoredDocumentIndex(documentPath: string): Promise<StoredDocumentIndex | null> {
    const pool = this.getPool();
    const client = await pool.connect();

    try {
      const query = `
        SELECT * FROM document_indices WHERE document_path = $1
      `;
      const result = await client.query(query, [documentPath]);

      if (result.rows.length === 0) {
        return null;
      }

      return result.rows[0];
    } catch (error) {
      console.error("Error getting stored document index:", error);
      return null;
    } finally {
      client.release();
    }
  }

  // Get all stored document paths
  async getAllStoredDocumentPaths(): Promise<string[]> {
    const pool = this.getPool();
    const client = await pool.connect();

    try {
      const query = "SELECT document_path FROM document_indices ORDER BY document_path";
      const result = await client.query(query);
      return result.rows.map(row => row.document_path);
    } catch (error) {
      console.error("Error getting all stored document paths:", error);
      return [];
    } finally {
      client.release();
    }
  }

  // Delete stored document index
  async deleteStoredDocumentIndex(documentPath: string): Promise<void> {
    const pool = this.getPool();
    const client = await pool.connect();

    try {
      const query = "DELETE FROM document_indices WHERE document_path = $1";
      await client.query(query, [documentPath]);
      console.log(`Deleted stored document index for: ${documentPath}`);
    } catch (error) {
      console.error("Error deleting stored document index:", error);
      throw error;
    } finally {
      client.release();
    }
  }

  // Close database connection
  async close(): Promise<void> {
    if (this.pool) {
      await this.pool.end();
      this.pool = null;
      console.log("Database connection closed");
    }
  }

  // Health check
  async healthCheck(): Promise<boolean> {
    try {
      const pool = this.getPool();
      const client = await pool.connect();
      await client.query('SELECT 1');
      client.release();
      return true;
    } catch (error) {
      console.error("Database health check failed:", error);
      return false;
    }
  }
}

// Export singleton instance
export const databaseService = DatabaseService.getInstance(); 