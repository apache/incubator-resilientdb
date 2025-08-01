#!/usr/bin/env tsx

import { configureLlamaSettings } from "@/lib/config/llama-settings";
import chalk from "chalk";
import { simpleDocumentService } from "../src/lib/simple-document-service";

/**
 * Test script to verify document caching functionality
 * Tests the optimization that prevents re-parsing documents
 */

async function testDocumentCaching() {
  console.log(chalk.blue("🧪 Testing Document Caching Optimization"));
  console.log(chalk.gray("This test verifies that documents are cached and not re-parsed"));

  try {
    // Configure LlamaIndex settings first
    console.log(chalk.yellow("0. Configuring LlamaIndex settings..."));
    configureLlamaSettings();
    console.log(chalk.green("✓ LlamaIndex settings configured"));
    // Test documents (adjust paths as needed)
    const testDocuments = [
      "documents/resilientdb.pdf"
    ];

    console.log(chalk.yellow("\n1. Getting initial cache stats..."));
    const initialStats = await simpleDocumentService.getCacheStats();
    console.log(chalk.gray(`Initial cache: ${initialStats.documentCount} documents, ${initialStats.totalChunks} chunks`));

    console.log(chalk.yellow("\n2. First indexing run (should parse documents)..."));
    const startTime1 = Date.now();
    const index1 = await simpleDocumentService.indexDocuments(testDocuments);
    const duration1 = Date.now() - startTime1;
    console.log(chalk.green(`✓ First run completed in ${duration1}ms`));

    console.log(chalk.yellow("\n3. Getting cache stats after first run..."));
    const afterFirstStats = await simpleDocumentService.getCacheStats();
    console.log(chalk.gray(`After first run: ${afterFirstStats.documentCount} documents, ${afterFirstStats.totalChunks} chunks`));

    console.log(chalk.yellow("\n4. Second indexing run (should use cache)..."));
    const startTime2 = Date.now();
    const index2 = await simpleDocumentService.indexDocuments(testDocuments);
    const duration2 = Date.now() - startTime2;
    console.log(chalk.green(`✓ Second run completed in ${duration2}ms`));

    // Calculate performance improvement
    const improvement = duration1 > 0 ? ((duration1 - duration2) / duration1 * 100).toFixed(1) : '0.0';
    console.log(chalk.blue(`\n📊 Performance Improvement: ${improvement}% faster (${duration1}ms → ${duration2}ms)`));

    console.log(chalk.yellow("\n5. Testing query functionality..."));
    const queryResult = await simpleDocumentService.queryDocuments(
      "What is ResilientDB?",
      { topK: 3, documentPaths: testDocuments }
    );
    console.log(chalk.green(`✓ Query returned ${queryResult.chunks.length} chunks`));

    console.log(chalk.yellow("\n6. Testing cache management..."));
    console.log(chalk.gray("Testing document removal from cache..."));
    await simpleDocumentService.removeCachedDocument(testDocuments[0]);
    
    const afterRemovalStats = await simpleDocumentService.getCacheStats();
    console.log(chalk.gray(`After removal: ${afterRemovalStats.documentCount} documents, ${afterRemovalStats.totalChunks} chunks`));

    console.log(chalk.yellow("\n7. Third indexing run (should parse again after cache removal)..."));
    const startTime3 = Date.now();
    const index3 = await simpleDocumentService.indexDocuments(testDocuments);
    const duration3 = Date.now() - startTime3;
    console.log(chalk.green(`✓ Third run completed in ${duration3}ms`));

    console.log(chalk.green("\n🎉 Document caching test completed successfully!"));
    console.log(chalk.blue("\nTest Results Summary:"));
    console.log(chalk.gray(`- First run (no cache): ${duration1}ms`));
    console.log(chalk.gray(`- Second run (with cache): ${duration2}ms`));
    console.log(chalk.gray(`- Third run (after cache removal): ${duration3}ms`));
    console.log(chalk.gray(`- Performance improvement: ${improvement}%`));

    const finalStats = await simpleDocumentService.getCacheStats();
    console.log(chalk.gray(`- Final cache state: ${finalStats.documentCount} documents, ${finalStats.totalChunks} chunks`));

  } catch (error) {
    console.error(chalk.red("❌ Test failed:"), error);
    throw error;
  }
}

// Run the test if this script is executed directly
if (require.main === module) {
  testDocumentCaching()
    .then(() => {
      console.log(chalk.green("Test completed successfully"));
      process.exit(0);
    })
    .catch((error) => {
      console.error(chalk.red("Test failed:"), error);
      process.exit(1);
    });
}

export { testDocumentCaching };
