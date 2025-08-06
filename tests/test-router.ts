import { agentStreamEvent, agentToolCallEvent, agentToolCallResultEvent } from "@llamaindex/workflow";
import chalk from "chalk";
import { ChatMessage } from "llamaindex";
import { configureLlamaSettings } from "../src/lib/config/llama-settings";
import { llamaService } from "../src/lib/llama-service";

// const testChatEngine = async (testQuery: string, documentPaths: string[]) => {
//   console.log("\n🧪 Testing Chat Engine...");
//   const chatEngine = await llamaService.createChatEngine(documentPaths);
//   const chatResponse = await chatEngine.chat({
//     message: testQuery,
//     stream: true,
//   });

//   console.log("📺 Chat response:");
//   console.log("─".repeat(50));
//   let lastChunk;
  
//   for await (const chunk of chatResponse) {
//     lastChunk = chunk; // Store the current chunk as the last one
    
//     const content = chunk.response || chunk.delta || "";
//     if (content) {
//       process.stdout.write(content);
//     }
//   }
  
//   // After the loop, process the last chunk's source nodes
//   if (lastChunk?.sourceNodes) {
//     process.stdout.write("\n📚 Source Node Metadata:\n");
//     process.stdout.write("─".repeat(30) + "\n");
    
//     for (const sourceNode of lastChunk.sourceNodes) {
//       if (sourceNode.node.metadata) {
//         process.stdout.write(JSON.stringify(sourceNode.node.metadata, null, 2) + "\n");
//       }
//     }
//   }
// };

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
    // await testChatEngine(testQuery, documentPaths);
    // await testQueryEngine(testQuery, documentPaths, options);
  } catch (error) {
    console.error("❌ Test failed:", error);
    if (error instanceof Error) {
      console.error("Error message:", error.message);
      console.error("Stack trace:", error.stack);
    }
  }
}

async function testIngestion() {
  console.log("🧪 Testing ingestion...");
  configureLlamaSettings();
  const documentPaths = ["documents/rcc.pdf"];
  await llamaService.ingestDocs(documentPaths);
}


async function testAgent() {
  const ctx = [{
    role: "user",
    content: "My name is Abdel."
  }] as ChatMessage[]
  const testQuery = "Can you explain the replica failure algorithm? After that, search the web for new ResilientDB developments"
  const documents = ["documents/rcc.pdf"]
  const agent = await llamaService.createNexusAgent(documents);
  const response = await agent.runStream(testQuery, {
    // chatHistory: ctx,
  });
  for await (const event of response) {
    if (agentToolCallEvent.include(event)) {
      console.log(chalk.yellow(`\nTool being called: ${JSON.stringify(event.data, null, 2)}`));
    }
    if (agentToolCallResultEvent.include(event)) {
      console.log(chalk.green(`\nTool result received: ${JSON.stringify(event.data, null, 2)}`));
    }
    if (agentStreamEvent.include(event)) {
      process.stdout.write(event.data.delta);
    }

  }
}


// Main execution
async function main() {
  // console.log("🚀 Starting Router Behavior Test");
  console.log("=".repeat(50));
  
  // Single detailed test
  // await testRouterBehavior();
  // await testIngestion();
  await testAgent();
  // await testExample();
  
  console.log("\n\n🔄 Running multiple test queries...");
//   await runMultipleTests();

  // await testSourceNodes();
  
  console.log("\nAll tests completed!");
}

// Run the test
main().catch(error => {
  console.error("💥 Fatal error:", error);
  process.exit(1);
});