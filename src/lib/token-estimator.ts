import { getEncoding, TiktokenEncoding } from "js-tiktoken";

// using this to estimate token count for the code reranker
export interface TokenEstimator {
    estimate(text: string): number;
  }
  
  export class TikTokenEstimator implements TokenEstimator {
    private encoder: any;
  
    constructor() {
      try {
        const encoding = "cl100k_base"; // default encoding for deepseek-chat
        this.encoder = getEncoding(encoding as TiktokenEncoding);
      } catch {
        this.encoder = null;
      }
    }
  
    estimate(text: string): number {
      try {
        const tokens = this.encoder.encode(text);
        return tokens.length;
      } catch {
        return Math.ceil(text.length / 4);
      }
    }
  
    dispose(): void {
      if (this.encoder && typeof this.encoder.free === 'function') {
        try {
          this.encoder.free();
        } catch {
        }
      }
    }
  }
  