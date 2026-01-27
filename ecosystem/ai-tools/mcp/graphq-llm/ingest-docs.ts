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

#!/usr/bin/env tsx

/**
 * Document Ingestion Script
 * 
 * Ingests all documentation from docs/ directory and GraphQL schema
 * into ResilientDB for RAG (Retrieval-Augmented Generation).
 */

import { IngestionPipeline } from './src/rag/ingestion-pipeline';
import { DocumentLoader } from './src/rag/document-loader';
import { ResilientDBClient } from './src/resilientdb/client';
import { ResilientDBHTTPWrapper } from './src/resilientdb/http-wrapper';
import * as path from 'path';

async function ingestDocuments() {
  console.log('📚 Document Ingestion for RAG');
  console.log('================================\n');

  // Start HTTP wrapper for ResilientDB on port 18001 (18000 is used by Docker)
  console.log('🔧 Starting ResilientDB HTTP wrapper on port 18001...\n');
  const httpWrapper = new ResilientDBHTTPWrapper(18001);
  try {
    await httpWrapper.start();
    console.log('✅ ResilientDB HTTP API available at http://localhost:18001\n');
    
    // Update environment variable for this session
    process.env.RESILIENTDB_GRAPHQL_URL = 'http://localhost:18001/graphql';
  } catch (error) {
    console.log('⚠️  HTTP wrapper not started:', (error as Error).message);
    console.log('⚠️  Will try to use existing server\n');
  }

  const pipeline = new IngestionPipeline();
  const loader = new DocumentLoader();
  const client = new ResilientDBClient();

  // Step 1: Check availability
  console.log('🔍 Checking service availability...\n');
  const availability = await pipeline.checkAvailability();
  
  console.log(`Embedding Service: ${availability.embeddingService ? '✅ Available' : '❌ Unavailable'}`);
  console.log(`Vector Store: ${availability.vectorStore ? '✅ Available' : '❌ Unavailable'}`);
  
  if (!availability.embeddingService || !availability.vectorStore) {
    console.log(`\n⚠️  Warning: ${availability.message}`);
    console.log('Some services are unavailable. Ingestion may be limited.\n');
  }

  // Step 2: Load documents from docs/ directory
  console.log('📁 Loading documents from docs/ directory...\n');
  const docsPath = path.join(process.cwd(), 'docs');
  
  let documents;
  try {
    documents = await loader.loadDirectory(docsPath, {
      recursive: true,
      extensions: ['.md', '.txt', '.json'],
      exclude: ['node_modules', '.git', 'dist', 'build'],
    });
    console.log(`✅ Loaded ${documents.length} documents\n`);
    
    if (documents.length === 0) {
      console.log('⚠️  No documents found in docs/ directory');
      console.log('💡 Add .md, .txt, or .json files to docs/ directory\n');
      return;
    }
  } catch (error) {
    console.error(`❌ Failed to load documents: ${error instanceof Error ? error.message : String(error)}\n`);
    return;
  }

  // Step 3: Load GraphQL schema
  console.log('🔍 Loading GraphQL schema via introspection...\n');
  let schemaDocument;
  try {
    const schema = await client.introspectSchema();
    
    // Convert schema to readable text
    const schemaText = `# GraphQL Schema

## Queries
${schema.queries.map(q => `- ${q.name}: ${q.type || 'unknown'}`).join('\n')}

## Mutations
${schema.mutations.map(m => `- ${m.name}: ${m.type || 'unknown'}`).join('\n')}

## Types
${schema.types.map(t => `- ${t.name}: ${t.kind || 'unknown'}`).join('\n')}
`;

    schemaDocument = await loader.loadSchema(schemaText, 'graphql_schema');
    console.log('✅ GraphQL schema loaded\n');
    
    // Add schema to documents
    documents.push(schemaDocument);
  } catch (error) {
    console.warn(`⚠️  Failed to load GraphQL schema: ${error instanceof Error ? error.message : String(error)}`);
    console.log('Continuing with document ingestion only...\n');
  }

  // Step 4: Ingest documents
  console.log('🚀 Starting ingestion...\n');
  console.log(`Total documents to process: ${documents.length}\n`);

  let progressCount = 0;
  const progress = await pipeline.ingestDocuments(documents, undefined, {
    onProgress: (currentProgress) => {
      const percentage = currentProgress.totalChunks > 0
        ? Math.round((currentProgress.processedChunks / currentProgress.totalChunks) * 100)
        : 0;
      
      // Only log every 10% or on significant milestones
      const newCount = Math.floor(percentage / 10);
      if (newCount > progressCount || currentProgress.processedChunks === currentProgress.totalChunks) {
        console.log(
          `📊 Progress: ${currentProgress.processedChunks}/${currentProgress.totalChunks} chunks ` +
          `(${percentage}%) | ` +
          `Embedded: ${currentProgress.embeddedChunks} | ` +
          `Stored: ${currentProgress.storedChunks}`
        );
        progressCount = newCount;
      }
    },
  });

  // Step 5: Results
  console.log('\n' + '='.repeat(50));
  console.log('📊 Ingestion Results');
  console.log('='.repeat(50));
  console.log(`Documents processed: ${progress.processedDocuments}/${progress.totalDocuments}`);
  console.log(`Chunks created: ${progress.totalChunks}`);
  console.log(`Chunks embedded: ${progress.embeddedChunks}`);
  console.log(`Chunks stored: ${progress.storedChunks}`);
  console.log(`Errors: ${progress.errors.length}`);

  if (progress.errors.length > 0) {
    console.log('\n⚠️  Errors encountered:');
    progress.errors.forEach((error, index) => {
      console.log(`  ${index + 1}. ${error.documentId}: ${error.error}`);
    });
  }

  if (progress.storedChunks > 0) {
    console.log('\n✅ Ingestion completed successfully!');
    console.log(`📦 ${progress.storedChunks} document chunks are now available for RAG`);
    console.log('\n💡 You can now use the RAG system for query explanations and optimizations!');
  } else {
    console.log('\n⚠️  No chunks were stored. Check errors above.');
  }

  console.log('\n');
}

// Run ingestion
if (require.main === module) {
  ingestDocuments()
    .catch((error) => {
      console.error('\n❌ Fatal error during ingestion:', error);
      process.exit(1);
    });
}

export { ingestDocuments };

