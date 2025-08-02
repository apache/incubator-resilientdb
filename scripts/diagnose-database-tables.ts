#!/usr/bin/env tsx

import chalk from "chalk";
import { config } from "../src/config/environment";
import { databaseConnection } from "../src/lib/database-connection";

async function main() {
  try {
    console.log(chalk.blue("🔍 Diagnosing database tables..."));
    
    // Show configuration
    console.log(chalk.cyan("\n📋 Vector Store Configuration:"));
    console.log(chalk.gray(`  Database: ${config.vectorStore.database}`));
    console.log(chalk.gray(`  Host: ${config.vectorStore.host}`));
    console.log(chalk.gray(`  Port: ${config.vectorStore.port}`));
    console.log(chalk.gray(`  User: ${config.vectorStore.user}`));
    console.log(chalk.gray(`  Table Name: ${config.vectorStore.tableName}`));
    console.log(chalk.gray(`  Environment VECTOR_TABLE_NAME: ${process.env.VECTOR_TABLE_NAME || 'not set'}`));
    
    // List all tables in the database
    console.log(chalk.blue("\n📊 Listing all tables in database:"));
    const tablesResult = await databaseConnection.query(`
      SELECT tablename 
      FROM pg_tables 
      WHERE schemaname = 'public' 
      ORDER BY tablename;
    `);
    
    if (tablesResult.rows.length === 0) {
      console.log(chalk.yellow("  No tables found in public schema"));
    } else {
      tablesResult.rows.forEach((row: any) => {
        console.log(chalk.gray(`  - ${row.tablename}`));
      });
    }
    
    // Check for vector-related tables specifically
    console.log(chalk.blue("\n🔍 Checking for vector-related tables:"));
    const vectorTablesResult = await databaseConnection.query(`
      SELECT tablename 
      FROM pg_tables 
      WHERE schemaname = 'public' 
      AND (tablename LIKE '%vector%' OR tablename LIKE '%embedding%' OR tablename LIKE '%document%')
      ORDER BY tablename;
    `);
    
    if (vectorTablesResult.rows.length === 0) {
      console.log(chalk.yellow("  No vector-related tables found"));
    } else {
      vectorTablesResult.rows.forEach((row: any) => {
        console.log(chalk.green(`  ✓ ${row.tablename}`));
      });
    }
    
    // Check if the configured table exists
    const expectedTableName = process.env.VECTOR_TABLE_NAME || config.vectorStore.tableName;
    console.log(chalk.blue(`\n🎯 Checking for expected table: ${expectedTableName}`));
    
    const tableExistsResult = await databaseConnection.query(`
      SELECT EXISTS (
        SELECT FROM information_schema.tables 
        WHERE table_schema = 'public' 
        AND table_name = $1
      );
    `, [expectedTableName]);
    
    const tableExists = tableExistsResult.rows[0].exists;
    
    if (tableExists) {
      console.log(chalk.green(`  ✅ Table '${expectedTableName}' exists!`));
      
      // Get table structure
      const structureResult = await databaseConnection.query(`
        SELECT column_name, data_type, is_nullable
        FROM information_schema.columns
        WHERE table_schema = 'public' 
        AND table_name = $1
        ORDER BY ordinal_position;
      `, [expectedTableName]);
      
      console.log(chalk.cyan("\n📋 Table structure:"));
      structureResult.rows.forEach((row: any) => {
        console.log(chalk.gray(`  - ${row.column_name}: ${row.data_type} (${row.is_nullable === 'YES' ? 'nullable' : 'not null'})`));
      });
      
      // Get row count
      const countResult = await databaseConnection.query(`SELECT COUNT(*) as count FROM ${expectedTableName}`);
      const rowCount = parseInt(countResult.rows[0].count) || 0;
      console.log(chalk.cyan(`\n📊 Row count: ${rowCount}`));
      
      if (rowCount > 0) {
        // Sample a few rows to see the structure
        const sampleResult = await databaseConnection.query(`
          SELECT id, metadata, LEFT(document, 50) as document_preview 
          FROM ${expectedTableName} 
          LIMIT 3
        `);
        
        console.log(chalk.cyan("\n📝 Sample rows:"));
        sampleResult.rows.forEach((row: any, index: number) => {
          console.log(chalk.gray(`  ${index + 1}. ID: ${row.id}`));
          console.log(chalk.gray(`     Metadata: ${JSON.stringify(row.metadata)}`));
          console.log(chalk.gray(`     Document: ${row.document_preview}...`));
        });
      }
    } else {
      console.log(chalk.red(`  ❌ Table '${expectedTableName}' does not exist!`));
      console.log(chalk.yellow("\n💡 Suggestions:"));
      console.log(chalk.gray("  1. Run: npm run db:setup-vector"));
      console.log(chalk.gray("  2. Or check if VECTOR_TABLE_NAME environment variable is set correctly"));
      console.log(chalk.gray("  3. Or check if the vector store has been initialized"));
    }
    
  } catch (error) {
    console.error(chalk.red("❌ Error during diagnosis:"), error);
    process.exit(1);
  }
}

if (require.main === module) {
  main().catch(console.error);
}