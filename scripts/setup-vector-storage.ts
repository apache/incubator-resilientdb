#!/usr/bin/env tsx

import chalk from "chalk";
import { Pool } from "pg";
import { config } from "../src/config/environment";

/**
 * Migration script to set up PostgreSQL with pgvector extension for vector storage
 * This script should be run once to prepare the database for vector operations
 */

async function setupVectorStorage() {
  const pool = new Pool({
    connectionString: config.databaseUrl,
    ssl: process.env.NODE_ENV === "production" ? { rejectUnauthorized: false } : false,
  });

  const client = await pool.connect();

  try {
    console.log(chalk.blue("Setting up PostgreSQL vector storage..."));

    // 1. Enable pgvector extension
    console.log(chalk.yellow("1. Enabling pgvector extension..."));
    await client.query(`
      CREATE EXTENSION IF NOT EXISTS vector;
    `);
    console.log(chalk.green("✓ pgvector extension enabled"));

    // 2. Create the vector embeddings table
    console.log(chalk.yellow("2. Creating vector embeddings table..."));
    const tableName = config.vectorStore.tableName;
    const embedDim = config.vectorStore.embedDim;

    // Check if table exists and drop it to ensure correct dimensions
    const tableExistsResult = await client.query(`
      SELECT table_name 
      FROM information_schema.tables 
      WHERE table_name = $1;
    `, [tableName]);
    
    if (tableExistsResult.rows.length > 0) {
      console.log(chalk.yellow(`Table '${tableName}' exists, dropping and recreating with ${embedDim} dimensions...`));
      await client.query(`DROP TABLE IF EXISTS ${tableName} CASCADE;`);
    }

    await client.query(`
      CREATE TABLE IF NOT EXISTS ${tableName} (
        id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
        external_id VARCHAR(255),
        collection VARCHAR(255) DEFAULT '',
        document TEXT NOT NULL,
        metadata JSONB DEFAULT '{}',
        embeddings vector(${embedDim}) NOT NULL,
        created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
        updated_at TIMESTAMP WITH TIME ZONE DEFAULT NOW()
      );
    `);
    console.log(chalk.green(`✓ Table '${tableName}' created`));

    // 3. Create indexes for better performance
    console.log(chalk.yellow("3. Creating performance indexes..."));
    
    // HNSW index for vector similarity search
    await client.query(`
      CREATE INDEX IF NOT EXISTS ${tableName}_embeddings_hnsw_idx 
      ON ${tableName} 
      USING hnsw (embeddings vector_cosine_ops)
      WITH (m = ${config.vectorStore.hnswConfig.m}, ef_construction = ${config.vectorStore.hnswConfig.efConstruction});
    `);
    console.log(chalk.green("✓ HNSW vector index created"));

    // Index on external_id for faster lookups
    await client.query(`
      CREATE INDEX IF NOT EXISTS ${tableName}_external_id_idx 
      ON ${tableName} (external_id);
    `);
    console.log(chalk.green("✓ External ID index created"));

    // Index on collection for faster lookups and data segregation
    await client.query(`
      CREATE INDEX IF NOT EXISTS ${tableName}_collection_idx 
      ON ${tableName} (collection);
    `);
    console.log(chalk.green("✓ Collection index created"));

    // GIN index on metadata for JSON queries
    await client.query(`
      CREATE INDEX IF NOT EXISTS ${tableName}_metadata_gin_idx 
      ON ${tableName} 
      USING GIN (metadata);
    `);
    console.log(chalk.green("✓ Metadata GIN index created"));

    // Index on timestamps for time-based queries
    await client.query(`
      CREATE INDEX IF NOT EXISTS ${tableName}_created_at_idx 
      ON ${tableName} (created_at);
    `);
    console.log(chalk.green("✓ Timestamp index created"));

    // 4. Create updated_at trigger function (if not exists)
    console.log(chalk.yellow("4. Setting up timestamp triggers..."));
    await client.query(`
      CREATE OR REPLACE FUNCTION update_updated_at_column()
      RETURNS TRIGGER AS $$
      BEGIN
          NEW.updated_at = NOW();
          RETURN NEW;
      END;
      $$ language 'plpgsql';
    `);

    // Create trigger for the embeddings table
    await client.query(`
      DO $$
      BEGIN
        IF NOT EXISTS (
          SELECT 1 FROM pg_trigger WHERE tgname = 'update_${tableName}_updated_at'
        ) THEN
          CREATE TRIGGER update_${tableName}_updated_at 
          BEFORE UPDATE ON ${tableName} 
          FOR EACH ROW 
          EXECUTE FUNCTION update_updated_at_column();
        END IF;
      END $$;
    `);
    console.log(chalk.green("✓ Updated timestamp trigger created"));

    // 5. Create parsed document chunks table for caching LlamaParse results
    console.log(chalk.yellow("5. Creating parsed document chunks table..."));
    await client.query(`
      CREATE TABLE IF NOT EXISTS parsed_document_chunks (
        id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
        file_path TEXT NOT NULL,
        chunk_index INTEGER NOT NULL,
        chunk_content TEXT NOT NULL,
        chunk_metadata JSONB DEFAULT '{}',
        created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
        UNIQUE(file_path, chunk_index)
      );
    `);
    console.log(chalk.green("✓ Parsed document chunks table created"));

    // Create indexes for the parsed chunks table
    await client.query(`
      CREATE INDEX IF NOT EXISTS idx_parsed_chunks_file_path 
      ON parsed_document_chunks(file_path);
    `);
    await client.query(`
      CREATE INDEX IF NOT EXISTS idx_parsed_chunks_created_at 
      ON parsed_document_chunks(created_at);
    `);
    console.log(chalk.green("✓ Parsed document chunks indexes created"));

    // 6. Verify the setup
    console.log(chalk.yellow("6. Verifying setup..."));
    
    // Check if pgvector extension is properly installed
    const extensionResult = await client.query(`
      SELECT * FROM pg_extension WHERE extname = 'vector';
    `);
    
    if (extensionResult.rows.length === 0) {
      throw new Error("pgvector extension is not installed");
    }

    // Check if table exists and has correct structure
    const tableResult = await client.query(`
      SELECT column_name, data_type 
      FROM information_schema.columns 
      WHERE table_name = $1 
      ORDER BY ordinal_position;
    `, [tableName]);

    if (tableResult.rows.length === 0) {
      throw new Error(`Table '${tableName}' was not created properly`);
    }

    console.log(chalk.green("✓ Setup verification completed"));
    console.log(chalk.blue("\nTable structure:"));
    tableResult.rows.forEach(row => {
      console.log(chalk.gray(`  - ${row.column_name}: ${row.data_type}`));
    });

    // 7. Display configuration summary
    console.log(chalk.blue("\nVector Storage Configuration:"));
    console.log(chalk.gray(`  Database: ${config.vectorStore.database}`));
    console.log(chalk.gray(`  Host: ${config.vectorStore.host}:${config.vectorStore.port}`));
    console.log(chalk.gray(`  Table: ${tableName}`));
    console.log(chalk.gray(`  Embedding Dimension: ${embedDim}`));
    console.log(chalk.gray(`  HNSW Config: m=${config.vectorStore.hnswConfig.m}, ef_construction=${config.vectorStore.hnswConfig.efConstruction}`));

    console.log(chalk.green("\n🎉 PostgreSQL vector storage setup completed successfully!"));
    console.log(chalk.blue("\nYou can now use PGVectorStore for your LlamaIndex vector operations."));

  } catch (error) {
    console.error(chalk.red("❌ Setup failed:"), error);
    throw error;
  } finally {
    client.release();
    await pool.end();
  }
}

// Run the setup if this script is executed directly
if (require.main === module) {
  setupVectorStorage()
    .then(() => {
      console.log(chalk.green("Setup completed successfully"));
      process.exit(0);
    })
    .catch((error) => {
      console.error(chalk.red("Setup failed:"), error);
      process.exit(1);
    });
}

export { setupVectorStorage };
