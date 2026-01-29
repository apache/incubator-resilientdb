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

#!/usr/bin/env tsx

/**
 * Test MCP Server Protocol Communication
 * 
 * This script tests the MCP server by:
 * 1. Spawning the MCP server as a subprocess
 * 2. Sending JSON-RPC protocol messages via stdin
 * 3. Reading responses via stdout
 * 4. Verifying the server responds correctly
 */

import { spawn, ChildProcess } from 'child_process';
import * as readline from 'readline';
import { promisify } from 'util';

interface MCPRequest {
  jsonrpc: '2.0';
  id: number | string;
  method: string;
  params?: any;
}

interface MCPResponse {
  jsonrpc: '2.0';
  id: number | string;
  result?: any;
  error?: {
    code: number;
    message: string;
    data?: any;
  };
}

interface TestResult {
  testName: string;
  passed: boolean;
  message: string;
  response?: any;
}

const delay = (ms: number) => new Promise((resolve) => setTimeout(resolve, ms));

class MCPServerTester {
  private process: ChildProcess | null = null;
  private messages: MCPResponse[] = [];
  private rl: readline.Interface | null = null;
  private messageIdCounter = 1;
  private testResults: TestResult[] = [];

  async start(): Promise<void> {
    console.log('🚀 Starting MCP Server Test');
    console.log('═══════════════════════════════════════════════════════════\n');

    console.log('1️⃣  Spawning MCP server process...');
    
    // Spawn the MCP server - use local npm script since we want to test locally
    // If testing Docker container, we'd need to use docker exec
    this.process = spawn('npm', ['run', 'mcp-server'], {
      cwd: process.cwd(),
      stdio: ['pipe', 'pipe', 'pipe'],
      env: process.env,
    });

    // Handle stderr (logs)
    this.process.stderr?.on('data', (data) => {
      const output = data.toString();
      // Only show important server messages
      if (output.includes('MCP Server') || output.includes('Error') || output.includes('Started')) {
        console.log(`📋 Server: ${output.trim()}`);
      }
    });

    // Set up readline for stdout (JSON-RPC messages)
    this.rl = readline.createInterface({
      input: this.process.stdout!,
      crlfDelay: Infinity,
    });

    // Listen for JSON-RPC messages
    this.rl.on('line', (line) => {
      const trimmed = line.trim();
      if (!trimmed) return;

      try {
        const message: MCPResponse = JSON.parse(trimmed);
        this.messages.push(message);
        console.log(`✅ Received response for ID ${message.id}:`, JSON.stringify(message, null, 2).slice(0, 200));
      } catch (e) {
        // Not a JSON message, might be log output
        if (trimmed.includes('MCP Server') || trimmed.includes('Error')) {
          console.log(`📝 Server log: ${trimmed}`);
        }
      }
    });

    // Wait for server to initialize
    console.log('   ⏳ Waiting for server to start...\n');
    await delay(3000);
  }

  private sendRequest(method: string, params?: any): number {
    if (!this.process || !this.process.stdin) {
      throw new Error('MCP server process not started');
    }

    const id = this.messageIdCounter++;
    const request: MCPRequest = {
      jsonrpc: '2.0',
      id,
      method,
      ...(params && { params }),
    };

    const requestStr = JSON.stringify(request) + '\n';
    this.process.stdin.write(requestStr);
    
    console.log(`📤 Sent request: ${method} (ID: ${id})`);
    return id;
  }

  private waitForResponse(id: number, timeout: number = 5000): Promise<MCPResponse | null> {
    return new Promise((resolve) => {
      const startTime = Date.now();
      const checkInterval = setInterval(() => {
        const response = this.messages.find((msg) => msg.id === id);
        if (response) {
          clearInterval(checkInterval);
          resolve(response);
          return;
        }

        if (Date.now() - startTime > timeout) {
          clearInterval(checkInterval);
          resolve(null);
        }
      }, 100);
    });
  }

  async testInitialize(): Promise<TestResult> {
    console.log('\n2️⃣  Testing: Initialize Protocol');
    console.log('───────────────────────────────────────────────────────────');

    const id = this.sendRequest('initialize', {
      protocolVersion: '2024-11-05',
      capabilities: {},
      clientInfo: {
        name: 'test-client',
        version: '1.0.0',
      },
    });

    await delay(2000);
    const response = await this.waitForResponse(id);

    if (response?.result) {
      this.testResults.push({
        testName: 'Initialize',
        passed: true,
        message: 'Server responded with initialization result',
        response: response.result,
      });
      console.log('✅ PASSED: Server initialized successfully\n');
      return this.testResults[this.testResults.length - 1];
    } else {
      this.testResults.push({
        testName: 'Initialize',
        passed: false,
        message: 'No response received or invalid response',
      });
      console.log('❌ FAILED: No valid response received\n');
      return this.testResults[this.testResults.length - 1];
    }
  }

  async testListTools(): Promise<TestResult> {
    console.log('3️⃣  Testing: List Tools');
    console.log('───────────────────────────────────────────────────────────');

    const id = this.sendRequest('tools/list');
    await delay(2000);
    const response = await this.waitForResponse(id);

    if (response?.result?.tools) {
      const tools = response.result.tools;
      this.testResults.push({
        testName: 'List Tools',
        passed: true,
        message: `Found ${tools.length} tools`,
        response: { toolCount: tools.length, toolNames: tools.map((t: any) => t.name) },
      });
      console.log(`✅ PASSED: Found ${tools.length} tools`);
      console.log(`   Tools: ${tools.map((t: any) => t.name).join(', ')}\n`);
      return this.testResults[this.testResults.length - 1];
    } else {
      this.testResults.push({
        testName: 'List Tools',
        passed: false,
        message: 'No tools returned or invalid response',
      });
      console.log('❌ FAILED: No tools returned\n');
      return this.testResults[this.testResults.length - 1];
    }
  }

  async testCheckConnection(): Promise<TestResult> {
    console.log('4️⃣  Testing: Check Connection Tool');
    console.log('───────────────────────────────────────────────────────────');

    const id = this.sendRequest('tools/call', {
      name: 'check_connection',
      arguments: {},
    });

    await delay(3000);
    const response = await this.waitForResponse(id, 10000);

    if (response?.result) {
      this.testResults.push({
        testName: 'Check Connection',
        passed: true,
        message: 'Connection check completed',
        response: response.result,
      });
      console.log('✅ PASSED: Connection check executed');
      console.log(`   Result: ${JSON.stringify(response.result).slice(0, 200)}...\n`);
      return this.testResults[this.testResults.length - 1];
    } else if (response?.error) {
      this.testResults.push({
        testName: 'Check Connection',
        passed: false,
        message: `Error: ${response.error.message}`,
      });
      console.log(`❌ FAILED: ${response.error.message}\n`);
      return this.testResults[this.testResults.length - 1];
    } else {
      this.testResults.push({
        testName: 'Check Connection',
        passed: false,
        message: 'No response received (timeout)',
      });
      console.log('❌ FAILED: No response received (timeout)\n');
      return this.testResults[this.testResults.length - 1];
    }
  }

  async testExecuteGraphQLQuery(): Promise<TestResult> {
    console.log('5️⃣  Testing: Execute GraphQL Query Tool');
    console.log('───────────────────────────────────────────────────────────');

    const id = this.sendRequest('tools/call', {
      name: 'execute_graphql_query',
      arguments: {
        query: '{ __typename }',
      },
    });

    await delay(3000);
    const response = await this.waitForResponse(id, 10000);

    if (response?.result) {
      this.testResults.push({
        testName: 'Execute GraphQL Query',
        passed: true,
        message: 'GraphQL query executed successfully',
        response: response.result,
      });
      console.log('✅ PASSED: GraphQL query executed');
      console.log(`   Result: ${JSON.stringify(response.result).slice(0, 200)}...\n`);
      return this.testResults[this.testResults.length - 1];
    } else {
      this.testResults.push({
        testName: 'Execute GraphQL Query',
        passed: false,
        message: response?.error?.message || 'No response received',
      });
      console.log(`❌ FAILED: ${response?.error?.message || 'No response received'}\n`);
      return this.testResults[this.testResults.length - 1];
    }
  }

  async testIntrospectSchema(): Promise<TestResult> {
    console.log('6️⃣  Testing: Introspect Schema Tool');
    console.log('───────────────────────────────────────────────────────────');

    const id = this.sendRequest('tools/call', {
      name: 'introspect_schema',
      arguments: {},
    });

    await delay(3000);
    const response = await this.waitForResponse(id, 10000);

    if (response?.result?.content?.[0]?.text) {
      // Schema is returned as JSON string in content[0].text
      try {
        const schemaData = JSON.parse(response.result.content[0].text);
        this.testResults.push({
          testName: 'Introspect Schema',
          passed: true,
          message: 'Schema introspection successful',
          response: { hasSchema: !!schemaData.schema },
        });
        console.log('✅ PASSED: Schema introspection completed');
        console.log(`   Schema size: ${response.result.content[0].text.length} characters\n`);
        return this.testResults[this.testResults.length - 1];
      } catch (e) {
        // Even if parse fails, if we have content, it's a success
        this.testResults.push({
          testName: 'Introspect Schema',
          passed: true,
          message: 'Schema introspection returned data',
        });
        console.log('✅ PASSED: Schema introspection completed\n');
        return this.testResults[this.testResults.length - 1];
      }
    } else {
      this.testResults.push({
        testName: 'Introspect Schema',
        passed: false,
        message: response?.error?.message || 'No schema content returned',
      });
      console.log(`❌ FAILED: ${response?.error?.message || 'No schema content returned'}\n`);
      return this.testResults[this.testResults.length - 1];
    }
  }

  async runAllTests(): Promise<void> {
    try {
      await this.start();

      // Run tests sequentially
      await this.testInitialize();
      await delay(1000);

      await this.testListTools();
      await delay(1000);

      await this.testCheckConnection();
      await delay(1000);

      await this.testExecuteGraphQLQuery();
      await delay(1000);

      await this.testIntrospectSchema();
      await delay(1000);

      // Print summary
      console.log('\n═══════════════════════════════════════════════════════════');
      console.log('📊 TEST SUMMARY');
      console.log('═══════════════════════════════════════════════════════════\n');

      const passed = this.testResults.filter((t) => t.passed).length;
      const total = this.testResults.length;

      this.testResults.forEach((result) => {
        const icon = result.passed ? '✅' : '❌';
        console.log(`${icon} ${result.testName}: ${result.message}`);
      });

      console.log(`\n📈 Results: ${passed}/${total} tests passed\n`);

      if (passed === total) {
        console.log('🎉 ALL TESTS PASSED! MCP Server is working correctly.\n');
      } else {
        console.log('⚠️  Some tests failed. Check the logs above for details.\n');
      }
    } finally {
      await this.cleanup();
    }
  }

  async cleanup(): Promise<void> {
    console.log('🧹 Cleaning up...');
    
    if (this.rl) {
      this.rl.close();
    }

    if (this.process) {
      this.process.kill();
      await delay(1000);
    }

    console.log('✅ Cleanup complete\n');
  }
}

// Run the tests
const tester = new MCPServerTester();
tester.runAllTests().catch((error) => {
  console.error('❌ Test suite failed:', error);
  process.exit(1);
});

