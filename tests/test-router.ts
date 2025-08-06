import { configureLlamaSettings } from "../src/lib/config/llama-settings";
import { llamaService } from "../src/lib/llama-service";

const testChatEngine = async (testQuery: string, documentPaths: string[]) => {
  console.log("\n🧪 Testing Chat Engine...");
  const chatEngine = await llamaService.createChatEngine(documentPaths, []);
  const chatResponse = await chatEngine.chat({
    message: testQuery,
    stream: true,
  });

  console.log("📺 Chat response:");
  console.log("─".repeat(50));
  let lastChunk;
  
  for await (const chunk of chatResponse) {
    lastChunk = chunk; // Store the current chunk as the last one
    
    const content = chunk.response || chunk.delta || "";
    if (content) {
      process.stdout.write(content);
    }
  }
  
  // After the loop, process the last chunk's source nodes
  if (lastChunk?.sourceNodes) {
    process.stdout.write("\n📚 Source Node Metadata:\n");
    process.stdout.write("─".repeat(30) + "\n");
    
    for (const sourceNode of lastChunk.sourceNodes) {
      if (sourceNode.node.metadata) {
        process.stdout.write(JSON.stringify(sourceNode.node.metadata, null, 2) + "\n");
      }
    }
  }
};

// const testQueryEngine = async (testQuery: string, documentPaths: string[], options: { topK: number; tool: string }) => {
//   console.log("\n🔧 Creating streaming query engine...");
//   const queryEngine = await llamaService.createQueryEngine(documentPaths);
//   console.log("✅ Query engine created successfully");
//   const response = await queryEngine.query({
//     query: testQuery,
//     stream: true,
//   });
//   console.log("📺 Streaming response:");
//   console.log("─".repeat(50));
//   let responseText = "";
//   for await (const chunk of response) {
//     const content = chunk.response || chunk.delta || "";
//     if (content) {
//       process.stdout.write(content);
//       responseText += content;
//     }
//   }
//   console.log("\n" + "─".repeat(50));
//   console.log("✅ Streaming completed successfully");
// };

async function testRouterBehavior() {
  console.log("🧪 Testing router behavior with rcc.pdf...");
  try {
    configureLlamaSettings();
    const testQuery = "Explain the replica failure algoirthm in 50 words. \nWhen you use information from the documents, append a citation like [^id], where id is the index of the source node in the source node array.";
    const documentPaths = ["documents/rcc.pdf"];
    const options = {
      topK: 5,
      tool: "default"
    };
    console.log(`📋 Query: "${testQuery}"`);
    console.log(`📄 Documents: ${documentPaths.join(", ")}`);
    console.log(`⚙️  Options:`, options);
    await testChatEngine(testQuery, documentPaths);
    // await testQueryEngine(testQuery, documentPaths, options);
  } catch (error) {
    console.error("❌ Test failed:", error);
    if (error instanceof Error) {
      console.error("Error message:", error.message);
      console.error("Stack trace:", error.stack);
    }
  }
}


// Main execution
async function main() {
  console.log("🚀 Starting Router Behavior Test");
  console.log("=".repeat(80));
  
  // Single detailed test
  await testRouterBehavior();
  
  console.log("\n\n🔄 Running multiple test queries...");
//   await runMultipleTests();

  // await testSourceNodes();
  
  console.log("\n✅ All tests completed!");
}

// Run the test
main().catch(error => {
  console.error("💥 Fatal error:", error);
  process.exit(1);
});