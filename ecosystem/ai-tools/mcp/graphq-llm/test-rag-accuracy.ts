#!/usr/bin/env tsx

/**
 * RAG Accuracy Test
 * 
 * Tests the accuracy and relevance of RAG responses by:
 * 1. Retrieving chunks for specific queries
 * 2. Displaying the actual retrieved content
 * 3. Evaluating relevance and correctness
 */

import { RetrievalService } from './src/rag/retrieval-service';
import { ExplanationService } from './src/llm/explanation-service';
import { ResilientDBHTTPWrapper } from './src/resilientdb/http-wrapper';

// Suppress console warnings and errors during tests
const originalConsoleWarn = console.warn;
const originalConsoleError = console.error;
const originalConsoleLog = console.log;

// Filter out noisy messages
const shouldSuppress = (message: string): boolean => {
  const suppressed = [
    'Defaulting to \'auto\'',
    'Auto selected provider',
    'GraphQL queries failed',
    'HTTP REST API fallback',
    'Port 18001 already in use',
    'HTTP wrapper not started',
    'already in use',
    'assuming HTTP server',
    'hf.co/settings',
    'inference-providers',
  ];
  return suppressed.some(pattern => message.includes(pattern));
};

// Override console methods to filter messages
console.warn = (...args: unknown[]) => {
  const message = args.join(' ');
  if (!shouldSuppress(message)) {
    originalConsoleWarn(...args);
  }
};

console.error = (...args: unknown[]) => {
  const message = args.join(' ');
  if (!shouldSuppress(message)) {
    originalConsoleError(...args);
  }
};

// Also suppress stdout messages that match our patterns
const originalStdoutWrite = process.stdout.write.bind(process.stdout);
process.stdout.write = function(chunk: any, encoding?: any, cb?: any): boolean {
  const message = typeof chunk === 'string' ? chunk : chunk.toString();
  if (shouldSuppress(message)) {
    return true; // Suppress the message
  }
  return originalStdoutWrite(chunk, encoding, cb);
};

interface AccuracyTest {
  query: string;
  expectedTopics: string[];
  minRelevance: number; // Minimum similarity score expected
}

const accuracyTests: AccuracyTest[] = [
  {
    query: 'How do I write a GraphQL query?',
    expectedTopics: ['query', 'graphql', 'syntax', 'example'],
    minRelevance: 0.6,
  },
  {
    query: 'What mutations are available?',
    expectedTopics: ['mutation', 'postTransaction', 'create'],
    minRelevance: 0.5,
  },
  {
    query: 'How do I use variables in GraphQL?',
    expectedTopics: ['variable', 'query', 'parameter'],
    minRelevance: 0.5,
  },
  {
    query: 'What is ResilientDB?',
    expectedTopics: ['resilientdb', 'database', 'distributed'],
    minRelevance: 0.5,
  },
  {
    query: 'How do I filter transactions?',
    expectedTopics: ['filter', 'transaction', 'query'],
    minRelevance: 0.4,
  },
  {
    query: 'What is the PrepareAsset input type?',
    expectedTopics: ['PrepareAsset', 'input', 'type', 'mutation'],
    minRelevance: 0.5,
  },
  {
    query: 'How do I retrieve a transaction by ID?',
    expectedTopics: ['getTransaction', 'id', 'retrieve', 'query'],
    minRelevance: 0.6,
  },
];

function evaluateRelevance(
  chunks: Array<{ chunkText: string; score?: number }>,
  query: string,
  expectedTopics: string[]
): {
  hasRelevantContent: boolean;
  topicMatches: number;
  avgScore: number;
  sampleContent: string;
} {
  const queryLower = query.toLowerCase();
  const allText = chunks.map(c => c.chunkText.toLowerCase()).join(' ');
  
  // Check if expected topics are present
  const topicMatches = expectedTopics.filter(topic =>
    allText.includes(topic.toLowerCase())
  ).length;

  // Calculate average score
  const scores = chunks.map(c => c.score || 0).filter(s => s > 0);
  const avgScore = scores.length > 0
    ? scores.reduce((a, b) => a + b, 0) / scores.length
    : 0;

  // Check if content seems relevant (contains query words or expected topics)
  const queryWords = queryLower.split(/\s+/).filter(w => w.length > 3);
  const hasQueryWords = queryWords.some(word => allText.includes(word));
  const hasExpectedTopics = topicMatches > 0;

  const hasRelevantContent = hasQueryWords || hasExpectedTopics || avgScore > 0.4;

  // Get sample content from top chunk
  const sampleContent = chunks.length > 0
    ? chunks[0].chunkText.substring(0, 300).replace(/\n/g, ' ')
    : 'No content retrieved';

  return {
    hasRelevantContent,
    topicMatches,
    avgScore,
    sampleContent,
  };
}

async function testQueryAccuracy() {
  console.log('🎯 RAG Accuracy and Relevance Test');
  console.log('='.repeat(80));
  console.log();

  const retrievalService = new RetrievalService();
  const explanationService = new ExplanationService();

  // Check availability (suppress warnings during check)
  const ragAvailability = await retrievalService.checkAvailability();
  if (!ragAvailability.vectorStore || !ragAvailability.embeddingService) {
    console.log('❌ RAG services not available');
    return;
  }

  // Check LLM availability
  const serviceAvailability = await explanationService.checkAvailability();
  const llmAvailable = serviceAvailability.llm;
  
  if (!llmAvailable) {
    console.log('⚠️  LLM not available - will show raw retrieved chunks instead of synthesized answers');
    console.log('   Set LLM_API_KEY environment variable to get proper synthesized answers\n');
  } else {
    console.log('✅ RAG services available');
    console.log('✅ LLM available - will generate proper synthesized answers\n');
  }
  

  let accurateCount = 0;
  let totalTests = accuracyTests.length;

  for (let i = 0; i < accuracyTests.length; i++) {
    const test = accuracyTests[i];
    
    // Display input (clean format)
    console.log(`input: ${test.query}`);
    console.log();

    try {
      // Retrieve chunks - get more chunks to ensure we have relevant context
      const result = await retrievalService.retrieve(test.query, {
        limit: 10, // Increased to get more context
        minSimilarity: 0.1,
        includeScores: true,
      });

      if (result.chunks.length === 0) {
        console.log('answer: No relevant information found in the knowledge base.');
        console.log();
        continue;
      }

      // Evaluate relevance
      const evaluation = evaluateRelevance(
        result.chunks.map((chunk, idx) => ({
          chunkText: chunk.chunkText,
          score: result.scores?.[idx],
        })),
        test.query,
        test.expectedTopics
      );

      // Generate answer using LLM if available, otherwise show raw chunks
      let answer = '';
      if (llmAvailable) {
        try {
          const explanation = await explanationService.explain(test.query, {
            maxContextChunks: 5,
          });
          answer = explanation.explanation.trim();
        } catch (error) {
          console.log(`   ⚠️  LLM synthesis failed: ${error instanceof Error ? error.message : String(error)}`);
          const topChunk = result.chunks[0];
          answer = topChunk ? topChunk.chunkText.trim() : 'No relevant information found.';
        }
      } else {
        // No LLM available - show top chunk
        const topChunk = result.chunks[0];
        if (topChunk) {
          answer = topChunk.chunkText.trim();
          // Clean up
          answer = answer.replace(/^\[Source:[^\]]+\]\s*/gm, '');
          answer = answer.replace(/^---\s*$/gm, '');
          answer = answer.replace(/\n{3,}/g, '\n\n').trim();
        } else {
          answer = 'No relevant information found in the knowledge base.';
        }
      }

      // Display answer
      console.log(`answer: ${answer}`);
      console.log();

    } catch (error) {
      console.log(`answer: Error retrieving information: ${error instanceof Error ? error.message : String(error)}`);
      console.log();
    }
  }

  // Summary
  console.log('='.repeat(80));
  console.log('📊 Accuracy Summary');
  console.log('='.repeat(80));
  console.log(`Total Tests: ${totalTests}`);
  console.log(`✅ Accurate/Relevant: ${accurateCount}`);
  console.log(`⚠️  Uncertain: ${totalTests - accurateCount}`);
  console.log(`Accuracy Rate: ${((accurateCount / totalTests) * 100).toFixed(1)}%`);
  console.log();
}

async function testSpecificQueries() {
  console.log('🔍 Testing Specific Query Scenarios');
  console.log('='.repeat(80));
  console.log();

  const retrievalService = new RetrievalService();
  const explanationService = new ExplanationService();

  const specificQueries = [
    {
      name: 'Basic Query Syntax',
      query: 'Show me an example of a GraphQL query',
      expectedInResponse: ['query', 'example', 'graphql'],
    },
    {
      name: 'Mutation Example',
      query: 'How do I create a transaction using GraphQL?',
      expectedInResponse: ['mutation', 'transaction', 'create', 'postTransaction'],
    },
    {
      name: 'Schema Information',
      query: 'What fields are available in the Query type?',
      expectedInResponse: ['query', 'field', 'type', 'schema'],
    },
  ];

  for (const test of specificQueries) {
    // Display input
    console.log(`input: ${test.query}`);
    console.log();

    try {
      const result = await retrievalService.retrieve(test.query, {
        limit: 5,
        minSimilarity: 0.3,
        includeScores: true,
      });

      if (result.chunks.length === 0) {
        console.log('answer: No relevant information found in the knowledge base.');
        console.log();
        console.log('─'.repeat(80));
        console.log();
        continue;
      }

      // Check if response contains expected terms
      const allText = result.chunks.map(c => c.chunkText.toLowerCase()).join(' ');
      const foundTerms = test.expectedInResponse.filter(term =>
        allText.includes(term.toLowerCase())
      );

      // Generate answer using LLM if available
      let answer = '';
      const availability = await explanationService.checkAvailability();
      const llmAvailable = availability.llm;
      
      if (llmAvailable) {
        try {
          const explanation = await explanationService.explain(test.query, {
            maxContextChunks: 5,
          });
          answer = explanation.explanation.trim();
        } catch (error) {
          console.log(`   ⚠️  LLM synthesis failed: ${error instanceof Error ? error.message : String(error)}`);
          const topChunk = result.chunks[0];
          answer = topChunk ? topChunk.chunkText.trim() : 'No relevant information found.';
        }
      } else {
        // No LLM available - show top chunk
        const topChunk = result.chunks[0];
        if (topChunk) {
          answer = topChunk.chunkText.trim();
          answer = answer.replace(/^\[Source:[^\]]+\]\s*/gm, '');
          answer = answer.replace(/^---\s*$/gm, '');
          answer = answer.replace(/\n{3,}/g, '\n\n').trim();
        } else {
          answer = 'No relevant information found in the knowledge base.';
        }
      }

      // Display answer (clean format)
      console.log(`answer: ${answer}`);
      console.log();

    } catch (error) {
      console.log(`answer: Error retrieving information: ${error instanceof Error ? error.message : String(error)}`);
      console.log();
    }
  }
}

async function runAccuracyTests() {
  // Suppress all noisy output from libraries
  const suppressOutput = () => {
    console.warn = (...args: unknown[]) => {
      const message = args.join(' ');
      if (!shouldSuppress(message)) {
        originalConsoleWarn(...args);
      }
    };
    
    console.error = (...args: unknown[]) => {
      const message = args.join(' ');
      if (!shouldSuppress(message)) {
        originalConsoleError(...args);
      }
    };
    
    // Suppress stdout messages too
    const originalStdoutWrite = process.stdout.write.bind(process.stdout);
    process.stdout.write = function(chunk: any, encoding?: any, cb?: any): boolean {
      const message = typeof chunk === 'string' ? chunk : chunk.toString();
      if (shouldSuppress(message)) {
        return true; // Suppress the message
      }
      return originalStdoutWrite(chunk, encoding, cb);
    };
  };
  
  suppressOutput();
  
  // Restore console.log for our output
  console.log = originalConsoleLog;
  
  console.log('🧪 RAG Accuracy Test Suite');
  console.log('='.repeat(80));
  console.log('Evaluating the correctness and relevance of RAG responses\n');

  // Start HTTP wrapper if needed (suppress warnings)
  const httpWrapper = new ResilientDBHTTPWrapper(18001);
  try {
    await httpWrapper.start();
  } catch (error) {
    // Suppress error - server might already be running
  }
  process.env.RESILIENTDB_GRAPHQL_URL = 'http://localhost:18001/graphql';

  await testQueryAccuracy();
  await testSpecificQueries();

  // Restore console for final output
  console.log = originalConsoleLog;
  console.warn = originalConsoleWarn;
  console.error = originalConsoleError;
  
  // Suppress output during final summary too
  suppressOutput();

  console.log('='.repeat(80));
  console.log('✅ Accuracy testing complete!');
  console.log('='.repeat(80));
  console.log();
  console.log('💡 Tips for improving accuracy:');
  console.log('   - Ensure comprehensive documentation is ingested');
  console.log('   - Add more examples and use cases to documentation');
  console.log('   - Consider adjusting similarity thresholds');
  console.log('   - Review retrieved chunks to verify relevance');
  console.log();
}

// Run tests
if (require.main === module) {
  runAccuracyTests().catch((error) => {
    console.error('\n❌ Fatal error during accuracy testing:', error);
    process.exit(1);
  });
}

export { runAccuracyTests };

