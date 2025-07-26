export interface Document {
  id: string;
  name: string;
  path: string;
  size: number;
  uploadedAt: string;
  displayTitle?: string;
}

export interface CodeGeneration {
  id: string;
  language: string;
  query: string;
  plan: string;
  pseudocode: string;
  implementation: string;
  hasStructuredResponse: boolean;
  timestamp: string;
  isStreaming?: boolean;
  currentSection?: 'plan' | 'pseudocode' | 'implementation';
} 