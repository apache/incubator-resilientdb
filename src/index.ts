#!/usr/bin/env node

/**
 * GraphQ-LLM Main Entry Point
 * AI Query Tutor for ResilientDB
 */

import { MCPServer } from './mcp/server';
import { DataPipeline } from './pipeline/data-pipeline';
import { ResilientDBHTTPWrapper } from './resilientdb/http-wrapper';
import { LiveStatsService } from './services/live-stats-service';
import env from './config/environment';

async function main() {
  console.log('GraphQ-LLM: AI Query Tutor for ResilientDB');
  console.log('==========================================\n');

  // Check if running as MCP server
  if (process.argv.includes('--mcp-server')) {
    console.log('Starting MCP Server...');
    const mcpServer = new MCPServer();
    await mcpServer.start();
    return;
  }

  // Otherwise, run as standalone service
  console.log('Starting GraphQ-LLM service...\n');

  // Start HTTP wrapper for ResilientDB (if HTTP server isn't running)
  const httpWrapper = new ResilientDBHTTPWrapper(18000);
  try {
    await httpWrapper.start();
    console.log('✅ ResilientDB HTTP API available\n');
  } catch (error) {
    console.log('⚠️  HTTP wrapper not started (port may be in use - assuming server is running)\n');
  }

  // Start Live Stats Service if enabled
  const liveStatsService = new LiveStatsService();
  if (env.RESLENS_LIVE_MODE) {
    await liveStatsService.startPolling();
    console.log('✅ Live Stats Service started\n');
  }

  // Initialize pipeline
  const pipeline = new DataPipeline();

  // Health check
  console.log('Performing health checks...');
  const health = await pipeline.healthCheck();
  console.log(`ResilientDB: ${health.resilientdb ? '✓' : '✗'}`);
  console.log(`ResLens: ${health.reslens ? '✓' : '✗'}`);
  console.log(`Live Stats: ${health.liveStats ? '✓' : '✗'}\n`);

  if (!health.resilientdb) {
    console.warn(
      'Warning: ResilientDB connection failed. Please check your configuration.'
    );
  }

  console.log('GraphQ-LLM service initialized.');
  console.log('Available modes:');
  console.log('  - MCP Server: npm run dev -- --mcp-server');
  console.log('  - Standalone: npm run dev');
  console.log('\nConfiguration:');
  console.log(`  ResilientDB: ${env.RESILIENTDB_GRAPHQL_URL}`);
  console.log(`  MCP Server: ${env.MCP_SERVER_HOST}:${env.MCP_SERVER_PORT}`);
  console.log(`  Log Level: ${env.LOG_LEVEL}\n`);
}

// Handle graceful shutdown
process.on('SIGINT', () => {
  console.log('\nShutting down GraphQ-LLM...');
  process.exit(0);
});

process.on('SIGTERM', () => {
  console.log('\nShutting down GraphQ-LLM...');
  process.exit(0);
});

main().catch((error) => {
  console.error('Fatal error:', error);
  process.exit(1);
});

