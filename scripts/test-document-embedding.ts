import chalk from "chalk";
import { existsSync, mkdirSync, writeFileSync } from "fs";
import { join } from "path";
import { configureLlamaSettings } from "../src/lib/config/llama-settings";
import { documentService } from "../src/lib/document-service";
import { vectorStoreService } from "../src/lib/vector-store";

/**
 * Test script to embed a fake text document using HuggingFace embeddings
 * and the DocumentService.indexDocuments() method
 */
async function testDocumentEmbedding() {
  console.log(chalk.bold.blue("🧪 Testing Document Embedding with HuggingFace"));
  console.log(chalk.gray("=".repeat(60)));

  try {
    // Step 1: Configure LlamaIndex settings (DeepSeek + HuggingFace embeddings)
    console.log(chalk.blue("1. Configuring LlamaIndex settings..."));
    configureLlamaSettings();
    console.log(chalk.green("✅ LlamaIndex configured with HuggingFace embeddings"));

    // Step 2: Create a fake PDF-like text document
    console.log(chalk.blue("\n2. Creating fake text document..."));
    const fakeDocumentContent = createFakeDocument();
    
    // Create a temporary directory for the test document
    const testDir = join(process.cwd(), "temp-test");
    if (!existsSync(testDir)) {
      mkdirSync(testDir, { recursive: true });
    }
    
    const testDocPath = join(testDir, "fake-research-paper.txt");
    writeFileSync(testDocPath, fakeDocumentContent);
    console.log(chalk.green(`✅ Created fake document: ${testDocPath}`));

    // Step 3: Test document indexing
    console.log(chalk.blue("\n3. Testing document indexing..."));
    const relativePath = "temp-test/fake-research-paper.txt";
    
    const startTime = Date.now();
    const index = await documentService.indexDocuments([relativePath]);
    const indexingTime = Date.now() - startTime;
    
    console.log(chalk.green(`✅ Document indexed successfully in ${indexingTime}ms`));

    // Step 4: Test document querying
    console.log(chalk.blue("\n4. Testing document querying..."));
    
    const testQueries = [
      "What is blockchain technology?",
      "How does consensus work?",
      "What are the benefits of distributed systems?",
      "Tell me about security in blockchain networks"
    ];

    for (const query of testQueries) {
      console.log(chalk.yellow(`\nQuery: "${query}"`));
      
      const queryStartTime = Date.now();
      const result = await documentService.queryDocuments(query, {
        topK: 3,
        documentPaths: [relativePath]
      });
      const queryTime = Date.now() - queryStartTime;
      
      console.log(chalk.cyan(`  └─ Found ${result.totalChunks} relevant chunks in ${queryTime}ms`));
      console.log(chalk.gray(`  └─ Sources: ${result.sources.map(s => s.displayTitle).join(", ")}`));
      
      // Show top relevant chunk
      if (result.chunks.length > 0) {
        const topChunk = result.chunks[0];
        const preview = topChunk.content.substring(0, 150).replace(/\n/g, " ");
        console.log(chalk.gray(`  └─ Top result: "${preview}..."`));
        console.log(chalk.gray(`  └─ Relevance score: ${topChunk.metadata?.score?.toFixed(4) || 'N/A'}`));
      }
    }

    // Step 5: Test vector store directly
    console.log(chalk.blue("\n5. Testing vector store statistics..."));
    const vectorStore = await vectorStoreService.getVectorStore();
    console.log(chalk.green("✅ Vector store connection verified"));

    // Step 6: Display cache statistics
    console.log(chalk.blue("\n6. Checking cache statistics..."));
    const cacheStats = await documentService.getCacheStats();
    console.log(chalk.cyan("Cache Statistics:"));
    console.log(chalk.gray(`  Documents: ${cacheStats.documentCount}`));
    console.log(chalk.gray(`  Total Chunks: ${cacheStats.totalChunks}`));
    console.log(chalk.gray(`  Oldest: ${cacheStats.oldestDocument || 'None'}`));
    console.log(chalk.gray(`  Newest: ${cacheStats.newestDocument || 'None'}`));

    // Step 7: Test embedding model directly
    console.log(chalk.blue("\n7. Testing HuggingFace embedding model directly..."));
    const { getCurrentEmbedModel } = await import("../src/lib/config/llama-settings");
    const embedModel = getCurrentEmbedModel();
    
    const testText = "This is a test sentence for embedding.";
    const embedStartTime = Date.now();
    const embedding = await embedModel.getTextEmbedding(testText);
    const embedTime = Date.now() - embedStartTime;
    
    console.log(chalk.green(`✅ Generated embedding in ${embedTime}ms`));
    console.log(chalk.gray(`  └─ Embedding dimensions: ${embedding.length}`));
    console.log(chalk.gray(`  └─ First few values: [${embedding.slice(0, 5).map(v => v.toFixed(4)).join(", ")}...]`));

    // Step 8: Test multiple document filtering
    console.log(chalk.blue("\n8. Testing document filtering..."));
    const allDocsResult = await documentService.queryDocuments("blockchain", { topK: 5 });
    const filteredResult = await documentService.queryDocuments("blockchain", {
      topK: 5,
      documentPaths: [relativePath]
    });
    
    console.log(chalk.cyan(`  └─ All documents query: ${allDocsResult.totalChunks} chunks`));
    console.log(chalk.cyan(`  └─ Filtered query: ${filteredResult.totalChunks} chunks`));

    // Cleanup
    console.log(chalk.blue("\n9. Cleaning up..."));
    try {
      const fs = await import("fs/promises");
      await fs.unlink(testDocPath);
      await fs.rmdir(testDir);
      console.log(chalk.green("✅ Test files cleaned up"));
    } catch (cleanupError) {
      console.log(chalk.yellow("⚠️ Manual cleanup may be needed for temp files"));
    }

    console.log(chalk.bold.green("\n🎉 All tests completed successfully!"));
    console.log(chalk.gray("=".repeat(60)));

  } catch (error) {
    console.error(chalk.red("\n❌ Test failed:"));
    console.error(chalk.red(error instanceof Error ? error.message : String(error)));
    if (error instanceof Error && error.stack) {
      console.error(chalk.gray(error.stack));
    }
    process.exit(1);
  }
}

/**
 * Create a fake research document with multiple sections
 * This simulates a PDF that would be parsed by LlamaParse
 */
function createFakeDocument(): string {
  return `
# Blockchain Technology and Distributed Consensus Systems

## Abstract

This research paper explores the fundamental concepts of blockchain technology and distributed consensus mechanisms. We examine the cryptographic foundations, network protocols, and security considerations that make blockchain networks resilient and trustworthy. Our analysis covers various consensus algorithms including Proof-of-Work, Proof-of-Stake, and emerging hybrid approaches.

## 1. Introduction

Blockchain technology represents a paradigm shift in how distributed systems achieve consensus without requiring a central authority. The technology emerged from the need to solve the double-spending problem in digital currencies, but its applications have expanded far beyond cryptocurrency.

### 1.1 Background

Traditional distributed systems rely on central coordinators to maintain consistency across nodes. Blockchain networks, however, use cryptographic proof and economic incentives to achieve consensus in a trustless environment. This approach eliminates single points of failure and reduces the need for intermediaries.

### 1.2 Key Components

The core components of blockchain systems include:
- Cryptographic hash functions for data integrity
- Digital signatures for authentication
- Merkle trees for efficient verification
- Consensus protocols for agreement
- Peer-to-peer networks for decentralization

## 2. Consensus Mechanisms

Consensus mechanisms are the heart of blockchain networks, enabling distributed nodes to agree on the state of the system without trusting each other.

### 2.1 Proof-of-Work (PoW)

Proof-of-Work requires network participants to solve computationally expensive puzzles to validate transactions and create new blocks. Bitcoin's success demonstrated the viability of PoW, but concerns about energy consumption have led to alternative approaches.

### 2.2 Proof-of-Stake (PoS)

Proof-of-Stake selects validators based on their stake in the network rather than computational power. This approach significantly reduces energy consumption while maintaining security through economic incentives.

### 2.3 Hybrid Approaches

Modern blockchain networks often combine multiple consensus mechanisms to optimize for different requirements such as throughput, finality, and decentralization.

## 3. Security Considerations

Security in blockchain networks depends on several factors:

### 3.1 Cryptographic Security

The underlying cryptographic primitives must resist attacks from quantum computers and other advanced threats. Hash functions like SHA-256 and elliptic curve digital signatures provide current security.

### 3.2 Network Security

The distributed nature of blockchain networks makes them resistant to traditional attacks, but new threat vectors emerge such as 51% attacks and eclipse attacks.

### 3.3 Smart Contract Security

Programmable blockchains introduce additional security considerations through smart contracts, which can contain vulnerabilities leading to significant financial losses.

## 4. Performance and Scalability

Current blockchain networks face the blockchain trilemma: the difficulty of simultaneously achieving decentralization, security, and scalability.

### 4.1 Throughput Limitations

Bitcoin processes approximately 7 transactions per second, while Ethereum handles around 15. These limitations stem from the consensus requirements and block size constraints.

### 4.2 Scaling Solutions

Various approaches aim to improve blockchain scalability:
- Layer 2 solutions like Lightning Network
- Sharding to distribute load across multiple chains
- Off-chain computation with on-chain verification

## 5. Applications Beyond Cryptocurrency

Blockchain technology enables numerous applications:

### 5.1 Supply Chain Management

Immutable records provide transparency and traceability in complex supply chains, helping verify product authenticity and ethical sourcing.

### 5.2 Digital Identity

Self-sovereign identity systems give users control over their personal data while enabling secure authentication across services.

### 5.3 Decentralized Finance (DeFi)

Smart contracts enable complex financial instruments without traditional intermediaries, creating new opportunities for lending, trading, and investment.

## 6. Future Directions

The future of blockchain technology involves addressing current limitations while exploring new use cases:

### 6.1 Interoperability

Cross-chain protocols will enable value and data transfer between different blockchain networks.

### 6.2 Privacy Enhancements

Zero-knowledge proofs and other privacy-preserving technologies will enable confidential transactions while maintaining auditability.

### 6.3 Sustainability

More energy-efficient consensus mechanisms and carbon-neutral blockchain networks will address environmental concerns.

## 7. Conclusion

Blockchain technology has evolved from supporting simple cryptocurrency transactions to enabling complex decentralized applications. While challenges remain in scalability, energy efficiency, and user experience, ongoing research and development continue to expand the possibilities.

The convergence of cryptography, distributed systems, and economic incentives in blockchain networks represents a fundamental innovation in computer science. As the technology matures, we expect to see increased adoption across industries and the development of new paradigms for digital interaction.

## References

1. Nakamoto, S. (2008). Bitcoin: A Peer-to-Peer Electronic Cash System.
2. Buterin, V. (2014). Ethereum: A Next-Generation Smart Contract and Decentralized Application Platform.
3. King, S., & Nadal, S. (2012). PPCoin: Peer-to-Peer Crypto-Currency with Proof-of-Stake.
4. Castro, M., & Liskov, B. (1999). Practical Byzantine Fault Tolerance.
5. Wood, G. (2014). Ethereum: A Secure Decentralised Generalised Transaction Ledger.

---

This document contains approximately 1,000 words covering blockchain technology, consensus mechanisms, security, scalability, and applications. The content is structured to provide comprehensive coverage suitable for embedding and retrieval testing.
`.trim();
}

// Run the test if this script is executed directly
if (require.main === module) {
  testDocumentEmbedding().catch(console.error);
}

export { createFakeDocument, testDocumentEmbedding };

