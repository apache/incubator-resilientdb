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
  currentSection?: 'topic' | 'plan' | 'pseudocode' | 'implementation';
} 