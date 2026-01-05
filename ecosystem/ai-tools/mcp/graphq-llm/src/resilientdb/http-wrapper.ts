/**
 * Simple HTTP Wrapper for ResilientDB KV Service
 * 
 * Since the HTTP server isn't running in the container,
 * this provides a simple HTTP API that stores data directly.
 * 
 * This is a temporary solution until the full HTTP server is configured.
 */

import http from 'http';
import { URL } from 'url';

export interface KVStore {
  [key: string]: any;
}

// Simple in-memory store for now (can be replaced with direct KV service calls)
// In production, this should connect to the KV service via gRPC or direct API
const store: KVStore = {};

export class ResilientDBHTTPWrapper {
  private server: http.Server | null = null;
  private port: number;

  constructor(port: number = 18000) {
    this.port = port;
  }

  /**
   * Start the HTTP wrapper server
   */
  start(): Promise<void> {
    return new Promise((resolve, reject) => {
      this.server = http.createServer((req, res) => {
        this.handleRequest(req, res);
      });

      this.server.listen(this.port, '0.0.0.0', () => {
        console.log(`✅ ResilientDB HTTP Wrapper listening on port ${this.port}`);
        resolve();
      });

      this.server.on('error', (error) => {
        if ((error as any).code === 'EADDRINUSE') {
          console.log(`⚠️  Port ${this.port} already in use, assuming HTTP server is running`);
          resolve(); // Port is in use, assume server is running
        } else {
          reject(error);
        }
      });
    });
  }

  /**
   * Stop the HTTP wrapper server
   */
  stop(): Promise<void> {
    return new Promise((resolve) => {
      if (this.server) {
        this.server.close(() => {
          console.log('HTTP Wrapper server stopped');
          resolve();
        });
      } else {
        resolve();
      }
    });
  }

  /**
   * Handle HTTP requests
   */
  private async handleRequest(req: http.IncomingMessage, res: http.ServerResponse): Promise<void> {
    // Enable CORS
    res.setHeader('Access-Control-Allow-Origin', '*');
    res.setHeader('Access-Control-Allow-Methods', 'GET, POST, OPTIONS');
    res.setHeader('Access-Control-Allow-Headers', 'Content-Type');

    if (req.method === 'OPTIONS') {
      res.writeHead(200);
      res.end();
      return;
    }

    const url = new URL(req.url || '/', `http://${req.headers.host}`);
    const pathname = url.pathname;

    try {
      // POST /v1/transactions/commit - Store a key-value pair
      if (req.method === 'POST' && pathname === '/v1/transactions/commit') {
        const body = await this.readBody(req);
        const data = JSON.parse(body);

        if (!data.id || !data.value) {
          res.writeHead(400, { 'Content-Type': 'application/json' });
          res.end(JSON.stringify({ error: 'Missing id or value' }));
          return;
        }

        // Store the data
        store[data.id] = data.value;

        res.writeHead(200, { 'Content-Type': 'application/json' });
        res.end(JSON.stringify({ id: data.id, success: true }));
        return;
      }

      // GET /v1/transactions/<key> - Get a value
      if (req.method === 'GET' && pathname.startsWith('/v1/transactions/')) {
        const key = pathname.replace('/v1/transactions/', '');
        
        if (key === '' || key === 'test') {
          // List all or test endpoint
          res.writeHead(200, { 'Content-Type': 'application/json' });
          res.end(JSON.stringify(Object.keys(store)));
          return;
        }

        const value = store[key];
        if (value === undefined) {
          res.writeHead(404, { 'Content-Type': 'application/json' });
          res.end(JSON.stringify({ error: 'Not found' }));
          return;
        }

        res.writeHead(200, { 'Content-Type': 'application/json' });
        res.end(JSON.stringify({ id: key, value: value }));
        return;
      }

      // GET /v1/transactions - List all keys
      if (req.method === 'GET' && pathname === '/v1/transactions') {
        res.writeHead(200, { 'Content-Type': 'application/json' });
        res.end(JSON.stringify(Object.keys(store).map(key => ({ id: key }))));
        return;
      }

      // Default: 404
      res.writeHead(404, { 'Content-Type': 'application/json' });
      res.end(JSON.stringify({ error: 'Not found' }));
    } catch (error) {
      res.writeHead(500, { 'Content-Type': 'application/json' });
      res.end(JSON.stringify({ 
        error: error instanceof Error ? error.message : String(error) 
      }));
    }
  }

  /**
   * Read request body
   */
  private readBody(req: http.IncomingMessage): Promise<string> {
    return new Promise((resolve, reject) => {
      let body = '';
      req.on('data', (chunk) => {
        body += chunk.toString();
      });
      req.on('end', () => {
        resolve(body);
      });
      req.on('error', reject);
    });
  }

  /**
   * Get stored value (for testing)
   */
  getValue(key: string): any {
    return store[key];
  }

  /**
   * Get all keys (for testing)
   */
  getAllKeys(): string[] {
    return Object.keys(store);
  }
}

