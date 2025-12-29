#!/usr/bin/env tsx

/**
 * Test specific questions from the image
 */

import { ExplanationService } from './src/llm/explanation-service';
import { ResilientDBHTTPWrapper } from './src/resilientdb/http-wrapper';

const questions = [
  'How do I write a GraphQL query?',
  'What mutations are available?',
  'How do I use variables in GraphQL?',
  'What is ResilientDB?',
  'How do I filter transactions?',
];

async function testQuestions() {
  console.log('🧪 Testing Specific Questions\n');
  console.log('='.repeat(80));
  console.log();

  // Start HTTP wrapper if needed
  const httpWrapper = new ResilientDBHTTPWrapper(18001);
  try {
    await httpWrapper.start();
  } catch (error) {
    // Suppress error - server might already be running
  }
  process.env.RESILIENTDB_GRAPHQL_URL = 'http://localhost:18001/graphql';

  const explanationService = new ExplanationService();

  for (const question of questions) {
    console.log(`input: ${question}`);
    console.log();

    try {
      const explanation = await explanationService.explain(question, {
        maxContextChunks: 5,
      });

      console.log(`answer: ${explanation.explanation.trim()}`);
      console.log();
      console.log('─'.repeat(80));
      console.log();
    } catch (error) {
      console.log(`answer: Error: ${error instanceof Error ? error.message : String(error)}`);
      console.log();
      console.log('─'.repeat(80));
      console.log();
    }
  }

  console.log('='.repeat(80));
  console.log('✅ Testing complete!');
  console.log('='.repeat(80));
}

if (require.main === module) {
  testQuestions().catch((error) => {
    console.error('\n❌ Fatal error:', error);
    process.exit(1);
  });
}

export { testQuestions };

