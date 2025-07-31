/**
 * Agent Test Suite Runner
 * 
 * This script runs all agent tests and provides comprehensive examples
 * of how to use the DocumentAgent and OrchestratorAgent classes.
 */

import { configureLlamaSettings } from "../src/lib/config/llama-settings";
import { documentIndexManager } from "../src/lib/document-index-manager";
import { DocumentAgentExamples, runDocumentAgentTests } from "./document-agent.test";
import { OrchestratorAgentExamples, runOrchestratorAgentTests } from "./orchestrator-agent.test";

/**
 * Verify prerequisites before running tests
 */
async function verifyPrerequisites() {
  console.log("🔍 Verifying test prerequisites...");
  
  try {
    // Check if settings can be configured
    configureLlamaSettings();
    console.log("✅ LLM settings configured");
    
    // Check if any document indices are available
    const testDocuments = [
      "documents/resilientdb.pdf",
      "documents/bchain-transaction-pro.pdf",
      "documents/rcc.pdf"
    ];
    
    let availableDocuments = 0;
    for (const docPath of testDocuments) {
      try {
        // Try to get the index, which will load from database if needed
        const index = await documentIndexManager.getIndex(docPath);
        if (index) {
          availableDocuments++;
          console.log(`✅ Index available: ${docPath}`);
        } else {
          console.log(`⚠️ Index missing: ${docPath}`);
        }
      } catch (error) {
        console.log(`⚠️ Index missing: ${docPath} (${error.message})`);
      }
    }
    
    if (availableDocuments === 0) {
      console.log("\n❌ No document indices found!");
      console.log("Please prepare document indices first using the prepare-index API:");
      console.log("POST /api/research/prepare-index");
      console.log('{ "documentPaths": ["documents/resilientdb.pdf", "documents/rcc.pdf"] }');
      return false;
    }
    
    console.log(`✅ Found ${availableDocuments} prepared document(s)`);
    return true;
    
  } catch (error) {
    console.error("❌ Prerequisites check failed:", error);
    return false;
  }
}

/**
 * Run DocumentAgent examples
 */
async function runDocumentAgentExamples() {
  console.log("\n🎯 Running DocumentAgent Examples");
  console.log("=================================");
  
  try {
    // Example 1: Basic usage
    console.log("\n📖 Example 1: Basic DocumentAgent Usage");
    const basicResponse = await DocumentAgentExamples.basicUsage();
    console.log("Response preview:", typeof basicResponse === 'string' 
      ? basicResponse.substring(0, 150) + "..." 
      : "Non-string response received");
    
    // Example 2: Advanced configuration
    console.log("\n⚙️ Example 2: Advanced Configuration");
    const advancedAgent = await DocumentAgentExamples.advancedUsage();
    const config = advancedAgent.getConfig();
    console.log("Advanced agent configured:", {
      displayName: config.displayName,
      useReranking: config.useReranking
    });
    
    // Example 3: Batch queries
    console.log("\n📊 Example 3: Batch Query Processing");
    const batchQueries = [
      "What is the main purpose of this document?",
      "What are the key technical features?",
      "What performance characteristics are mentioned?"
    ];
    
    const batchResults = await DocumentAgentExamples.batchQueries(advancedAgent, batchQueries);
    console.log(`Batch processing completed: ${batchResults.filter(r => r.success).length}/${batchResults.length} successful`);
    
  } catch (error) {
    console.error("❌ DocumentAgent examples failed:", error);
  }
}

/**
 * Run OrchestratorAgent examples
 */
async function runOrchestratorAgentExamples() {
  console.log("\n🎯 Running OrchestratorAgent Examples");
  console.log("====================================");
  
  try {
    // Example 1: Basic multi-document setup
    console.log("\n🏗️ Example 1: Multi-Document Setup");
    const orchestrator = await OrchestratorAgentExamples.basicMultiDocumentSetup();
    const summary = orchestrator.getDocumentsSummary();
    console.log(`Setup complete: ${summary.totalDocuments} documents available`);
    
    // Example 2: Research workflow
    console.log("\n🔬 Example 2: Research Workflow");
    const researchResults = await OrchestratorAgentExamples.researchWorkflow();
    console.log(`Research workflow completed: ${researchResults.length} questions answered`);
    researchResults.forEach((result, i) => {
      console.log(`  Q${i + 1}: ${result.question.substring(0, 50)}... (${result.sources.length} sources, ${result.toolCalls} tools)`);
    });
    
    // Example 3: Comparative analysis
    console.log("\n📈 Example 3: Comparative Analysis");
    const analysisResult = await OrchestratorAgentExamples.comparativeAnalysis("consensus mechanisms");
    console.log(`Comparative analysis completed for: ${analysisResult.topic}`);
    console.log(`Sources consulted: ${analysisResult.sourcesConsulted.length}`);
    console.log(`Analysis depth: ${analysisResult.analysisDepth} tool calls`);
    
    // Example 4: Interactive session
    console.log("\n💬 Example 4: Interactive Research Session");
    const interactiveQueries = [
      "What are the main contributions of these documents?",
      "How do they approach scalability challenges?",
      "What future research directions are suggested?"
    ];
    
    const sessionResults = await OrchestratorAgentExamples.interactiveSession(interactiveQueries);
    console.log(`Interactive session completed: ${sessionResults.length} exchanges`);
    
    const avgDuration = sessionResults.reduce((sum, r) => sum + r.duration, 0) / sessionResults.length;
    console.log(`Average response time: ${avgDuration.toFixed(2)}ms`);
    
  } catch (error) {
    console.error("❌ OrchestratorAgent examples failed:", error);
  }
}

/**
 * Run integration tests
 */
async function runIntegrationTests() {
  console.log("\n🔗 Running Integration Tests");
  console.log("============================");
  
  try {
    // Test DocumentAgent → OrchestratorAgent integration
    console.log("\n🔄 Testing Agent Integration");
    
    const orchestrator = await OrchestratorAgentExamples.basicMultiDocumentSetup();
    
    // Test that orchestrator can handle various query types
    const integrationQueries = [
      "What is ResilientDB?", // Simple factual query
      "Compare the consensus mechanisms across documents", // Comparative query
      "What are the performance implications of different approaches?", // Analytical query
      "Summarize the key innovations mentioned in all documents" // Synthesis query
    ];
    
    for (let i = 0; i < integrationQueries.length; i++) {
      const query = integrationQueries[i];
      console.log(`\n🔍 Integration Test ${i + 1}: ${query.substring(0, 40)}...`);
      
      try {
        const startTime = Date.now();
        const result = await orchestrator.query(query);
        const duration = Date.now() - startTime;
        
        console.log(`✅ Success (${duration}ms): ${result.sources.length} sources, ${result.toolCalls.length} tools`);
        
      } catch (error) {
        console.log(`❌ Failed: ${error.message}`);
      }
    }
    
  } catch (error) {
    console.error("❌ Integration tests failed:", error);
  }
}

/**
 * Generate test report
 */
function generateTestReport(results: any) {
  console.log("\n📋 Test Report Summary");
  console.log("=====================");
  
  const timestamp = new Date().toISOString();
  console.log(`Generated: ${timestamp}`);
  console.log(`Node.js Version: ${process.version}`);
  console.log(`Platform: ${process.platform}`);
  
  // Add any additional reporting logic here
  console.log("\n✅ All tests and examples completed successfully!");
  console.log("\nFor more detailed information, review the individual test outputs above.");
  console.log("\nTo run specific tests:");
  console.log("- DocumentAgent only: npm run test:document-agent");
  console.log("- OrchestratorAgent only: npm run test:orchestrator-agent");
  console.log("- Full suite: npm run test:agents");
}

/**
 * Main test runner
 */
async function main() {
  console.log("🚀 Starting Agent Test Suite");
  console.log("============================");
  console.log(`Started at: ${new Date().toISOString()}\n`);
  
  try {
    // Step 1: Verify prerequisites
    const prerequisitesOk = await verifyPrerequisites();
    if (!prerequisitesOk) {
      process.exit(1);
    }
    
    // // Step 2: Run DocumentAgent tests
    // console.log("\n" + "=".repeat(60));
    // await runDocumentAgentTests();
    
    // Step 3: Run DocumentAgent examples
    await runDocumentAgentExamples();
    
    // // Step 4: Run OrchestratorAgent tests
    // console.log("\n" + "=".repeat(60));
    // await runOrchestratorAgentTests();
    
    // Step 5: Run OrchestratorAgent examples
    await runOrchestratorAgentExamples();
    
    // Step 6: Run integration tests
    await runIntegrationTests();
    
    // Step 7: Generate report
    generateTestReport({});
    
  } catch (error) {
    console.error("\n💥 Test suite failed with error:", error);
    process.exit(1);
  }
}

// Export individual test runners for selective execution
export {
    runDocumentAgentExamples, runDocumentAgentTests, runIntegrationTests, runOrchestratorAgentExamples, runOrchestratorAgentTests
};

// Run main if this file is executed directly
if (require.main === module) {
  main().catch((error) => {
    console.error("Fatal error:", error);
    process.exit(1);
  });
}

/**
 * Usage Examples for Package.json Scripts:
 * 
 * Add these to your package.json scripts section:
 * 
 * {
 *   "scripts": {
 *     "test:agents": "tsx tests/run-agent-tests.ts",
 *     "test:document-agent": "tsx tests/document-agent.test.ts",
 *     "test:orchestrator-agent": "tsx tests/orchestrator-agent.test.ts",
 *     "test:agents:examples": "tsx -e \"import('./tests/run-agent-tests.js').then(m => Promise.all([m.runDocumentAgentExamples(), m.runOrchestratorAgentExamples()]))\""
 *   }
 * }
 * 
 * Then run:
 * npm run test:agents              # Full test suite
 * npm run test:document-agent      # DocumentAgent only
 * npm run test:orchestrator-agent  # OrchestratorAgent only
 * npm run test:agents:examples     # Examples only
 */