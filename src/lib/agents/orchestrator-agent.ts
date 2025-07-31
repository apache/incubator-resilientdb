import chalk from "chalk";
import { FunctionTool, ReActAgent } from "llamaindex";
import { getCurrentLLM } from "../config/llama-settings";
import { DocumentAgent } from "./document-agent";

export interface QueryResult {
  response: string;
  sources: string[];
  toolCalls: Array<{
    tool: string;
    input: any;
    output: string;
  }>;
}

export interface OrchestratorAgentConfig {
  systemPrompt?: string;
  verbose?: boolean;
}

/**
 * OrchestratorAgent manages multiple DocumentAgents and routes queries intelligently
 * Uses ReActAgent with DeepSeek for tool selection and reasoning
 */
export class OrchestratorAgent {
  private agent!: ReActAgent;
  private documentAgents: Map<string, DocumentAgent> = new Map();
  private toolCallHistory: Array<any> = [];
  private config: OrchestratorAgentConfig;
  
  constructor(config: OrchestratorAgentConfig = {}) {
    // Settings should be configured at application startup
    
    this.config = {
      systemPrompt: config.systemPrompt || this.getDefaultSystemPrompt(),
      verbose: true,
    };
    
    // Check DeepSeek configuration for ReActAgent compatibility
    const currentLLM = getCurrentLLM();
    console.log(chalk.blue(`[OrchestratorAgent] LLM Type: ${currentLLM?.constructor?.name || 'Unknown'}`));
    
    if (currentLLM?.constructor?.name === 'DeepSeekLLM') {
      console.log(chalk.blue(`[OrchestratorAgent] DeepSeek LLM detected - ensuring ReActAgent compatibility`));
      console.log(chalk.blue(`[OrchestratorAgent] DeepSeek model: ${(currentLLM as any)?.model || 'Unknown'}`));
    }
    
    // Initialize with empty tools (will be updated as documents are added)
    this.createAgent();
    
    console.log(chalk.green("[OrchestratorAgent] OrchestratorAgent initialized"));
  }
  
  /**
   * Add an existing DocumentAgent to the orchestrator (preferred method)
   */
  addDocumentAgent(documentAgent: DocumentAgent): void {
    const documentPath = documentAgent.getDocumentPath();
    
    if (this.documentAgents.has(documentPath)) {
      return;
    }
    
    this.documentAgents.set(documentPath, documentAgent);
    
    // Recreate agent with updated tools
    this.createAgent();
    
    console.log(chalk.green(`[OrchestratorAgent] DocumentAgent added: ${documentAgent.getDisplayName()}`));
  }

  
  /**
   * Remove a document from the orchestrator
   */
  removeDocument(documentPath: string): boolean {
    const removed = this.documentAgents.delete(documentPath);
    if (removed) {
      this.createAgent(); // Recreate agent with updated tools
      console.log(chalk.green(`[OrchestratorAgent] Document removed: ${documentPath}`));
    }
    return removed;
  }
  
  /**
   * Check if a document exists
   */
  hasDocument(documentPath: string): boolean {
    return this.documentAgents.has(documentPath);
  }
  
  /**
   * Get all available document paths
   */
  getAvailableDocuments(): string[] {
    return Array.from(this.documentAgents.keys());
  }
  
  /**
   * Query the orchestrator - it will intelligently route to appropriate documents
   */
  async query(query: string): Promise<QueryResult> {
    // Add timeout wrapper to prevent hanging
    return Promise.race([
      this.executeQuery(query),
      new Promise<QueryResult>((_, reject) => 
        setTimeout(() => reject(new Error("Orchestrator query timeout after 60 seconds")), 60000)
      )
    ]);
  }

  /**
   * Execute the actual orchestrator query - SIMPLIFIED FOR DEBUGGING
   */
  private async executeQuery(query: string): Promise<QueryResult> {
    try {
      console.log(chalk.green("🚀 [OrchestratorAgent] ORCHESTRATOR QUERY STARTED"));
      console.log(chalk.green(`📝 [OrchestratorAgent] Query: "${query}"`));
      console.log(chalk.green(`🔧 [OrchestratorAgent] Available tools: ${this.documentAgents.size} DocumentAgents + 1 query_planner`));
      
      const startTime = Date.now();
      
      // Use ReActAgent to process the query
      console.log(chalk.blue("� [OrchestratorAgent] Calling agent.chat()..."));
      const response = await this.agent.chat({
        message: query,
      });
      
      const duration = Date.now() - startTime;
      console.log(chalk.green(`✅ [OrchestratorAgent] ORCHESTRATOR QUERY COMPLETED (${duration}ms)`));
      
      // LOG COMPLETE RESPONSE STRUCTURE
      console.log(chalk.magenta("=" .repeat(80)));
      console.log(chalk.magenta("🔍 COMPLETE REACTAGENT RESPONSE ANALYSIS"));
      console.log(chalk.magenta("=" .repeat(80)));
      
      console.log(chalk.cyan(`� Response type: ${typeof response}`));
      console.log(chalk.cyan(`� Response constructor: ${response?.constructor?.name || 'Unknown'}`));
      console.log(chalk.cyan(`� Response keys: [${Object.keys(response || {}).join(', ')}]`));
      
      // Log each property in detail
      Object.keys(response || {}).forEach(key => {
        const value = (response as any)[key];
        console.log(chalk.yellow(`🔍 ${key}: ${typeof value} ${Array.isArray(value) ? `(array, length: ${value.length})` : ''}`));
        if (key === 'reasons' && Array.isArray(value)) {
          value.forEach((reason: any, index: number) => {
            console.log(chalk.gray(`   [${index}] type: ${reason.type}, action: ${reason.action || 'none'}`));
          });
        }
      });
      
      // Log the response as JSON (truncated)
      try {
        const responseJson = JSON.stringify(response, null, 2);
        console.log(chalk.gray("📄 Full Response JSON:"));
        console.log(chalk.gray(responseJson));
      } catch (jsonError) {
        console.log(chalk.red(`❌ Failed to stringify response: ${jsonError}`));
      }
      
      console.log(chalk.magenta("=" .repeat(80)));
      
      // Extract response text
      const responseText = this.extractResponseText(response);
      
      // Parse tool calls from the text response (LlamaIndex 0.11.24 format)
      const toolCalls = this.parseToolCallsFromText(responseText);
      
      // Get unique sources from actual tool calls
      const sources = this.extractUniqueSources(toolCalls);
      
      console.log(chalk.green(`📊 [OrchestratorAgent] Tools used: ${toolCalls.length} tool calls`));
      
      if (toolCalls.length > 0) {
        console.log(chalk.green(`🔧 [OrchestratorAgent] Tools called: ${toolCalls.map(tc => tc.tool).join(", ")}`));
        console.log(chalk.green(`📋 [OrchestratorAgent] Sources used: ${sources.join(", ")}`));
      } else {
        console.log(chalk.yellow(`⚠️ [OrchestratorAgent] No tool calls found in response - this may indicate a parsing issue`));
      }
      
      // Extract the final answer from the response
      const finalAnswer = this.extractFinalAnswer(responseText);
      
      return {
        response: finalAnswer,
        sources,
        toolCalls,
      };
    } catch (error) {
      console.error(chalk.red(`❌ [OrchestratorAgent] Orchestrator query failed: ${error}`));
      throw new Error(`Query failed: ${error instanceof Error ? error.message : "Unknown error"}`);
    }
  }
  
  /**
   * Get summary of available documents
   */
  getDocumentsSummary(): {
    totalDocuments: number;
    availableDocuments: Array<{
      path: string;
      displayName: string;
      toolName: string;
    }>;
  } {
    const availableDocuments = Array.from(this.documentAgents.entries()).map(
      ([path, agent]) => ({
        path,
        displayName: agent.getDisplayName(),
        toolName: agent.getMetadata().name,
      })
    );
    
    return {
      totalDocuments: this.documentAgents.size,
      availableDocuments,
    };
  }
  
  /**
   * Create or recreate the ReActAgent with current tools
   */
  private createAgent(): void {
    const documentTools = Array.from(this.documentAgents.values())
      .map(agent => {
        const tool = agent.getTool();
        // Wrap tool with logging and query validation at the orchestrator level
        const originalCall = tool.call.bind(tool);
        tool.call = async (input: any) => {
          // Normalize and validate query input
          let normalizedInput: any;
          let queryText: string;
          
          if (typeof input === 'string') {
            queryText = input;
            normalizedInput = { query: input };
          } else if (input && typeof input === 'object') {
            queryText = input.query || input.input || input.text || JSON.stringify(input);
            normalizedInput = { query: queryText };
          } else {
            queryText = String(input || '');
            normalizedInput = { query: queryText };
          }
          
          // Fix common malformed queries
          if (queryText === '{}' || queryText === 'undefined' || queryText === 'null' || !queryText.trim()) {
            queryText = 'authors'; // Default fallback query
            normalizedInput = { query: queryText };
            console.log(chalk.yellow(`[DocumentAgent] ⚠️  Fixed malformed query, using fallback: "${queryText}"`));
          }
          
          console.log(chalk.blue(`[DocumentAgent] 🎯 TOOL CALLED: ${agent.getDisplayName()}`));
          console.log(chalk.blue(`[DocumentAgent] 📄 File: ${agent.getDocumentPath().split("/").pop()}`));
          console.log(chalk.blue(`[DocumentAgent] ❓ Query: "${queryText}"`));
          
          const startTime = Date.now();
          const result = await originalCall(normalizedInput);
          const duration = Date.now() - startTime;
          
          console.log(chalk.blue(`[DocumentAgent] ✅ TOOL COMPLETED: ${agent.getDisplayName()} (${duration}ms)`));
          return result;
        };
        return tool;
      });
    
    // Add query planning tool
    const planningTool = this.createQueryPlanningTool();
    const allTools = [...documentTools, planningTool];
    
    // Create ReActAgent with DeepSeek and enhanced debugging
    this.agent = new ReActAgent({
      tools: allTools,
      llm: getCurrentLLM(),
      verbose: true, // Force verbose for debugging ReActAgent issues
      systemPrompt: this.updateSystemPrompt(),
    });
    
    console.log(chalk.green(`[OrchestratorAgent] ReActAgent created with enhanced debugging enabled`));
    console.log(chalk.green(`[OrchestratorAgent] System prompt length: ${this.updateSystemPrompt().length} characters`));
    
    console.log(chalk.green(`[OrchestratorAgent] Agent updated with ${allTools.length} tools`));
    

  }
  
  /**
   * Create query planning tool for complex queries
   */
  private createQueryPlanningTool() {
    return FunctionTool.from(
      ({ originalQuery, documentsToSearch, reasoning }: { 
        originalQuery: string; 
        documentsToSearch: string[]; 
        reasoning: string; 
      }) => {
        console.log(chalk.green(`[OrchestratorAgent] Query plan: ${documentsToSearch.join(", ")}`));
        
        const availableDocs = this.getAvailableDocuments();
        const relevantDocs = documentsToSearch.filter(doc => 
          availableDocs.some(available => available.includes(doc) || doc.includes(available))
        );
        
        return `Query planning complete:
- Original Query: ${originalQuery}
- Target Documents: ${relevantDocs.join(", ")}
- Reasoning: ${reasoning}
- Available Tools: ${availableDocs.join(", ")}

Proceed to search the identified documents.`;
      },
      {
        name: "query_planner",
        description: "Plan how to answer complex queries by identifying which documents to search and reasoning about the approach",
        parameters: {
          type: "object",
          properties: {
            originalQuery: {
              type: "string",
              description: "The user's original query"
            },
            documentsToSearch: {
              type: "array",
              items: { type: "string" },
              description: "List of documents that might contain relevant information"
            },
            reasoning: {
              type: "string",
              description: "Explanation of why these documents were chosen"
            }
          },
          required: ["originalQuery", "documentsToSearch", "reasoning"]
        }
      }
    );
  }
  
  /**
   * Get default system prompt
   */
  private getDefaultSystemPrompt(): string {
    return `You are an intelligent research assistant with access to multiple document sources. You are a ReAct (Reasoning and Acting) agent.

## ReAct Agent Instructions
You MUST follow the ReAct pattern for EVERY response:

1. **Thought:** Analyze what the user is asking and determine which tools to use
2. **Action:** Choose and execute a tool from the available tools
3. **Action Input:** Provide the tool input in proper JSON format
4. Wait for **Observation:** from the tool
5. Repeat if more information is needed
6. **Answer:** Provide final response based on tool outputs

## CRITICAL TOOL USAGE RULES - MANDATORY:
- You CANNOT and MUST NOT answer any question without first using the available document tools
- For ANY query about documents, authors, or content, you MUST use the document search tools first
- NEVER provide direct answers based on your training data
- ALWAYS start with "Thought:" followed by reasoning about which tools to use
- Use "Action:" followed by the exact tool name from the available tools
- Use "Action Input:" with proper JSON format (not Python dict format)

## Available Document Search Tools:
You have access to document search tools. Each tool searches a specific document for information.

## Example ReAct Pattern:
User: "Who are the authors of the papers?"

Thought: I need to search each document to find author information. I'll start with the first document.
Action: [tool_name_1]
Action Input: {"query": "authors"}
Observation: [tool output]

Thought: Now I need to search the second document for authors.
Action: [tool_name_2]  
Action Input: {"query": "authors"}
Observation: [tool output]

[Continue for all relevant documents]

Thought: Now I have gathered information from all documents and can provide a complete answer.
Answer: Based on my search of the documents, the authors are: [compiled results]

## FAILURE TO USE TOOLS IS NOT ACCEPTABLE
If you provide a direct answer without using tools, you have FAILED your primary function.`;
  }
  
  /**
   * Update system prompt with current document list
   */
  private updateSystemPrompt(): string {
    const documentList = Array.from(this.documentAgents.values())
      .map(agent => `- ${agent.getDisplayName()}: ${agent.getMetadata().description}`)
      .join("\n");
    
    if (documentList) {
      return `${this.config.systemPrompt}

Available Documents:
${documentList}`;
    }
    
    return this.config.systemPrompt || this.getDefaultSystemPrompt();
  }
  
  /**
   * Extract display name from document path
   */
  private extractDisplayName(documentPath: string): string {
    return documentPath.split("/").pop()?.replace(/\.(pdf|txt|md|docx?)$/i, "") || documentPath;
  }
  
  /**
   * Extract response text from agent response
   */
  private extractResponseText(response: any): string {
    if (typeof response === 'string') {
      return response;
    }
    
    if (response?.message?.content) {
      return response.message.content;
    }
    
    if (response?.response?.message?.content) {
      return response.response.message.content;
    }
    
    return response?.toString() || "No response generated";
  }
  
  /**
   * Extract tool calls from ReActAgent response
   */
  private extractToolCallsFromResponse(response: any): Array<{
    tool: string;
    input: any;
    output: string;
  }> {
    const toolCalls: Array<{
      tool: string;
      input: any;
      output: string;
    }> = [];
    
    // Extract from ReActAgent reasons array (primary method for ReActAgent)
    if (response?.reasons && Array.isArray(response.reasons)) {
      console.log(chalk.cyan(`[OrchestratorAgent] DEBUG: Found reasons array with ${response.reasons.length} items`));
      response.reasons.forEach((reason: any, index: number) => {
        console.log(chalk.cyan(`[OrchestratorAgent] DEBUG: Reason ${index}: type=${reason.type}, action=${reason.action || 'none'}`));
        if (reason.type === 'action') {
          toolCalls.push({
            tool: reason.action,
            input: reason.input || {},
            output: 'Tool executed via ReActAgent'
          });
        }
      });
    } else {
      console.log(chalk.yellow(`[OrchestratorAgent] DEBUG: No reasons array found in response`));
    }
    
    // Extract from AgentRunner response format (alternative method)
    if (response?.response?.reasons && Array.isArray(response.response.reasons)) {
      console.log(chalk.cyan(`[OrchestratorAgent] DEBUG: Found response.reasons array with ${response.response.reasons.length} items`));
      response.response.reasons.forEach((reason: any, index: number) => {
        console.log(chalk.cyan(`[OrchestratorAgent] DEBUG: Response.Reason ${index}: type=${reason.type}, action=${reason.action || 'none'}`));
        if (reason.type === 'action') {
          toolCalls.push({
            tool: reason.action,
            input: reason.input || {},
            output: 'Tool executed via AgentRunner'
          });
        }
      });
    } else if (response?.response) {
      console.log(chalk.yellow(`[OrchestratorAgent] DEBUG: response.response exists but no reasons array`));
    }
    
    // Legacy: Try to extract tool calls from other response formats
    if (response?.toolCalls) {
      response.toolCalls.forEach((call: any) => {
        toolCalls.push({
          tool: call.tool || call.name || 'unknown',
          input: call.input || call.arguments || {},
          output: call.output || call.result || 'No output'
        });
      });
    }
    
    // Legacy: Check if response has sources or tool usage info
    if (response?.sources) {
      response.sources.forEach((source: any) => {
        if (source.tool) {
          toolCalls.push({
            tool: source.tool,
            input: source.input || {},
            output: source.content || 'Tool executed'
          });
        }
      });
    }
    
    // Legacy: Only extract from sourceNodes if they contain actual document content
    if (response?.sourceNodes && Array.isArray(response.sourceNodes) && response.sourceNodes.length > 0) {
      response.sourceNodes.forEach((node: any) => {
        if (node?.metadata?.source || node?.source) {
          const source = node.metadata?.source || node.source;
          // Find matching document agent by source path
          Array.from(this.documentAgents.values()).forEach(agent => {
            if (source.includes(agent.getDocumentPath()) || source.includes(agent.getDisplayName())) {
              toolCalls.push({
                tool: agent.getMetadata().name,
                input: { source: source },
                output: node.text || node.content || 'Document content retrieved'
              });
            }
          });
        }
      });
    }
    
    console.log(chalk.cyan(`[OrchestratorAgent] DEBUG: Final extraction result: ${toolCalls.length} tool calls found`));
    toolCalls.forEach((call, index) => {
      console.log(chalk.cyan(`[OrchestratorAgent] DEBUG: Tool call ${index}: ${call.tool}`));
    });
    
    return toolCalls;
  }
  
  /**
   * Extract unique sources from actual tool calls
   */
  private extractUniqueSources(actualToolCalls: Array<{tool: string; input: any; output: string}>): string[] {
    const sources = new Set<string>();
    
    // Only add document paths that were actually called via tools
    actualToolCalls.forEach(toolCall => {
      Array.from(this.documentAgents.values()).forEach(agent => {
        if (agent.getMetadata().name === toolCall.tool) {
          sources.add(agent.getDocumentPath());
        }
      });
    });
    
    return Array.from(sources);
  }

  /**
   * Parse tool calls from LlamaIndex 0.11.24 ReActAgent text response
   * Format: "Action: tool_name\nAction Input: {...}\nObservation: result"
   */
  private parseToolCallsFromText(responseText: string): Array<{tool: string; input: any; output: string}> {
    const toolCalls: Array<{tool: string; input: any; output: string}> = [];
    
    // Match Action/Action Input/Observation sequences
    const actionRegex = /Action:\s*([^\n]+)\s*Action Input:\s*({[^}]*}|\{[^{]*\}|[^\n]+)\s*Observation:\s*([^]*?)(?=(?:Thought:|Action:|Final Answer:|$))/gi;
    
    let match;
    while ((match = actionRegex.exec(responseText)) !== null) {
      const tool = match[1].trim();
      const inputStr = match[2].trim();
      const output = match[3].trim();
      
      let parsedInput;
      try {
        // Try to parse as JSON first
        parsedInput = JSON.parse(inputStr);
      } catch {
        // If not JSON, use as string
        parsedInput = { query: inputStr };
      }
      
      toolCalls.push({
        tool,
        input: parsedInput,
        output
      });
    }
    
    return toolCalls;
  }

  /**
   * Extract the final answer from ReActAgent response text
   */
  private extractFinalAnswer(responseText: string): string {
    // Look for "Final Answer:" section
    const finalAnswerMatch = responseText.match(/Final Answer:\s*([^]*?)$/i);
    if (finalAnswerMatch) {
      return finalAnswerMatch[1].trim();
    }
    
    // If no "Final Answer:" found, look for the last substantial text after the last "Observation:"
    const lastObservationMatch = responseText.match(/Observation:\s*([^]*?)(?=(?:Thought:|Action:|$))/gi);
    if (lastObservationMatch && lastObservationMatch.length > 0) {
      const lastObservation = lastObservationMatch[lastObservationMatch.length - 1];
      const cleanedObservation = lastObservation.replace(/^Observation:\s*/i, '').trim();
      if (cleanedObservation.length > 50) { // Only use if substantial
        return cleanedObservation;
      }
    }
    
    // Fallback: return the entire response if no clear structure
    return responseText;
  }

}