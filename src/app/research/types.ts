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
  currentSection?: 'reading-documents' | 'topic' | 'plan' | 'pseudocode' | 'implementation';
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
  implementationWeight?: number;
  theoreticalWeight?: number;
  qualityWeight?: number;
}

export interface AnalyzedChunk {
  content: string;
  originalScore: number;
  implementRel: number;
  codeQuality: number;
  queryAlignment: number;
  hasCodeExamples: boolean;
  finalScore: number;
  reasoning: string;
}

export interface AgentStats {
  totalChunks: number;
  analyzedChunks: number;
  selectedChunks: number;
  totalTokens: number;
  processingTime: number;
}
