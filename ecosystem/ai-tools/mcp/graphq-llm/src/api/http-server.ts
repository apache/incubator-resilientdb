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

/**
 * GraphQ-LLM HTTP API Server
 * Exposes REST endpoints for Nexus and other clients
 */

import http from 'http';
import { URL } from 'url';
import { ExplanationService } from '../llm/explanation-service';
import { OptimizationService } from '../llm/optimization-service';
import { EfficiencyEstimator } from '../services/efficiency-estimator';
import { QueryAnalyzer } from '../services/query-analyzer';

export class HTTPServer {
  private server: http.Server;
  private explanationService: ExplanationService;
  private optimizationService: OptimizationService;
  private efficiencyEstimator: EfficiencyEstimator;
  private queryAnalyzer: QueryAnalyzer;
  private port: number;

  constructor(port: number = 3001) {
    this.port = port;
    this.explanationService = new ExplanationService();
    this.optimizationService = new OptimizationService();
    this.efficiencyEstimator = new EfficiencyEstimator();
    this.queryAnalyzer = new QueryAnalyzer();
    this.server = http.createServer(this.handleRequest.bind(this));
  }

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
    const path = url.pathname;

    try {
      if (req.method === 'POST') {
        const body = await this.readBody(req);
        const data = JSON.parse(body);

        if (path === '/api/explanations/explain') {
          await this.handleExplain(res, data);
        } else if (path === '/api/optimizations/optimize') {
          await this.handleOptimize(res, data);
        } else if (path === '/api/efficiency/estimate') {
          await this.handleEfficiency(res, data);
        } else if (path === '/api/analyze') {
          await this.handleAnalyze(res, data);
        } else {
          this.sendError(res, 404, 'Endpoint not found');
        }
      } else if (req.method === 'GET' && path === '/health') {
        this.sendJSON(res, 200, { status: 'ok', service: 'graphq-llm-api' });
      } else {
        this.sendError(res, 405, 'Method not allowed');
      }
    } catch (error) {
      console.error('API error:', error);
      this.sendError(res, 500, error instanceof Error ? error.message : 'Internal server error');
    }
  }

  private async handleExplain(res: http.ServerResponse, data: any): Promise<void> {
    const { query, detailed, includeLiveStats } = data;

    if (!query || typeof query !== 'string') {
      this.sendError(res, 400, 'Query is required');
      return;
    }

    try {
      // Get explanation from RAG + LLM (uses documentation context)
      const explanation = await this.explanationService.explain(query, {
        detailed: detailed ?? true,
        includeLiveStats: includeLiveStats ?? false,
      });

      // Get complexity from QueryAnalyzer (structural analysis)
      const queryAnalysis = this.queryAnalyzer.analyze(query);

      this.sendJSON(res, 200, {
        ...explanation,
        complexity: queryAnalysis.complexity, // From QueryAnalyzer, not hardcoded
        fieldCount: queryAnalysis.fieldCount,
        depth: queryAnalysis.depth,
      });
    } catch (error) {
      console.error('Explanation error:', error);
      this.sendError(res, 500, 'Failed to generate explanation');
    }
  }

  private async handleOptimize(res: http.ServerResponse, data: any): Promise<void> {
    const { query, includeSimilarQueries } = data;

    if (!query || typeof query !== 'string') {
      this.sendError(res, 400, 'Query is required');
      return;
    }

    try {
      const optimization = await this.optimizationService.optimize(
        query,
        {
          includeSimilarQueries: includeSimilarQueries ?? true,
        }
      );

      this.sendJSON(res, 200, optimization);
    } catch (error) {
      console.error('Optimization error:', error);
      this.sendError(res, 500, 'Failed to generate optimizations');
    }
  }

  private async handleEfficiency(res: http.ServerResponse, data: any): Promise<void> {
    const { query, useLiveStats } = data;

    if (!query || typeof query !== 'string') {
      this.sendError(res, 400, 'Query is required');
      return;
    }

    try {
      // Efficiency uses ResLens for similar query metrics + QueryAnalyzer for structure
      const efficiency = await this.efficiencyEstimator.estimateEfficiency(
        query,
        {
          useLiveStats: useLiveStats ?? false,
          includeSimilarQueries: true, // Use ResLens to get similar queries
        }
      );

      // Also include complexity from QueryAnalyzer
      const queryAnalysis = this.queryAnalyzer.analyze(query);

      this.sendJSON(res, 200, {
        ...efficiency,
        complexity: queryAnalysis.complexity, // From QueryAnalyzer
        queryAnalysis: {
          fieldCount: queryAnalysis.fieldCount,
          depth: queryAnalysis.depth,
          opportunities: queryAnalysis.opportunities,
        },
      });
    } catch (error) {
      console.error('Efficiency estimation error:', error);
      this.sendError(res, 500, 'Failed to estimate efficiency');
    }
  }

  private async handleAnalyze(res: http.ServerResponse, data: any): Promise<void> {
    const { query } = data;

    if (!query || typeof query !== 'string') {
      this.sendError(res, 400, 'Query is required');
      return;
    }

    try {
      // Analyze query structure first (for complexity)
      const queryAnalysis = this.queryAnalyzer.analyze(query);

      // Get all three analyses in parallel
      const [explanation, optimization, efficiency] = await Promise.allSettled([
        this.explanationService.explain(query, { detailed: true, includeLiveStats: false }),
        this.optimizationService.optimize(query, { includeSimilarQueries: true }),
        this.efficiencyEstimator.estimateEfficiency(query, { 
          useLiveStats: false,
          includeSimilarQueries: true, // Use ResLens
        }),
      ]);

      this.sendJSON(res, 200, {
        explanation:
          explanation.status === 'fulfilled'
            ? {
                explanation: explanation.value.explanation, // From RAG + LLM (uses docs)
                complexity: queryAnalysis.complexity, // From QueryAnalyzer
                fieldCount: queryAnalysis.fieldCount,
                depth: queryAnalysis.depth,
                recommendations: queryAnalysis.opportunities.map(opp => opp.description), // From QueryAnalyzer
              }
            : {
                explanation: 'Failed to generate explanation',
                complexity: queryAnalysis.complexity, // Still provide complexity even if explanation fails
                recommendations: [],
              },
        optimizations:
          optimization.status === 'fulfilled'
            ? (optimization.value.suggestions || []).map((s: any) => ({
                query: s.optimizedQuery || optimization.value.query,
                explanation: s.reason || s.suggestion,
                confidence: 0.8, // Default confidence
              }))
            : [],
        efficiency:
          efficiency.status === 'fulfilled'
            ? {
                score: efficiency.value.efficiencyScore, // From EfficiencyEstimator (uses ResLens)
                estimatedTime: `${efficiency.value.estimatedExecutionTime}ms`, // From ResLens similar queries
                resourceUsage: efficiency.value.estimatedResourceUsage
                  ? `CPU: ${efficiency.value.estimatedResourceUsage.cpu || 'N/A'}%, Memory: ${efficiency.value.estimatedResourceUsage.memory || 'N/A'}MB`
                  : 'Unknown',
                recommendations: efficiency.value.recommendations || [],
                complexity: queryAnalysis.complexity, // Also include complexity
                similarQueries: efficiency.value.similarQueries, // From ResLens
              }
            : {
                score: 0,
                estimatedTime: 'Unknown',
                resourceUsage: 'Unknown',
                recommendations: [],
                complexity: queryAnalysis.complexity, // Still provide complexity
              },
      });
    } catch (error) {
      console.error('Analysis error:', error);
      this.sendError(res, 500, 'Failed to analyze query');
    }
  }

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

  private sendJSON(res: http.ServerResponse, status: number, data: any): void {
    res.writeHead(status, { 'Content-Type': 'application/json' });
    res.end(JSON.stringify(data));
  }

  private sendError(res: http.ServerResponse, status: number, message: string): void {
    this.sendJSON(res, status, { error: message });
  }

  async start(): Promise<void> {
    return new Promise((resolve) => {
      this.server.listen(this.port, () => {
        console.log(`✅ GraphQ-LLM HTTP API server running on port ${this.port}`);
        console.log(`   Endpoints:`);
        console.log(`   - POST /api/explanations/explain`);
        console.log(`   - POST /api/optimizations/optimize`);
        console.log(`   - POST /api/efficiency/estimate`);
        console.log(`   - POST /api/analyze (full analysis)`);
        console.log(`   - GET  /health`);
        resolve();
      });
    });
  }

  async stop(): Promise<void> {
    return new Promise((resolve) => {
      this.server.close(() => {
        console.log('GraphQ-LLM HTTP API server stopped');
        resolve();
      });
    });
  }
}

