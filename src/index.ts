#!/usr/bin/env node

/**
 * GraphQ-LLM Main Entry Point
 * AI Query Tutor for ResilientDB
 */

import { MCPServer } from './mcp/server';
import { DataPipeline } from './pipeline/data-pipeline';
import { ResilientDBHTTPWrapper } from './resilientdb/http-wrapper';
import { LiveStatsService } from './services/live-stats-service';
import { HTTPServer } from './api/http-server';
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

  // Check if running as HTTP API server
  if (process.argv.includes('--http-api')) {
    console.log('Starting HTTP API Server...');
    const httpServer = new HTTPServer(env.MCP_SERVER_PORT);
    await httpServer.start();
    // Keep server running
    process.on('SIGINT', async () => {
      await httpServer.stop();
      process.exit(0);
    });
    process.on('SIGTERM', async () => {
      await httpServer.stop();
      process.exit(0);
    });
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
  console.log(`Live Stats: ${health.liveStats ? '✓' : '✗'}`);
  console.log(`Nexus: ${health.nexus ? '✓' : '✗'}\n`);

  if (!health.resilientdb) {
    console.warn(
      'Warning: ResilientDB connection failed. Please check your configuration.'
    );
  }

  if (!health.nexus) {
    console.warn(
      'Info: Nexus integration not available. Set NEXUS_API_URL to enable Nexus features.'
    );
  }

  // Start HTTP API server for Nexus integration
  httpServerInstance = new HTTPServer(env.MCP_SERVER_PORT);
  await httpServerInstance.start();
  console.log('');

  console.log('GraphQ-LLM service initialized.');
  console.log('Available modes:');
  console.log('  - MCP Server: npm run dev -- --mcp-server');
  console.log('  - HTTP API: npm run dev -- --http-api');
  console.log('  - Standalone: npm run dev (includes HTTP API)');
  console.log('\nConfiguration:');
  console.log(`  ResilientDB: ${env.RESILIENTDB_GRAPHQL_URL}`);
  console.log(`  Nexus: ${env.NEXUS_API_URL || 'Not configured'}`);
  console.log(`  MCP Server: ${env.MCP_SERVER_HOST}:${env.MCP_SERVER_PORT}`);
  console.log(`  Log Level: ${env.LOG_LEVEL}\n`);
}

// Handle graceful shutdown (for standalone mode)
let httpServerInstance: HTTPServer | null = null;
process.on('SIGINT', async () => {
  console.log('\nShutting down GraphQ-LLM...');
  if (httpServerInstance) {
    await httpServerInstance.stop();
  }
  process.exit(0);
});

process.on('SIGTERM', async () => {
  console.log('\nShutting down GraphQ-LLM...');
  if (httpServerInstance) {
    await httpServerInstance.stop();
  }
  process.exit(0);
});

main().catch((error) => {
  console.error('Fatal error:', error);
  process.exit(1);
});

