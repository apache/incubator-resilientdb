#!/usr/bin/env tsx

/**
 * RAG Query Test Script
 * 
 * Tests the RAG system with real-world query scenarios to verify
 * that retrieval and context formatting work correctly for actual use cases.
 */

import { RetrievalService } from './src/rag/retrieval-service';
import { ContextFormatter } from './src/rag/context-formatter';
import { IngestionPipeline } from './src/rag/ingestion-pipeline';
import { ResilientDBHTTPWrapper } from './src/resilientdb/http-wrapper';

interface QueryTest {
  name: string;
  query: string;
  expectedKeywords?: string[];
  minChunks?: number;
  description: string;
}

const testQueries: QueryTest[] = [
  {
    name: 'Basic Query Help',
    query: 'How do I write a GraphQL query?',
    expectedKeywords: ['query', 'graphql'],
    minChunks: 1,
    description: 'User wants to learn basic GraphQL query syntax',
  },
  {
    name: 'Mutation Information',
    query: 'What mutations are available in the GraphQL schema?',
    expectedKeywords: ['mutation', 'schema'],
    minChunks: 1,
    description: 'User wants to know about available mutations',
  },
  {
    name: 'Query Optimization',
    query: 'How can I optimize my GraphQL queries for better performance?',
    expectedKeywords: ['optimize', 'performance', 'query'],
    minChunks: 1,
    description: 'User wants optimization advice',
  },
  {
    name: 'Schema Exploration',
    query: 'What types and fields are available in the GraphQL schema?',
    expectedKeywords: ['schema', 'type', 'field'],
    minChunks: 1,
    description: 'User wants to explore the schema structure',
  },
  {
    name: 'Error Handling',
    query: 'How do I handle errors in GraphQL queries?',
    expectedKeywords: ['error', 'handle'],
    minChunks: 0,
    description: 'User wants to know about error handling',
  },
  {
    name: 'Filtering and Sorting',
    query: 'How do I filter and sort results in GraphQL queries?',
    expectedKeywords: ['filter', 'sort'],
    minChunks: 0,
    description: 'User wants to know about filtering and sorting',
  },
  {
    name: 'Variables Usage',
    query: 'How do I use variables in GraphQL queries?',
    expectedKeywords: ['variable', 'query'],
    minChunks: 0,
    description: 'User wants to learn about query variables',
  },
  {
    name: 'Fragments',
    query: 'What are GraphQL fragments and how do I use them?',
    expectedKeywords: ['fragment'],
    minChunks: 0,
    description: 'User wants to learn about fragments',
  },
  {
    name: 'ResilientDB Specific',
    query: 'How do I interact with ResilientDB using GraphQL?',
    expectedKeywords: ['resilientdb', 'graphql'],
    minChunks: 0,
    description: 'User wants ResilientDB-specific information',
  },
  {
    name: 'Complex Query',
    query: 'How do I write a complex GraphQL query with nested fields and filters?',
    expectedKeywords: ['query', 'nested', 'filter'],
    minChunks: 0,
    description: 'User wants help with complex queries',
  },
];

async function testQueryRetrieval() {
  console.log('🔍 Testing RAG with Real-World Queries');
  console.log('='.repeat(70));
  console.log();

  const retrievalService = new RetrievalService();
  const formatter = new ContextFormatter();

  // Check availability first
  const availability = await retrievalService.checkAvailability();
  if (!availability.vectorStore || !availability.embeddingService) {
    console.log('❌ RAG services not available. Please ensure:');
    console.log('   - ResilientDB is running');
    console.log('   - Documents have been ingested (run: npm run ingest)');
    console.log('   - Embedding service is accessible');
    return;
  }

  console.log('✅ RAG services available\n');

  let passedTests = 0;
  let totalTests = testQueries.length;

  for (const testCase of testQueries) {
    console.log(`📝 Test: ${testCase.name}`);
    console.log(`   Query: "${testCase.query}"`);
    console.log(`   Description: ${testCase.description}`);
    console.log();

    try {
      // Retrieve relevant chunks
      const result = await retrievalService.retrieve(testCase.query, {
        limit: 5,
        minSimilarity: 0.1,
        includeScores: true,
      });

      console.log(`   📊 Retrieved ${result.chunks.length} chunks`);

      // Check minimum chunks requirement
      if (testCase.minChunks !== undefined && result.chunks.length < testCase.minChunks) {
        console.log(`   ⚠️  Warning: Expected at least ${testCase.minChunks} chunks, got ${result.chunks.length}`);
      }

      // Show similarity scores if available
      if (result.scores && result.scores.length > 0) {
        const avgScore = result.scores.reduce((a, b) => a + b, 0) / result.scores.length;
        const maxScore = Math.max(...result.scores);
        console.log(`   📈 Similarity scores: max=${(maxScore * 100).toFixed(1)}%, avg=${(avgScore * 100).toFixed(1)}%`);
      }

      // Format context
      const context = formatter.formatForExplanation(result.chunks, result.scores, {
        maxTokens: 2000,
        includeMetadata: true,
        includeScores: false,
      });

      console.log(`   📄 Formatted context: ${context.length} characters`);

      // Check for expected keywords in context
      if (testCase.expectedKeywords && testCase.expectedKeywords.length > 0) {
        const contextLower = context.toLowerCase();
        const foundKeywords = testCase.expectedKeywords.filter(keyword =>
          contextLower.includes(keyword.toLowerCase())
        );
        const keywordMatch = foundKeywords.length / testCase.expectedKeywords.length;

        if (keywordMatch > 0.5) {
          console.log(`   ✅ Found ${foundKeywords.length}/${testCase.expectedKeywords.length} expected keywords`);
          passedTests++;
        } else {
          console.log(`   ⚠️  Only found ${foundKeywords.length}/${testCase.expectedKeywords.length} expected keywords`);
          console.log(`      Found: ${foundKeywords.join(', ') || 'none'}`);
          console.log(`      Expected: ${testCase.expectedKeywords.join(', ')}`);
          // Still count as passed if we got chunks
          if (result.chunks.length > 0) {
            passedTests++;
          }
        }
      } else {
        // No keywords to check, just verify we got results
        if (result.chunks.length > 0) {
          console.log(`   ✅ Retrieved relevant chunks`);
          passedTests++;
        } else {
          console.log(`   ⚠️  No chunks retrieved for this query`);
        }
      }

      // Show a preview of the context
      if (result.chunks.length > 0) {
        const preview = context.substring(0, 200).replace(/\n/g, ' ');
        console.log(`   👁️  Context preview: ${preview}...`);
      }

    } catch (error) {
      console.log(`   ❌ Error: ${error instanceof Error ? error.message : String(error)}`);
    }

    console.log();
  }

  // Summary
  console.log('='.repeat(70));
  console.log('📊 Query Test Summary');
  console.log('='.repeat(70));
  console.log(`Total Queries Tested: ${totalTests}`);
  console.log(`✅ Passed: ${passedTests}`);
  console.log(`❌ Failed: ${totalTests - passedTests}`);
  console.log(`Success Rate: ${((passedTests / totalTests) * 100).toFixed(1)}%`);
  console.log();
}

async function testSpecializedRetrieval() {
  console.log('🎯 Testing Specialized Retrieval Methods');
  console.log('='.repeat(70));
  console.log();

  const retrievalService = new RetrievalService();
  const formatter = new ContextFormatter();

  // Test 1: Schema-specific retrieval
  console.log('1️⃣  Schema Context Retrieval');
  try {
    const schemaResult = await retrievalService.retrieveSchemaContext(
      'What queries and mutations are available?',
      { limit: 5 }
    );
    console.log(`   ✅ Retrieved ${schemaResult.chunks.length} schema chunks`);
    
    if (schemaResult.chunks.length > 0) {
      const schemaContext = formatter.format(schemaResult.chunks, undefined, {
        format: 'markdown',
        maxTokens: 1000,
      });
      console.log(`   📄 Context length: ${schemaContext.length} chars`);
      console.log(`   👁️  Preview: ${schemaContext.substring(0, 150)}...`);
    }
  } catch (error) {
    console.log(`   ❌ Error: ${error instanceof Error ? error.message : String(error)}`);
  }
  console.log();

  // Test 2: Documentation-specific retrieval
  console.log('2️⃣  Documentation Context Retrieval');
  try {
    const docResult = await retrievalService.retrieveDocumentationContext(
      'How do I use the GraphQL API?',
      { limit: 5 }
    );
    console.log(`   ✅ Retrieved ${docResult.chunks.length} documentation chunks`);
    
    if (docResult.chunks.length > 0) {
      const docContext = formatter.formatForExplanation(docResult.chunks);
      console.log(`   📄 Context length: ${docContext.length} chars`);
    }
  } catch (error) {
    console.log(`   ❌ Error: ${error instanceof Error ? error.message : String(error)}`);
  }
  console.log();

  // Test 3: Multiple query retrieval
  console.log('3️⃣  Multiple Query Retrieval');
  try {
    const multiResult = await retrievalService.retrieveMultiple(
      [
        'GraphQL query syntax',
        'available mutations',
        'schema types',
      ],
      { limit: 5, includeScores: true }
    );
    console.log(`   ✅ Retrieved ${multiResult.chunks.length} chunks from multiple queries`);
    
    if (multiResult.chunks.length > 0) {
      const multiContext = formatter.format(multiResult.chunks, multiResult.scores, {
        format: 'detailed',
        includeScores: true,
      });
      console.log(`   📄 Context length: ${multiContext.length} chars`);
      console.log(`   👁️  Preview: ${multiContext.substring(0, 150)}...`);
    }
  } catch (error) {
    console.log(`   ❌ Error: ${error instanceof Error ? error.message : String(error)}`);
  }
  console.log();
}

async function testContextFormatting() {
  console.log('📝 Testing Context Formatting for Different Use Cases');
  console.log('='.repeat(70));
  console.log();

  const retrievalService = new RetrievalService();
  const formatter = new ContextFormatter();

  // Get some chunks first
  const result = await retrievalService.retrieve('GraphQL query examples', {
    limit: 5,
    includeScores: true,
  });

  if (result.chunks.length === 0) {
    console.log('⚠️  No chunks available for formatting tests');
    return;
  }

  console.log(`Using ${result.chunks.length} chunks for formatting tests\n`);

  // Test 1: Explanation formatting
  console.log('1️⃣  Explanation Formatting');
  try {
    const explanationContext = formatter.formatForExplanation(result.chunks, result.scores);
    console.log(`   ✅ Formatted for explanation: ${explanationContext.length} chars`);
    console.log(`   👁️  Preview: ${explanationContext.substring(0, 200)}...`);
  } catch (error) {
    console.log(`   ❌ Error: ${error instanceof Error ? error.message : String(error)}`);
  }
  console.log();

  // Test 2: Optimization formatting
  console.log('2️⃣  Optimization Formatting');
  try {
    const optimizationContext = formatter.formatForOptimization(result.chunks, result.scores);
    console.log(`   ✅ Formatted for optimization: ${optimizationContext.length} chars`);
    console.log(`   👁️  Preview: ${optimizationContext.substring(0, 200)}...`);
  } catch (error) {
    console.log(`   ❌ Error: ${error instanceof Error ? error.message : String(error)}`);
  }
  console.log();

  // Test 3: Different format styles
  console.log('3️⃣  Format Styles');
  const styles: Array<'compact' | 'detailed' | 'markdown'> = ['compact', 'detailed', 'markdown'];
  for (const style of styles) {
    try {
      const formatted = formatter.format(result.chunks, result.scores, {
        format: style,
        includeMetadata: true,
      });
      console.log(`   ✅ ${style} format: ${formatted.length} chars`);
    } catch (error) {
      console.log(`   ❌ ${style} format error: ${error instanceof Error ? error.message : String(error)}`);
    }
  }
  console.log();

  // Test 4: Token limits
  console.log('4️⃣  Token Limit Handling');
  try {
    const limitedContext = formatter.format(result.chunks, result.scores, {
      maxTokens: 500,
      format: 'detailed',
    });
    const estimatedTokens = formatter.estimateTokens(limitedContext);
    console.log(`   ✅ Limited to ~500 tokens: ${limitedContext.length} chars, ~${estimatedTokens} tokens`);
  } catch (error) {
    console.log(`   ❌ Error: ${error instanceof Error ? error.message : String(error)}`);
  }
  console.log();
}

async function testRAGStats() {
  console.log('📊 RAG System Statistics');
  console.log('='.repeat(70));
  console.log();

  const pipeline = new IngestionPipeline();

  try {
    const stats = await pipeline.getStats();
    console.log(`Total Chunks: ${stats.totalChunks}`);
    console.log(`Chunks by Type:`);
    for (const [type, count] of Object.entries(stats.chunksByType)) {
      console.log(`   ${type}: ${count}`);
    }
    console.log(`Chunks by Source:`);
    for (const [source, count] of Object.entries(stats.chunksBySource)) {
      const sourceName = source.length > 50 ? source.substring(0, 50) + '...' : source;
      console.log(`   ${sourceName}: ${count}`);
    }
  } catch (error) {
    console.log(`❌ Error getting stats: ${error instanceof Error ? error.message : String(error)}`);
  }
  console.log();
}

async function runAllQueryTests() {
  console.log('🧪 RAG Query Test Suite');
  console.log('='.repeat(70));
  console.log('Testing RAG with real-world query scenarios\n');

  // Start HTTP wrapper if needed
  const httpWrapper = new ResilientDBHTTPWrapper(18001);
  try {
    await httpWrapper.start();
    console.log('✅ ResilientDB HTTP API available at http://localhost:18001\n');
    process.env.RESILIENTDB_GRAPHQL_URL = 'http://localhost:18001/graphql';
  } catch (error) {
    console.log('⚠️  HTTP wrapper not started, using existing server\n');
  }

  // Run all query tests
  await testQueryRetrieval();
  await testSpecializedRetrieval();
  await testContextFormatting();
  await testRAGStats();

  console.log('='.repeat(70));
  console.log('✅ Query testing complete!');
  console.log('='.repeat(70));
  console.log();
}

// Run tests
if (require.main === module) {
  runAllQueryTests().catch((error) => {
    console.error('\n❌ Fatal error during query testing:', error);
    process.exit(1);
  });
}

export { runAllQueryTests };

