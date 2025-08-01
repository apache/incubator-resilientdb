import chalk from "chalk";
import { configureLlamaSettings, getCurrentEmbedModel } from "../src/lib/config/llama-settings";

/**
 * Test embedding functionality with HuggingFace model
 * This test validates that the embedding model can process text documents
 */
export const testHuggingFaceEmbedding = async (): Promise<void> => {
  console.log(chalk.bold.blue("[EmbeddingTest] Starting HuggingFace embedding test..."));

  try {
    // Ensure LlamaIndex settings are configured
    configureLlamaSettings();

    // Get the configured embedding model
    const embedModel = getCurrentEmbedModel();
    
    if (!embedModel) {
      throw new Error("Embedding model not configured");
    }

    console.log(chalk.yellow("[EmbeddingTest] Using model: BAAI/bge-large-en-v1.5"));

    // Fake text document for testing
    const testDocument = `
    ResilientDB is a high-throughput yielding permissioned blockchain fabric. 
    ResilientDB is built to scale BFT consensus and can process 100k+ transactions per second. 
    The system uses innovative techniques like multi-primary consensus and parallel execution 
    to achieve these performance levels while maintaining Byzantine fault tolerance.
    
    Key features include:
    - Multi-primary BFT consensus protocol
    - Parallel transaction execution
    - High throughput processing
    - Fault tolerance mechanisms
    - Scalable architecture design
    `;

    console.log(chalk.cyan("[EmbeddingTest] Text to embed:"));
    console.log(chalk.gray(testDocument.trim()));
    console.log();

    // Start timing the embedding process
    const startTime = Date.now();
    
    // Generate embedding for the test document
    console.log(chalk.yellow("[EmbeddingTest] Generating embedding..."));
    const embedding = await embedModel.getTextEmbedding(testDocument.trim());
    
    const endTime = Date.now();
    const duration = endTime - startTime;

    // Validate the embedding result
    if (!embedding || !Array.isArray(embedding) || embedding.length === 0) {
      throw new Error("Invalid embedding result");
    }

    // Log embedding statistics
    console.log(chalk.green("[EmbeddingTest] ✅ Embedding generated successfully!"));
    console.log(chalk.cyan(`[EmbeddingTest] Embedding dimensions: ${embedding.length}`));
    console.log(chalk.cyan(`[EmbeddingTest] Processing time: ${duration}ms`));
    console.log(chalk.cyan(`[EmbeddingTest] First 5 values: [${embedding.slice(0, 5).map(v => v.toFixed(4)).join(", ")}...]`));
    console.log(chalk.cyan(`[EmbeddingTest] Embedding magnitude: ${Math.sqrt(embedding.reduce((sum, val) => sum + val * val, 0)).toFixed(4)}`));

    // Test with multiple documents
    console.log(chalk.yellow("[EmbeddingTest] Testing batch embedding..."));
    
    const batchDocuments = [
      "Blockchain technology enables decentralized consensus.",
      "Byzantine fault tolerance ensures system reliability.",
      "High throughput processing requires parallel execution."
    ];

    const batchStartTime = Date.now();
    const batchEmbeddings = await Promise.all(
      batchDocuments.map(doc => embedModel.getTextEmbedding(doc))
    );
    const batchEndTime = Date.now();

    console.log(chalk.green("[EmbeddingTest] ✅ Batch embedding completed!"));
    console.log(chalk.cyan(`[EmbeddingTest] Batch processing time: ${batchEndTime - batchStartTime}ms`));
    console.log(chalk.cyan(`[EmbeddingTest] Average per document: ${((batchEndTime - batchStartTime) / batchDocuments.length).toFixed(2)}ms`));

    // Calculate similarity between embeddings
    const similarity = cosineSimilarity(batchEmbeddings[0], batchEmbeddings[1]);
    console.log(chalk.cyan(`[EmbeddingTest] Similarity between first two docs: ${similarity.toFixed(4)}`));

    console.log(chalk.bold.green("[EmbeddingTest] 🎉 All tests passed successfully!"));

  } catch (error) {
    console.error(chalk.red("[EmbeddingTest] ❌ Test failed:"), error);
    throw error;
  }
};

/**
 * Calculate cosine similarity between two embedding vectors
 */
const cosineSimilarity = (a: number[], b: number[]): number => {
  if (a.length !== b.length) {
    throw new Error("Vectors must have the same length");
  }

  let dotProduct = 0;
  let normA = 0;
  let normB = 0;

  for (let i = 0; i < a.length; i++) {
    dotProduct += a[i] * b[i];
    normA += a[i] * a[i];
    normB += b[i] * b[i];
  }

  return dotProduct / (Math.sqrt(normA) * Math.sqrt(normB));
};

/**
 * Run the embedding test if this file is executed directly
 */
if (require.main === module) {
  testHuggingFaceEmbedding()
    .then(() => {
      console.log(chalk.bold.green("Test completed successfully!"));
      process.exit(0);
    })
    .catch((error) => {
      console.error(chalk.red("Test failed:"), error);
      process.exit(1);
    });
}
