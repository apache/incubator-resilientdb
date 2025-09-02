export const TITLE_MAPPINGS: Record<string, string> = {
    "resilientdb.pdf": "ResilientDB: Global Scale Resilient Blockchain Fabric",
    "rcc.pdf":
      "Resilient Concurrent Consensus for High-Throughput Secure Transaction Processing",
    "bchain-transaction-pro.pdf": "Blockchain Transaction Processing",
  };

export const TOOL_CALL_MAPPINGS: Record<string, { input: string; output: string; error: string }> = {
    "search_web": {
      input: "Searching the web...",
      output: "Searched the web",
      error: "Error searching the web",
    },
    "search_documents": {
      input: "Reading",
      output: "Read", 
      error: "Error reading",
    },
}
export const MAX_TOKENS = 32000;
