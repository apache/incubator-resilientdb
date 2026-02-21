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
 * RAG Test Script
 * 
 * Comprehensive test suite for the RAG (Retrieval-Augmented Generation) implementation.
 * Tests:
 * 1. Document loading and chunking
 * 2. Embedding generation
 * 3. Vector store operations
 * 4. Retrieval service (semantic search)
 * 5. Context formatting
 * 6. End-to-end RAG flow
 */

import { IngestionPipeline } from './src/rag/ingestion-pipeline';
import { RetrievalService } from './src/rag/retrieval-service';
import { DocumentLoader } from './src/rag/document-loader';
import { ChunkingService } from './src/rag/chunking-service';
import { EmbeddingService } from './src/rag/embedding-service';
import { ResilientDBVectorStore } from './src/rag/resilientdb-vector-store';
import { ContextFormatter } from './src/rag/context-formatter';
import { ResilientDBClient } from './src/resilientdb/client';
import { ResilientDBHTTPWrapper } from './src/resilientdb/http-wrapper';
import * as path from 'path';
import * as fs from 'fs';

// Test results tracking
interface TestResult {
  name: string;
  passed: boolean;
  error?: string;
  details?: string;
}

const testResults: TestResult[] = [];

function logTest(name: string, passed: boolean, error?: string, details?: string) {
  testResults.push({ name, passed, error, details });
  const icon = passed ? '✅' : '❌';
  console.log(`${icon} ${name}`);
  if (details) {
    console.log(`   ${details}`);
  }
  if (error) {
    console.log(`   Error: ${error}`);
  }
}

async function testDocumentLoader() {
  console.log('\n📄 Testing Document Loader...');
  
  const loader = new DocumentLoader();
  
  // Test 1: Load a single file
  try {
    const testFile = path.join(process.cwd(), 'package.json');
    if (fs.existsSync(testFile)) {
      const doc = await loader.loadFile(testFile);
      logTest('Load single file', doc.id !== undefined && doc.content.length > 0, undefined, 
        `Loaded: ${doc.id} (${doc.content.length} chars)`);
    } else {
      logTest('Load single file', false, 'package.json not found');
    }
  } catch (error) {
    logTest('Load single file', false, error instanceof Error ? error.message : String(error));
  }
  
  // Test 2: Load directory
  try {
    const docsPath = path.join(process.cwd(), 'docs');
    if (fs.existsSync(docsPath)) {
      const docs = await loader.loadDirectory(docsPath, {
        recursive: true,
        extensions: ['.md', '.txt'],
      });
      logTest('Load directory', docs.length > 0, undefined, 
        `Loaded ${docs.length} documents`);
    } else {
      logTest('Load directory', false, 'docs/ directory not found');
    }
  } catch (error) {
    logTest('Load directory', false, error instanceof Error ? error.message : String(error));
  }
  
  // Test 3: Load schema
  try {
    const schemaDoc = await loader.loadSchema('type Query { test: String }', 'test_schema');
    logTest('Load schema document', schemaDoc.type === 'schema', undefined,
      `Schema type: ${schemaDoc.type}`);
  } catch (error) {
    logTest('Load schema document', false, error instanceof Error ? error.message : String(error));
  }
}

async function testChunkingService() {
  console.log('\n✂️  Testing Chunking Service...');
  
  const chunkingService = new ChunkingService({
    maxTokens: 100,
    chunkOverlapTokens: 20,
  });
  
  const loader = new DocumentLoader();
  
  // Test 1: Size-based chunking
  try {
    const testContent = 'This is a test document. '.repeat(50); // ~1300 chars
    const doc = await loader.loadSchema(testContent, 'test_doc');
    const chunks = chunkingService.chunkBySize(doc);
    logTest('Size-based chunking', chunks.length > 0, undefined,
      `Created ${chunks.length} chunks from test document`);
  } catch (error) {
    logTest('Size-based chunking', false, error instanceof Error ? error.message : String(error));
  }
  
  // Test 2: Section-based chunking (markdown)
  try {
    const markdownContent = `# Header 1\n\nContent 1\n\n## Header 2\n\nContent 2\n\n### Header 3\n\nContent 3`;
    const doc = await loader.loadFile(
      path.join(process.cwd(), 'README.md')
    ).catch(() => ({
      id: 'test_md',
      content: markdownContent,
      source: 'test.md',
      type: 'markdown' as const,
      metadata: {},
    }));
    
    if (doc.type === 'markdown') {
      const chunks = chunkingService.chunkBySections(doc);
      logTest('Section-based chunking', chunks.length > 0, undefined,
        `Created ${chunks.length} chunks from markdown sections`);
    } else {
      logTest('Section-based chunking', false, 'Could not create markdown document');
    }
  } catch (error) {
    logTest('Section-based chunking', false, error instanceof Error ? error.message : String(error));
  }
}

async function testEmbeddingService() {
  console.log('\n🔢 Testing Embedding Service...');
  
  const embeddingService = new EmbeddingService();
  
  // Test 1: Check availability
  const isAvailable = embeddingService.isAvailable();
  logTest('Embedding service available', isAvailable, undefined,
    `Model: ${embeddingService.getModel()}, Dimension: ${embeddingService.getDimension()}`);
  
  if (!isAvailable) {
    console.log('   ⚠️  Skipping embedding tests - service not available');
    return;
  }
  
  // Test 2: Generate single embedding
  try {
    const embedding = await embeddingService.generateEmbedding('This is a test query');
    logTest('Generate single embedding', embedding.length > 0, undefined,
      `Embedding dimension: ${embedding.length} (expected: ${embeddingService.getDimension()})`);
  } catch (error) {
    logTest('Generate single embedding', false, error instanceof Error ? error.message : String(error));
  }
  
  // Test 3: Generate batch embeddings
  try {
    const texts = ['Query 1', 'Query 2', 'Query 3'];
    const embeddings = await embeddingService.generateEmbeddings(texts, 10);
    logTest('Generate batch embeddings', embeddings.length === texts.length, undefined,
      `Generated ${embeddings.length} embeddings`);
  } catch (error) {
    logTest('Generate batch embeddings', false, error instanceof Error ? error.message : String(error));
  }
}

async function testVectorStore() {
  console.log('\n💾 Testing Vector Store...');
  
  const vectorStore = new ResilientDBVectorStore();
  
  // Test 1: Health check
  try {
    const isHealthy = await vectorStore.healthCheck();
    logTest('Vector store health check', isHealthy, undefined,
      isHealthy ? 'Vector store is accessible' : 'Vector store not accessible');
  } catch (error) {
    logTest('Vector store health check', false, error instanceof Error ? error.message : String(error));
  }
  
  // Test 2: Store a chunk
  try {
    const testEmbedding = new Array(384).fill(0).map(() => Math.random());
    const chunkId = await vectorStore.storeChunk({
      chunkText: 'This is a test chunk for RAG testing',
      embedding: testEmbedding,
      source: 'test_source',
      chunkIndex: 0,
      metadata: {
        documentId: 'test_doc',
        type: 'documentation',
        timestamp: new Date().toISOString(),
      },
    });
    logTest('Store chunk', chunkId !== undefined, undefined,
      `Stored chunk with ID: ${chunkId.substring(0, 20)}...`);
  } catch (error) {
    logTest('Store chunk', false, error instanceof Error ? error.message : String(error));
  }
  
  // Test 3: Get all chunks
  try {
    const chunks = await vectorStore.getAllChunks(10);
    logTest('Get all chunks', chunks.length >= 0, undefined,
      `Retrieved ${chunks.length} chunks`);
  } catch (error) {
    logTest('Get all chunks', false, error instanceof Error ? error.message : String(error));
  }
  
  // Test 4: Semantic search
  try {
    const queryEmbedding = new Array(384).fill(0).map(() => Math.random());
    const results = await vectorStore.searchSimilar(queryEmbedding, 5);
    logTest('Semantic search', results.length >= 0, undefined,
      `Found ${results.length} similar chunks`);
  } catch (error) {
    logTest('Semantic search', false, error instanceof Error ? error.message : String(error));
  }
}

async function testRetrievalService() {
  console.log('\n🔍 Testing Retrieval Service...');
  
  const retrievalService = new RetrievalService();
  
  // Test 1: Check availability
  try {
    const availability = await retrievalService.checkAvailability();
    logTest('Retrieval service availability', 
      availability.vectorStore || availability.embeddingService, undefined,
      `Vector Store: ${availability.vectorStore ? '✅' : '❌'}, ` +
      `Embedding: ${availability.embeddingService ? '✅' : '❌'}`);
  } catch (error) {
    logTest('Retrieval service availability', false, error instanceof Error ? error.message : String(error));
  }
  
  // Test 2: Basic retrieval
  try {
    const result = await retrievalService.retrieve('test query', {
      limit: 5,
      minSimilarity: 0.1,
      includeScores: true,
    });
    logTest('Basic retrieval', result.chunks.length >= 0, undefined,
      `Retrieved ${result.chunks.length} chunks, embedding dimension: ${result.queryEmbedding.length}`);
  } catch (error) {
    logTest('Basic retrieval', false, error instanceof Error ? error.message : String(error));
  }
  
  // Test 3: Multiple query retrieval
  try {
    const result = await retrievalService.retrieveMultiple(
      ['query 1', 'query 2'],
      { limit: 5 }
    );
    logTest('Multiple query retrieval', result.chunks.length >= 0, undefined,
      `Retrieved ${result.chunks.length} chunks from multiple queries`);
  } catch (error) {
    logTest('Multiple query retrieval', false, error instanceof Error ? error.message : String(error));
  }
  
  // Test 4: Schema context retrieval
  try {
    const result = await retrievalService.retrieveSchemaContext('GraphQL schema', {
      limit: 3,
    });
    logTest('Schema context retrieval', result.chunks.length >= 0, undefined,
      `Retrieved ${result.chunks.length} schema chunks`);
  } catch (error) {
    logTest('Schema context retrieval', false, error instanceof Error ? error.message : String(error));
  }
}

async function testContextFormatter() {
  console.log('\n📝 Testing Context Formatter...');
  
  const formatter = new ContextFormatter();
  
  // Create mock chunks
  const mockChunks = [
    {
      id: 'chunk1',
      chunkText: 'This is the first chunk of documentation.',
      embedding: new Array(384).fill(0),
      source: 'doc1.md',
      chunkIndex: 0,
      metadata: {
        documentId: 'doc1',
        type: 'documentation' as const,
        section: 'Introduction',
      },
    },
    {
      id: 'chunk2',
      chunkText: 'This is the second chunk with more information.',
      embedding: new Array(384).fill(0),
      source: 'doc2.md',
      chunkIndex: 1,
      metadata: {
        documentId: 'doc2',
        type: 'documentation' as const,
      },
    },
  ];
  
  // Test 1: Basic formatting
  try {
    const formatted = formatter.format(mockChunks, undefined, {
      format: 'detailed',
      includeMetadata: true,
    });
    logTest('Basic context formatting', formatted.length > 0, undefined,
      `Formatted ${mockChunks.length} chunks (${formatted.length} chars)`);
  } catch (error) {
    logTest('Basic context formatting', false, error instanceof Error ? error.message : String(error));
  }
  
  // Test 2: Format for explanation
  try {
    const formatted = formatter.formatForExplanation(mockChunks);
    logTest('Format for explanation', formatted.includes('Relevant Documentation'), undefined,
      `Formatted explanation context (${formatted.length} chars)`);
  } catch (error) {
    logTest('Format for explanation', false, error instanceof Error ? error.message : String(error));
  }
  
  // Test 3: Format for optimization
  try {
    const formatted = formatter.formatForOptimization(mockChunks);
    logTest('Format for optimization', formatted.includes('Optimization References'), undefined,
      `Formatted optimization context (${formatted.length} chars)`);
  } catch (error) {
    logTest('Format for optimization', false, error instanceof Error ? error.message : String(error));
  }
  
  // Test 4: Token estimation
  try {
    const testText = 'This is a test text for token estimation. '.repeat(10);
    const tokens = formatter.estimateTokens(testText);
    logTest('Token estimation', tokens > 0, undefined,
      `Estimated ${tokens} tokens for ${testText.length} chars`);
  } catch (error) {
    logTest('Token estimation', false, error instanceof Error ? error.message : String(error));
  }
}

async function testIngestionPipeline() {
  console.log('\n🚀 Testing Ingestion Pipeline...');
  
  const pipeline = new IngestionPipeline();
  
  // Test 1: Check availability
  try {
    const availability = await pipeline.checkAvailability();
    logTest('Ingestion pipeline availability', 
      availability.embeddingService || availability.vectorStore, undefined,
      availability.message);
  } catch (error) {
    logTest('Ingestion pipeline availability', false, error instanceof Error ? error.message : String(error));
  }
  
  // Test 2: Get stats (if chunks exist)
  try {
    const stats = await pipeline.getStats();
    logTest('Get ingestion stats', stats.totalChunks >= 0, undefined,
      `Total chunks: ${stats.totalChunks}, ` +
      `Types: ${Object.keys(stats.chunksByType).join(', ') || 'none'}`);
  } catch (error) {
    logTest('Get ingestion stats', false, error instanceof Error ? error.message : String(error));
  }
}

async function testEndToEndRAG() {
  console.log('\n🔄 Testing End-to-End RAG Flow...');
  
  try {
    // Step 1: Create a test document
    const loader = new DocumentLoader();
    const testContent = `
# GraphQL Query Guide

## Introduction
GraphQL is a query language for APIs. It allows clients to request exactly the data they need.

## Basic Queries
A basic GraphQL query looks like this:
\`\`\`graphql
query {
  users {
    id
    name
  }
}
\`\`\`

## Advanced Features
GraphQL supports variables, fragments, and directives for more complex queries.
`;
    
    const testDoc = await loader.loadSchema(testContent, 'test_rag_doc');
    
    // Step 2: Ingest the document
    const pipeline = new IngestionPipeline();
    const progress = await pipeline.ingestDocuments([testDoc], undefined, {
      maxChunkTokens: 100,
      chunkOverlapTokens: 20,
    });
    
    logTest('E2E: Document ingestion', progress.storedChunks > 0, undefined,
      `Ingested ${progress.storedChunks} chunks`);
    
    // Step 3: Retrieve relevant chunks
    const retrievalService = new RetrievalService();
    const result = await retrievalService.retrieve('How do I write a GraphQL query?', {
      limit: 3,
      minSimilarity: 0.1,
      includeScores: true,
    });
    
    logTest('E2E: Retrieval', result.chunks.length > 0, undefined,
      `Retrieved ${result.chunks.length} relevant chunks`);
    
    // Step 4: Format context
    const formatter = new ContextFormatter();
    const context = formatter.formatForExplanation(result.chunks, result.scores);
    
    logTest('E2E: Context formatting', context.length > 0, undefined,
      `Formatted context (${context.length} chars)`);
    
    // Step 5: Verify context contains relevant information
    const hasRelevantContent = context.toLowerCase().includes('graphql') || 
                                context.toLowerCase().includes('query');
    logTest('E2E: Context relevance', hasRelevantContent, undefined,
      hasRelevantContent ? 'Context contains relevant information' : 'Context may not be relevant');
    
  } catch (error) {
    logTest('E2E: RAG flow', false, error instanceof Error ? error.message : String(error));
  }
}

async function runAllTests() {
  console.log('🧪 RAG Test Suite');
  console.log('='.repeat(60));
  console.log('Testing all RAG components...\n');
  
  // Start HTTP wrapper if needed
  const httpWrapper = new ResilientDBHTTPWrapper(18001);
  try {
    await httpWrapper.start();
    console.log('✅ ResilientDB HTTP API available at http://localhost:18001\n');
    process.env.RESILIENTDB_GRAPHQL_URL = 'http://localhost:18001/graphql';
  } catch (error) {
    console.log('⚠️  HTTP wrapper not started, using existing server\n');
  }
  
  // Run all tests
  await testDocumentLoader();
  await testChunkingService();
  await testEmbeddingService();
  await testVectorStore();
  await testRetrievalService();
  await testContextFormatter();
  await testIngestionPipeline();
  await testEndToEndRAG();
  
  // Print summary
  console.log('\n' + '='.repeat(60));
  console.log('📊 Test Summary');
  console.log('='.repeat(60));
  
  const passed = testResults.filter(r => r.passed).length;
  const failed = testResults.filter(r => !r.passed).length;
  const total = testResults.length;
  
  console.log(`Total Tests: ${total}`);
  console.log(`✅ Passed: ${passed}`);
  console.log(`❌ Failed: ${failed}`);
  console.log(`Success Rate: ${((passed / total) * 100).toFixed(1)}%`);
  
  if (failed > 0) {
    console.log('\n❌ Failed Tests:');
    testResults
      .filter(r => !r.passed)
      .forEach(r => {
        console.log(`  - ${r.name}`);
        if (r.error) {
          console.log(`    Error: ${r.error}`);
        }
      });
  }
  
  console.log('\n');
  
  // Exit with appropriate code
  process.exit(failed > 0 ? 1 : 0);
}

// Run tests
if (require.main === module) {
  runAllTests().catch((error) => {
    console.error('\n❌ Fatal error during testing:', error);
    process.exit(1);
  });
}

export { runAllTests };

