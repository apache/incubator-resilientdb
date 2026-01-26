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

export interface CodeGeneration {
  id: string;
  language: string;
  query: string;
  topic: string;
  plan: string;
  pseudocode: string;
  implementation: string;
  hasStructuredResponse: boolean;
  timestamp: string;
  isStreaming?: boolean;
  currentSection?:
    | "reading-documents"
    | "topic"
    | "plan"
    | "pseudocode"
    | "implementation";
  sources?: {
    path: string;
    name: string;
    displayTitle?: string;
  }[];
}

export interface RetrievedNode {
  node: {
    getContent: (mode?: any) => string;
    metadata?: any;
  };
  score: number;
}

export interface AgentOptions {
  language?: string;
  scope?: string[];
  maxTokens?: number;
}

export interface AnalyzedChunk {
  content: string;
  originalScore: number;
  ir: number; // implementRel -> ir
  cq: number; // codeQuality -> cq
  qa: number; // queryAlignment -> qa
  ce: boolean; // hasCodeExamples -> ce
  finalScore: number;
  r: string; // reasoning -> r
}

export interface AgentStats {
  totalChunks: number;
  analyzedChunks: number;
  selectedChunks: number;
  totalTokens: number;
  processingTime: number;
}
