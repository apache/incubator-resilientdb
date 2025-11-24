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
