# Agent Tests and Examples

This directory contains comprehensive test suites and usage examples for the agent architecture components.

## Test Files

### 1. `document-agent.test.ts`
Tests and examples for the DocumentAgent class, which handles individual document querying.

**Key Test Areas:**
- Basic agent creation and configuration
- Reranking functionality
- Query execution and response handling
- Error handling and edge cases
- Performance benchmarking
- Multiple agent management

**Usage Examples:**
```typescript
import { DocumentAgentExamples } from './document-agent.test';

// Basic usage
const response = await DocumentAgentExamples.basicUsage();

// Advanced configuration with reranking
const agent = await DocumentAgentExamples.advancedUsage();

// Batch query processing
const results = await DocumentAgentExamples.batchQueries(agent, queries);
```

### 2. `orchestrator-agent.test.ts`
Tests and examples for the OrchestratorAgent class, which manages multi-document querying and intelligent routing.

**Key Test Areas:**
- Agent creation and document management
- Single and multi-document queries
- Query planning functionality
- Concurrent query handling
- Performance and scalability testing
- Error handling and recovery

**Usage Examples:**
```typescript
import { OrchestratorAgentExamples } from './orchestrator-agent.test';

// Multi-document setup
const orchestrator = await OrchestratorAgentExamples.basicMultiDocumentSetup();

// Research workflow
const results = await OrchestratorAgentExamples.researchWorkflow();

// Comparative analysis
const analysis = await OrchestratorAgentExamples.comparativeAnalysis("consensus");
```

### 3. `run-agent-tests.ts`
Main test runner that executes all test suites and provides integration testing.

**Features:**
- Prerequisites verification
- Complete test suite execution
- Integration testing between agents
- Performance benchmarking
- Comprehensive reporting

## Prerequisites

Before running the tests, ensure you have:

1. **Environment Variables Set:**
   ```bash
   DEEPSEEK_API_KEY=your_deepseek_api_key
   LLAMA_CLOUD_API_KEY=your_llamacloud_api_key
   ```

2. **Document Indices Prepared:**
   ```bash
   # Prepare document indices via API
   curl -X POST http://localhost:3000/api/research/prepare-index \
     -H "Content-Type: application/json" \
     -d '{"documentPaths": ["documents/resilientdb.pdf", "documents/rcc.pdf"]}'
   ```

3. **Dependencies Installed:**
   ```bash
   npm install
   ```

## Running Tests

### Option 1: Add to Package.json (Recommended)

Add these scripts to your `package.json`:

```json
{
  "scripts": {
    "test:agents": "tsx tests/run-agent-tests.ts",
    "test:document-agent": "tsx tests/document-agent.test.ts",
    "test:orchestrator-agent": "tsx tests/orchestrator-agent.test.ts"
  }
}
```

Then run:
```bash
npm run test:agents              # Full test suite
npm run test:document-agent      # DocumentAgent only
npm run test:orchestrator        # OrchestratorAgent only (NEW!)
```

### Option 2: Direct Execution

```bash
# Full test suite
npx tsx tests/run-agent-tests.ts

# Individual test files
npx tsx tests/document-agent.test.ts
npx tsx tests/orchestrator-agent.test.ts  # NEW! Comprehensive orchestrator tests
```

### Option 3: Node.js Direct

```bash
# Compile and run
npx tsc tests/run-agent-tests.ts --outDir dist
node dist/tests/run-agent-tests.js
```

## Test Output

The tests provide detailed output including:

- ✅ **Success indicators** for passed tests
- ❌ **Error indicators** for failed tests
- ⏱️ **Performance timing** for all operations
- 📊 **Statistics** on query performance
- 📋 **Configuration details** for agents
- 🔧 **Tool usage** breakdowns

### Sample Output
```
🧪 Starting DocumentAgent Test Suite
=====================================

=== Testing DocumentAgent Creation ===
✅ Agent created successfully
📋 Metadata: {
  name: "query_documents_resilientdb_pdf_tool",
  displayName: "ResilientDB Documentation",
  documentPath: "documents/resilientdb.pdf"
}

=== Testing Query: "What is ResilientDB?" ===
✅ Query executed successfully
⏱️ Duration: 1247ms
📄 Response preview: ResilientDB is a high-performance blockchain platform...
```

## New OrchestratorAgent Tests (Updated!)

The orchestrator test suite has been completely rewritten to focus on **tool routing validation** and the specific query **"Who are the authors of all three papers?"**

### Key Test Cases:

1. **Basic Initialization** - Verifies proper setup with document agents
2. **Single Document Query** - Tests routing to individual documents  
3. **Multi-Document Author Query** ⭐ - **Main test case using your specific prompt**
4. **Query Routing Validation** - Tests intelligent routing to appropriate documents
5. **Error Handling** - Edge cases and error scenarios

### Special Features:

- **Mock Fallback**: Automatically uses mock agents when real documents aren't available
- **Tool Call Validation**: Specifically validates ReActAgent properly calls document tools
- **Multi-Document Coordination**: Tests orchestrator's ability to query multiple documents simultaneously
- **Routing Diagnostics**: Detailed logging to debug ReActAgent routing issues
- **Performance Monitoring**: Tracks test execution times with 60-second timeout per test

### Expected Output for "Who are the authors of all three papers?":

```
🧪 Running test: Multi-Document Author Query
🔍 Querying: "Who are the authors of all three papers?"
[DocumentAgent] 🎯 TOOL CALLED: ResilientDB Paper
[DocumentAgent] 📄 File: resilientdb.pdf
[DocumentAgent] ❓ Query: "authors"
[DocumentAgent] ✅ TOOL COMPLETED: ResilientDB Paper (1200ms)
[DocumentAgent] 🎯 TOOL CALLED: Blockchain Transaction Paper
[DocumentAgent] 📄 File: bchain-transaction-pro.pdf
[DocumentAgent] ❓ Query: "authors"
[DocumentAgent] ✅ TOOL COMPLETED: Blockchain Transaction Paper (980ms)
[DocumentAgent] 🎯 TOOL CALLED: RCC Paper
[DocumentAgent] 📄 File: rcc.pdf
[DocumentAgent] ❓ Query: "authors"
[DocumentAgent] ✅ TOOL COMPLETED: RCC Paper (750ms)
✅ Multi-document author query completed
   Response length: 487 characters
   Sources used: 3 (resilientdb.pdf, bchain-transaction-pro.pdf, rcc.pdf)
   Tools called: 3 (search_resilientdb_paper, search_blockchain_transaction_paper, search_rcc_paper)
   Tool 1: search_resilientdb_paper -> Authors: Suyash Gupta, Jelle Hellings, Mohammad Sadoghi...
   Tool 2: search_blockchain_transaction_paper -> Authors: Mohammad Sadoghi, Spyros Blanas...
   Tool 3: search_rcc_paper -> Authors: H.T. Kung, John T. Robinson...
✅ Test passed: Multi-Document Author Query (3100ms)
```

### Running the Orchestrator Tests:

```bash
# Run integration tests with REAL document agents (RECOMMENDED)
npx tsx tests/orchestrator-integration.test.ts

# Or use the npm script
npm run test:orchestrator-integration

# Run mock-based tests (for development)  
npm run test:orchestrator
```

## 🚀 NEW: Real Integration Tests

**File: `tests/orchestrator-integration.test.ts`**

This is a **real integration test** that uses actual DocumentAgents and document indices - no mocks!

### **Prerequisites:**
1. **Prepare document indices first:**
   ```bash
   # Start your Next.js server
   npm run dev
   
   # Prepare indices via API
   curl -X POST http://localhost:3000/api/research/prepare-index \
     -H "Content-Type: application/json" \
     -d '{"documentPaths": ["documents/resilientdb.pdf", "documents/bchain-transaction-pro.pdf", "documents/rcc.pdf"]}'
   ```

2. **Set environment variables:**
   ```bash
   DEEPSEEK_API_KEY=your_deepseek_api_key
   ```

### **Test Cases:**
1. **"Who are the authors of these three papers?"** - Tests multi-document routing
2. **"Who is the author of resilientdb?"** - Tests single document targeting

### **Expected Output:**
```
🚀 Starting OrchestratorAgent Integration Tests
============================================================
These tests use REAL document indices and API calls
============================================================

🔧 Setting up OrchestratorAgent integration test environment...
📚 Loading real document indices...
✅ Real DocumentAgent loaded: ResilientDB Paper
✅ Real DocumentAgent loaded: Blockchain Transaction Paper  
✅ Real DocumentAgent loaded: RCC Paper
✅ OrchestratorAgent setup complete with 3 document agents

🧪 Running integration test: Multi-Document Author Query
🔍 Real query: "Who are the authors of these three papers?"
📝 Response preview: Based on my search of the documents, the authors are...
✅ Multiple sources consulted: 3
✅ Multiple tools called: 3
✅ Multi-document author query completed
   Tools called: 3 (search_resilientdb_paper, search_blockchain_transaction_paper, search_rcc_paper)

🧪 Running integration test: Single Document Author Query  
🔍 Real query: "Who is the author of resilientdb?"
📝 Response preview: The authors of ResilientDB are Suyash Gupta...
✅ ResilientDB document was consulted
✅ Single document author query completed

📊 Integration Test Results Summary
============================================================
✅ Passed: 2
❌ Failed: 0
⏱️ Total time: 15000ms (15.0s)
🎉 All integration tests passed!
✅ OrchestratorAgent routing is working correctly
```

## Test Categories

### Unit Tests
- Individual component functionality
- Configuration validation
- Error handling
- Edge case handling

### Integration Tests
- Agent interaction patterns
- Multi-document coordination
- Tool usage and routing
- End-to-end query flows

### Performance Tests
- Query execution timing
- Concurrent request handling
- Memory usage patterns
- Scalability benchmarks

### Example Tests
- Real-world usage patterns
- Best practice demonstrations
- Common workflow examples
- Advanced configuration scenarios

## Troubleshooting

### Common Issues

1. **"Document index not found"**
   - Ensure document indices are prepared via the prepare-index API
   - Check that document files exist in the `documents/` directory

2. **"DeepSeek API key is required"**
   - Set the `DEEPSEEK_API_KEY` environment variable
   - Verify the API key is valid and has sufficient credits

3. **"Settings not configured"**
   - Ensure `configureLlamaSettings()` is called before agent creation
   - Check that all required environment variables are set

4. **Timeout errors**
   - Increase timeout settings in the agent configuration
   - Check network connectivity to external APIs

### Debug Mode

Enable verbose logging by setting the agent configuration:

```typescript
const orchestrator = new OrchestratorAgent({
  verbose: true  // Enable detailed logging
});
```

## Extending Tests

To add new tests:

1. **Add test functions** to the appropriate test file
2. **Update the main test runner** to include new tests
3. **Add usage examples** to demonstrate new functionality
4. **Update documentation** to reflect new test coverage

### Example New Test Function

```typescript
async function testNewFeature() {
  console.log("\n=== Testing New Feature ===");
  
  try {
    // Test implementation
    const result = await someNewFunction();
    console.log("✅ New feature test passed");
    return result;
  } catch (error) {
    console.error("❌ New feature test failed:", error);
    throw error;
  }
}
```

## Performance Benchmarks

The tests include performance benchmarking that measures:

- **Query Response Time**: Average time to process queries
- **Throughput**: Queries processed per minute
- **Concurrent Performance**: Handling multiple simultaneous queries
- **Memory Usage**: Resource consumption patterns
- **Tool Efficiency**: Overhead of agent coordination

Results help optimize:
- Reranking configuration (speed vs quality trade-offs)
- Context window utilization
- Tool selection strategies
- Caching opportunities

## Contributing

When adding new tests:

1. Follow the existing naming conventions
2. Include both positive and negative test cases
3. Add performance measurements where relevant
4. Provide clear documentation and examples
5. Update this README with new test information

The test suite serves as both validation and documentation, helping developers understand how to effectively use the agent architecture.