#!/usr/bin/env tsx

/**
 * Test Nexus Integration
 * 
 * Tests the connection and functionality of Nexus integration
 */

import { NexusIntegration } from './src/nexus/integration';

async function testNexus() {
  console.log('🧪 Testing Nexus Integration...\n');
  
  const nexus = new NexusIntegration();
  
  // Check if enabled
  console.log('📋 Configuration:');
  console.log(`   Enabled: ${nexus.isEnabled() ? '✅ Yes' : '❌ No'}`);
  console.log(`   API URL: ${nexus.getApiUrl()}`);
  console.log();
  
  // Check connection
  console.log('1️⃣  Checking Nexus connection...');
  const health = await nexus.checkConnection();
  
  if (health.connected) {
    console.log('   ✅ Nexus is connected!');
    console.log(`   API URL: ${health.apiUrl}`);
  } else {
    console.log('   ❌ Nexus is not connected');
    console.log(`   API URL: ${health.apiUrl}`);
    if (health.error) {
      console.log(`   Error: ${health.error}`);
    }
    console.log();
    console.log('⚠️  To fix this:');
    console.log('   1. Start Nexus: cd nexus && npm run dev');
    console.log('   2. Verify it runs on http://localhost:3000');
    console.log('   3. Check NEXUS_API_URL in .env file');
    console.log('   4. Test: curl http://localhost:3000/api/research/documents');
    return;
  }
  console.log();
  
  // Test query analysis
  console.log('2️⃣  Testing query analysis...');
  const testQuery = {
    query: '{ getTransaction(id: "123") { asset } }',
    operationName: 'GetTransaction',
  };
  
  try {
    console.log(`   Query: ${testQuery.query}`);
    const analysis = await nexus.analyzeQuery(testQuery, {
      schema: 'GraphQL schema context for ResilientDB...',
    });
    
    console.log('   ✅ Analysis received:');
    console.log(`   Explanation: ${analysis.explanation.substring(0, 150)}${analysis.explanation.length > 150 ? '...' : ''}`);
    console.log(`   Complexity: ${analysis.complexity}`);
    console.log(`   Efficiency: ${analysis.estimatedEfficiency}/100`);
    console.log(`   Recommendations: ${analysis.recommendations.length} items`);
    if (analysis.recommendations.length > 0) {
      analysis.recommendations.slice(0, 2).forEach((rec, i) => {
        console.log(`     ${i + 1}. ${rec.substring(0, 80)}${rec.length > 80 ? '...' : ''}`);
      });
    }
  } catch (error) {
    console.log('   ❌ Analysis failed:');
    console.log(`   Error: ${error instanceof Error ? error.message : String(error)}`);
    if (error instanceof Error && error.stack) {
      console.log(`   Stack: ${error.stack.split('\n')[1]}`);
    }
  }
  console.log();
  
  // Test query suggestions
  console.log('3️⃣  Testing query suggestions...');
  try {
    const suggestions = await nexus.getSuggestions(testQuery, {
      schema: 'GraphQL schema context...',
    });
    
    console.log('   ✅ Suggestions received:');
    console.log(`   Count: ${suggestions.length}`);
    if (suggestions.length > 0) {
      const first = suggestions[0];
      console.log(`   First suggestion:`);
      console.log(`     Query: ${first.query.substring(0, 80)}${first.query.length > 80 ? '...' : ''}`);
      console.log(`     Explanation: ${first.explanation.substring(0, 100)}${first.explanation.length > 100 ? '...' : ''}`);
      console.log(`     Confidence: ${(first.confidence * 100).toFixed(0)}%`);
    }
  } catch (error) {
    console.log('   ❌ Suggestions failed:');
    console.log(`   Error: ${error instanceof Error ? error.message : String(error)}`);
  }
  console.log();
  
  console.log('✅ Nexus integration test complete!\n');
  console.log('📝 Summary:');
  console.log(`   Connection: ${health.connected ? '✅ Working' : '❌ Failed'}`);
  console.log(`   Query Analysis: ${health.connected ? '✅ Tested' : '⏭️  Skipped'}`);
  console.log(`   Query Suggestions: ${health.connected ? '✅ Tested' : '⏭️  Skipped'}`);
}

testNexus().catch((error) => {
  console.error('❌ Test failed:', error);
  process.exit(1);
});

