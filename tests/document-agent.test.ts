/**
 * DocumentAgent Test Suite
 * 
 * This file demonstrates how to use the DocumentAgent class and serves as
 * both documentation and functional testing examples.
 */

import { DocumentAgent, createDocumentAgent } from "../src/lib/agents/document-agent";
import { configureLlamaSettings } from "../src/lib/config/llama-settings";
import { documentIndexManager } from "../src/lib/document-index-manager";

// Mock document paths for testing
const TEST_DOCUMENTS = {
  RESILIENTDB: "documents/resilientdb.pdf",
  BLOCKCHAIN_TRANSACTION: "documents/bchain-transaction-pro.pdf",
  RCC: "documents/rcc.pdf"
};

/**
 * Test: Basic DocumentAgent Creation and Configuration
 */
async function testDocumentAgentCreation() {
  console.log("\n=== Testing DocumentAgent Creation ===");
  
  try {
    // Configure settings first
    configureLlamaSettings();
    
    // Get a document index (assuming it's already prepared)
    const index = await documentIndexManager.getIndex(TEST_DOCUMENTS.RESILIENTDB);
    
    if (!index) {
      throw new Error("Document index not found. Please prepare the index first.");
    }
    
    // Create DocumentAgent with basic configuration
    const agent = new DocumentAgent({
      documentPath: TEST_DOCUMENTS.RESILIENTDB,
      index,
      displayName: "ResilientDB Documentation",
      description: "Contains comprehensive information about Apache ResilientDB blockchain platform",
      useReranking: false // Disabled for faster testing
    });
    
    // Test metadata retrieval
    const metadata = agent.getMetadata();
    console.log("✅ Agent created successfully");
    console.log("📋 Metadata:", {
      name: metadata.name,
      displayName: metadata.displayName,
      documentPath: metadata.documentPath
    });
    
    // Test configuration retrieval
    const config = agent.getConfig();
    console.log("⚙️ Configuration:", {
      useReranking: config.useReranking,
      displayName: config.displayName
    });
    
    return agent;
    
  } catch (error) {
    console.error("❌ DocumentAgent creation failed:", error);
    throw error;
  }
}

/**
 * Test: DocumentAgent with Reranking Enabled
 */
async function testDocumentAgentWithReranking() {
  console.log("\n=== Testing DocumentAgent with Reranking ===");
  
  try {
    const index = await documentIndexManager.getIndex(TEST_DOCUMENTS.BLOCKCHAIN_TRANSACTION);
    
    if (!index) {
      throw new Error("Document index not found for blockchain transaction document");
    }
    
    // Create DocumentAgent with reranking enabled
    const agent = await createDocumentAgent(
      TEST_DOCUMENTS.BLOCKCHAIN_TRANSACTION,
      index,
      {
        displayName: "Blockchain Transaction Processing",
        useReranking: true,
        rerankingConfig: {
          topK: 5,
          minScore: 0.2,
          verbose: true
        }
      }
    );
    
    console.log("✅ Agent with reranking created successfully");
    
    const config = agent.getConfig();
    console.log("⚙️ Reranking config:", config.rerankingConfig);
    
    return agent;
    
  } catch (error) {
    console.error("❌ DocumentAgent with reranking creation failed:", error);
    throw error;
  }
}

/**
 * Test: DocumentAgent Query Execution
 */
async function testDocumentAgentQuery(agent: DocumentAgent, query: string) {
  console.log(`\n=== Testing Query: "${query}" ===`);
  
  try {
    const startTime = Date.now();
    
    // Get the tool and execute query
    const tool = agent.getTool();
    const response = await tool.call({ query });
    
    const duration = Date.now() - startTime;
    
    console.log("✅ Query executed successfully");
    console.log(`⏱️ Duration: ${duration}ms`);
    console.log("📄 Response preview:", 
      typeof response === 'string' 
        ? response.substring(0, 200) + "..."
        : JSON.stringify(response).substring(0, 200) + "..."
    );
    
    return response;
    
  } catch (error) {
    console.error("❌ Query execution failed:", error);
    throw error;
  }
}

/**
 * Test: Multiple DocumentAgents Creation
 */
async function testMultipleDocumentAgents() {
  console.log("\n=== Testing Multiple DocumentAgents ===");
  
  try {
    const agents = new Map<string, DocumentAgent>();
    
    // Create agents for all test documents
    for (const [name, path] of Object.entries(TEST_DOCUMENTS)) {
      const index = await documentIndexManager.getIndex(path);
      
      if (index) {
        const agent = await createDocumentAgent(path, index, {
          displayName: name.replace(/_/g, ' '),
          useReranking: false // Disabled for faster testing
        });
        
        agents.set(path, agent);
        console.log(`✅ Created agent for ${name}`);
      } else {
        console.log(`⚠️ Skipping ${name} - index not found`);
      }
    }
    
    console.log(`📊 Total agents created: ${agents.size}`);
    
    // Test each agent with a simple query
    const testQuery = "What is this document about?";
    
    for (const [path, agent] of agents) {
      try {
        console.log(`\n🔍 Testing agent: ${agent.getDisplayName()}`);
        await testDocumentAgentQuery(agent, testQuery);
      } catch (error) {
        console.error(`❌ Failed to query ${path}:`, error);
      }
    }
    
    return agents;
    
  } catch (error) {
    console.error("❌ Multiple agents creation failed:", error);
    throw error;
  }
}

/**
 * Test: DocumentAgent Error Handling
 */
async function testDocumentAgentErrorHandling() {
  console.log("\n=== Testing Error Handling ===");
  
  try {
    const index = await documentIndexManager.getIndex(TEST_DOCUMENTS.RESILIENTDB);
    
    if (!index) {
      throw new Error("Document index not found");
    }
    
    const agent = new DocumentAgent({
      documentPath: TEST_DOCUMENTS.RESILIENTDB,
      index,
      displayName: "Test Agent"
    });
    
    // Test with empty query
    try {
      await testDocumentAgentQuery(agent, "");
      console.log("⚠️ Empty query should have failed");
    } catch (error) {
      console.log("✅ Empty query properly handled:", error.message);
    }
    
    // Test with very long query
    const longQuery = "What is ".repeat(1000) + "ResilientDB?";
    try {
      await testDocumentAgentQuery(agent, longQuery);
      console.log("✅ Long query handled successfully");
    } catch (error) {
      console.log("⚠️ Long query failed:", error.message);
    }
    
  } catch (error) {
    console.error("❌ Error handling test failed:", error);
  }
}

/**
 * Test: DocumentAgent Performance Benchmarking
 */
async function testDocumentAgentPerformance() {
  console.log("\n=== Testing Performance ===");
  
  try {
    const index = await documentIndexManager.getIndex(TEST_DOCUMENTS.RESILIENTDB);
    
    if (!index) {
      throw new Error("Document index not found");
    }
    
    // Test without reranking
    const agentNoReranking = new DocumentAgent({
      documentPath: TEST_DOCUMENTS.RESILIENTDB,
      index,
      displayName: "Performance Test (No Reranking)",
      useReranking: false
    });
    
    // Test with reranking
    const agentWithReranking = new DocumentAgent({
      documentPath: TEST_DOCUMENTS.RESILIENTDB,
      index,
      displayName: "Performance Test (With Reranking)",
      useReranking: true,
      rerankingConfig: {
        topK: 5,
        minScore: 0.1
      }
    });
    
    const testQuery = "How does ResilientDB handle consensus?";
    const iterations = 3;
    
    // Benchmark without reranking
    console.log("\n🚀 Benchmarking without reranking...");
    const timesNoReranking: number[] = [];
    
    for (let i = 0; i < iterations; i++) {
      const startTime = Date.now();
      await agentNoReranking.getTool().call({ query: testQuery });
      const duration = Date.now() - startTime;
      timesNoReranking.push(duration);
      console.log(`  Run ${i + 1}: ${duration}ms`);
    }
    
    // Benchmark with reranking
    console.log("\n🎯 Benchmarking with reranking...");
    const timesWithReranking: number[] = [];
    
    for (let i = 0; i < iterations; i++) {
      const startTime = Date.now();
      await agentWithReranking.getTool().call({ query: testQuery });
      const duration = Date.now() - startTime;
      timesWithReranking.push(duration);
      console.log(`  Run ${i + 1}: ${duration}ms`);
    }
    
    // Calculate averages
    const avgNoReranking = timesNoReranking.reduce((a, b) => a + b, 0) / iterations;
    const avgWithReranking = timesWithReranking.reduce((a, b) => a + b, 0) / iterations;
    
    console.log("\n📊 Performance Summary:");
    console.log(`  Without reranking: ${avgNoReranking.toFixed(2)}ms average`);
    console.log(`  With reranking: ${avgWithReranking.toFixed(2)}ms average`);
    console.log(`  Overhead: ${(avgWithReranking - avgNoReranking).toFixed(2)}ms (${((avgWithReranking / avgNoReranking - 1) * 100).toFixed(1)}%)`);
    
  } catch (error) {
    console.error("❌ Performance test failed:", error);
  }
}

/**
 * Main test runner
 */
export async function runDocumentAgentTests() {
  console.log("🧪 Starting DocumentAgent Test Suite");
  console.log("=====================================");
  
  try {
    // Test 1: Basic creation
    const basicAgent = await testDocumentAgentCreation();
    
    // Test 2: Reranking configuration
    const rerankingAgent = await testDocumentAgentWithReranking();
    
    // Test 3: Query execution
    await testDocumentAgentQuery(basicAgent, "What is ResilientDB?");
    await testDocumentAgentQuery(basicAgent, "How does the consensus mechanism work?");
    await testDocumentAgentQuery(basicAgent, "What are the key features of this blockchain platform?");
    
    // Test 4: Multiple agents
    await testMultipleDocumentAgents();
    
    // Test 5: Error handling
    await testDocumentAgentErrorHandling();
    
    // Test 6: Performance benchmarking
    await testDocumentAgentPerformance();
    
    console.log("\n✅ All DocumentAgent tests completed successfully!");
    
  } catch (error) {
    console.error("\n❌ DocumentAgent test suite failed:", error);
    throw error;
  }
}

/**
 * Example usage patterns for DocumentAgent
 */
export const DocumentAgentExamples = {
  
  // Basic usage
  async basicUsage() {
    configureLlamaSettings();
    
    const index = await documentIndexManager.getIndex("documents/resilientdb.pdf");
    const agent = new DocumentAgent({
      documentPath: "documents/resilientdb.pdf",
      index: index!,
      displayName: "ResilientDB Documentation"
    });
    
    const response = await agent.getTool().call({ 
      query: "What is ResilientDB?" 
    });
    
    return response;
  },
  
  // Advanced configuration
  async advancedUsage() {
    const agent = await createDocumentAgent(
      "documents/resilientdb.pdf",
      await documentIndexManager.getIndex("documents/resilientdb.pdf")!,
      {
        displayName: "ResilientDB Technical Documentation",
        description: "Comprehensive guide to ResilientDB blockchain platform",
        useReranking: true,
        rerankingConfig: {
          topK: 10,
          minScore: 0.15,
          verbose: false
        }
      }
    );
    
    return agent;
  },
  
  // Batch processing
  async batchQueries(agent: DocumentAgent, queries: string[]) {
    const results = [];
    
    for (const query of queries) {
      try {
        const response = await agent.getTool().call({ query });
        results.push({ query, response, success: true });
      } catch (error) {
        results.push({ query, error: error.message, success: false });
      }
    }
    
    return results;
  }
};

// Run tests if this file is executed directly
if (require.main === module) {
  runDocumentAgentTests().catch(console.error);
}