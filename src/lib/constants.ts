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


const message = `
The system provides robust failure detection and recovery while maintaining concurrent operation of non-failed instances, adhering to wait-free design principles.__TOOL_CALL__{"id":"call_00_IOdanqyXuu8Y2EITBGegfPJa","type":"handOff","state":"input-available","input":{"toAgent":"PseudoCodeAgent","reason":"Research complete, moving to pseudocode design for replica failure detection and recovery mechanisms"}}

__TOOL_CALL__{"id":"call_00_IOdanqyXuu8Y2EITBGegfPJa","type":"handOff","state":"output-available"}

 

 

Creating structured pseudocode...

## Replica Failure Detection and Recovery Pseudocode

### 1. GLOBAL CONSTANTS AND VARIABLES
`