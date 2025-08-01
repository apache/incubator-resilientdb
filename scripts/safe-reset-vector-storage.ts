#!/usr/bin/env tsx

import chalk from "chalk";
import { Pool } from "pg";
import * as readline from "readline";
import { config } from "../src/config/environment";

/**
 * Interactive script to safely drop and recreate vector storage tables
 * Includes confirmation prompts to prevent accidental data loss
 */

function askQuestion(question: string): Promise<string> {
  const rl = readline.createInterface({
    input: process.stdin,
    output: process.stdout,
  });

  return new Promise((resolve) => {
    rl.question(question, (answer) => {
      rl.close();
      resolve(answer.trim().toLowerCase());
    });
  });
}

async function safeResetVectorStorage() {
  console.log(chalk.red("⚠️  DANGER ZONE: Vector Storage Reset"));
  console.log(chalk.yellow("This will permanently delete:"));
  console.log(chalk.gray("  • All vector embeddings"));
  console.log(chalk.gray("  • All cached parsed documents"));
  console.log(chalk.gray("  • All similarity search data"));
  console.log("");

  // First confirmation
  const confirm1 = await askQuestion(
    chalk.red("Are you sure you want to proceed? Type 'yes' to continue: ")
  );

  if (confirm1 !== "yes") {
    console.log(chalk.blue("Operation cancelled."));
    process.exit(0);
  }

  // Second confirmation with table name
  const tableName = config.vectorStore.tableName;
  const confirm2 = await askQuestion(
    chalk.red(`This will drop table '${tableName}'. Type '${tableName}' to confirm: `)
  );

  if (confirm2 !== tableName) {
    console.log(chalk.blue("Operation cancelled. Table name did not match."));
    process.exit(0);
  }

  // Final confirmation
  const confirm3 = await askQuestion(
    chalk.red("FINAL WARNING: This cannot be undone. Type 'DELETE ALL DATA' to proceed: ")
  );

  if (confirm3 !== "delete all data") {
    console.log(chalk.blue("Operation cancelled."));
    process.exit(0);
  }

  console.log(chalk.yellow("\nProceeding with vector storage reset..."));

  const pool = new Pool({
    connectionString: config.databaseUrl,
    ssl: process.env.NODE_ENV === "production" ? { rejectUnauthorized: false } : false,
  });

  const client = await pool.connect();

  try {
    console.log(chalk.blue("🔄 Resetting PostgreSQL vector storage..."));

    const embedDim = config.vectorStore.embedDim;

    // Show current data before deletion
    console.log(chalk.yellow("📊 Current data status:"));
    try {
      const vectorCount = await client.query(`SELECT COUNT(*) FROM ${tableName};`);
      console.log(chalk.gray(`  Vector embeddings: ${vectorCount.rows[0].count} rows`));
    } catch {
      console.log(chalk.gray("  Vector embeddings: table does not exist"));
    }

    try {
      const chunksCount = await client.query(`SELECT COUNT(*) FROM parsed_document_chunks;`);
      console.log(chalk.gray(`  Parsed chunks: ${chunksCount.rows[0].count} rows`));
    } catch {
      console.log(chalk.gray("  Parsed chunks: table does not exist"));
    }

    console.log("");

    // 1. Drop existing vector embeddings table
    console.log(chalk.yellow("1. Dropping existing vector embeddings table..."));
    await client.query(`DROP TABLE IF EXISTS ${tableName} CASCADE;`);
    console.log(chalk.green(`✓ Table '${tableName}' dropped`));

    // 2. Drop parsed document chunks table
    console.log(chalk.yellow("2. Dropping parsed document chunks table..."));
    await client.query(`DROP TABLE IF EXISTS parsed_document_chunks CASCADE;`);
    console.log(chalk.green("✓ Parsed document chunks table dropped"));

    // 3. Drop the timestamp trigger function if it exists
    console.log(chalk.yellow("3. Cleaning up trigger functions..."));
    await client.query(`DROP FUNCTION IF EXISTS update_updated_at_column() CASCADE;`);
    console.log(chalk.green("✓ Trigger functions cleaned up"));

    // 4. Recreate the vector embeddings table
    console.log(chalk.yellow("4. Recreating vector embeddings table..."));
    await client.query(`
      CREATE TABLE ${tableName} (
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
    console.log(chalk.green(`✓ Table '${tableName}' recreated with ${embedDim} dimensions`));

    // 5. Recreate indexes for better performance
    console.log(chalk.yellow("5. Recreating performance indexes..."));
    
    // HNSW index for vector similarity search
    await client.query(`
      CREATE INDEX ${tableName}_embeddings_hnsw_idx 
      ON ${tableName} 
      USING hnsw (embeddings vector_cosine_ops)
      WITH (m = ${config.vectorStore.hnswConfig.m}, ef_construction = ${config.vectorStore.hnswConfig.efConstruction});
    `);
    console.log(chalk.green("✓ HNSW vector index recreated"));

    // Index on external_id for faster lookups
    await client.query(`
      CREATE INDEX ${tableName}_external_id_idx 
      ON ${tableName} (external_id);
    `);
    console.log(chalk.green("✓ External ID index recreated"));

    // Index on collection for faster lookups and data segregation
    await client.query(`
      CREATE INDEX ${tableName}_collection_idx 
      ON ${tableName} (collection);
    `);
    console.log(chalk.green("✓ Collection index recreated"));

    // GIN index on metadata for JSON queries
    await client.query(`
      CREATE INDEX ${tableName}_metadata_gin_idx 
      ON ${tableName} 
      USING GIN (metadata);
    `);
    console.log(chalk.green("✓ Metadata GIN index recreated"));

    // Index on timestamps for time-based queries
    await client.query(`
      CREATE INDEX ${tableName}_created_at_idx 
      ON ${tableName} (created_at);
    `);
    console.log(chalk.green("✓ Timestamp index recreated"));

    // 6. Recreate updated_at trigger function
    console.log(chalk.yellow("6. Recreating timestamp triggers..."));
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
      CREATE TRIGGER update_${tableName}_updated_at 
      BEFORE UPDATE ON ${tableName} 
      FOR EACH ROW 
      EXECUTE FUNCTION update_updated_at_column();
    `);
    console.log(chalk.green("✓ Updated timestamp trigger recreated"));

    // 7. Recreate parsed document chunks table
    console.log(chalk.yellow("7. Recreating parsed document chunks table..."));
    await client.query(`
      CREATE TABLE parsed_document_chunks (
        id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
        file_path TEXT NOT NULL,
        chunk_index INTEGER NOT NULL,
        chunk_content TEXT NOT NULL,
        chunk_metadata JSONB DEFAULT '{}',
        created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
        UNIQUE(file_path, chunk_index)
      );
    `);
    console.log(chalk.green("✓ Parsed document chunks table recreated"));

    // Create indexes for the parsed chunks table
    await client.query(`
      CREATE INDEX idx_parsed_chunks_file_path 
      ON parsed_document_chunks(file_path);
    `);
    await client.query(`
      CREATE INDEX idx_parsed_chunks_created_at 
      ON parsed_document_chunks(created_at);
    `);
    console.log(chalk.green("✓ Parsed document chunks indexes recreated"));

    // 8. Verify the reset
    console.log(chalk.yellow("8. Verifying reset..."));
    
    // Check if table exists and has correct structure
    const tableResult = await client.query(`
      SELECT column_name, data_type 
      FROM information_schema.columns 
      WHERE table_name = $1 
      ORDER BY ordinal_position;
    `, [tableName]);

    if (tableResult.rows.length === 0) {
      throw new Error(`Table '${tableName}' was not recreated properly`);
    }

    // Check if both tables are empty
    const vectorTableCount = await client.query(`SELECT COUNT(*) FROM ${tableName};`);
    const chunksTableCount = await client.query(`SELECT COUNT(*) FROM parsed_document_chunks;`);

    console.log(chalk.green("✓ Reset verification completed"));

    // 9. Display reset summary
    console.log(chalk.blue("\n🎉 Vector storage reset completed successfully!"));
    console.log(chalk.gray(`  Vector embeddings table: ${tableName} (${vectorTableCount.rows[0].count} rows)`));
    console.log(chalk.gray(`  Parsed chunks table: parsed_document_chunks (${chunksTableCount.rows[0].count} rows)`));
    console.log(chalk.gray(`  Embedding Dimension: ${embedDim}`));
    
    console.log(chalk.blue("\n📝 Next steps:"));
    console.log(chalk.gray("  1. Clear your document cache: npx tsx scripts/clear-document-cache.ts"));
    console.log(chalk.gray("  2. Re-index your documents with the new embedding model"));
    console.log(chalk.gray("  3. Test queries to verify improved similarity scores"));

  } catch (error) {
    console.error(chalk.red("❌ Reset failed:"), error);
    throw error;
  } finally {
    client.release();
    await pool.end();
  }
}

// Run the reset if this script is executed directly
if (require.main === module) {
  safeResetVectorStorage()
    .then(() => {
      console.log(chalk.green("\nReset completed successfully"));
      process.exit(0);
    })
    .catch((error) => {
      console.error(chalk.red("Reset failed:"), error);
      process.exit(1);
    });
}

export { safeResetVectorStorage };
