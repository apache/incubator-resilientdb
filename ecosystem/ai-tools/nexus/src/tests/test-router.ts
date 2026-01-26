/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

import { CodeAgent } from "@/lib/agent";
import { agentStreamEvent, agentToolCallEvent } from "@llamaindex/workflow";
import chalk from "chalk";
import { configureLlamaSettings } from "../lib/config/llama-settings";

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

async function testCodeAgent() {
  console.log("🧪 Testing CodeAgent with 'Replica failure' query...");
  try {
    configureLlamaSettings();
    
    // Initialize the CodeAgent
    const codeAgent = new CodeAgent("ts");
    
    // Create agent workflow with the rcc.pdf document
    const agentWorkflow = await codeAgent.createAgent(["documents/rcc.pdf"], "code-agent-test");
    
    const prompt = "Replica failure";
    console.log(chalk.blue(`\n\nPrompt: ${prompt}\n`));
    console.log(chalk.green("Starting CodeAgent workflow...\n"));
    
    const response = await agentWorkflow.runStream(prompt);
    
    for await (const event of response) {
      if (agentToolCallEvent.include(event)) {
        console.log(chalk.yellow(`\nTool being called: ${JSON.stringify(event.data, null, 2)}`));
      }
      if (agentStreamEvent.include(event)) {
        process.stdout.write(event.data.delta);
      }
    }
    
    console.log(chalk.green("\n\nCodeAgent workflow completed!"));
    
  } catch (error) {
    console.error("❌ CodeAgent test failed:", error);
    if (error instanceof Error) {
      console.error("Error message:", error.message);
      console.error("Stack trace:", error.stack);
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
  // await testAgent();
  // await testAgentClass(); 
  await testCodeAgent();
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