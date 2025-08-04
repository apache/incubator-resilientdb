import { CodeComposerContext, generateCodeComposerPrompt, parseChainOfThoughtResponse } from "@/lib/code-composer-prompts";
import { configureLlamaSettings } from "@/lib/config/llama-settings";
import { TITLE_MAPPINGS } from "@/lib/constants";
import { queryEngine } from "@/lib/query-engine";
import { DeepSeekLLM } from "@llamaindex/deepseek";
import { Settings } from "llamaindex";
import { NextRequest, NextResponse } from "next/server";
import { config } from "../../../../config/environment";

// Simple system prompt for document Q&A
const RESEARCH_SYSTEM_PROMPT = `
You are Nexus, an AI research assistant specialized in Apache ResilientDB and its related blockchain technology, distributed systems, and fault-tolerant consensus protocols. Your primary role is to help students, researchers, and practitioners understand complex technical concepts related to Apache ResilientDB and blockchain systems, who can answer questions about documents. 
You have access to the content of a document and can provide accurate, detailed answers based on that content.
When asked about the document, always base your responses on the information provided in the document. When possible, cite sections, pages, or other specific information from the document.
If you cannot find specific information in the document, say so clearly.
Please favor referring to the document by its title, instead of the file name.
If asked about something that is not in the document, give a brief answer and try to guide the user to ask about something that is in the document.
`;

interface RequestData {
  query: string;
  documentPath?: string;
  documentPaths?: string[];
  tool?: string;
  language?: string;
  scope?: string[];
}

interface CitationInfo {
  id: number;
  page: number;
  path: string;
  name: string;
  displayTitle: string;
}

interface SourceInfo {
  sources: Array<{
    path: string;
    name: string;
    displayTitle: string;
    pages: number[];
  }>;
  isMultiDocument: boolean;
  totalDocuments: number;
  contextNodes: number;
  tool: string;
  language: string;
  scope: string[];
  citations: CitationInfo[];
}

const validateRequest = (data: RequestData): string | null => {
  const { query, documentPath, documentPaths } = data;

  if (!query) {
    return "Query is required";
  }

  if (!documentPath && !documentPaths) {
    return "Either documentPath or documentPaths is required";
  }

  if (documentPaths && (!Array.isArray(documentPaths) || documentPaths.length === 0)) {
    return "documentPaths must be a non-empty array";
  }

  if (!config.deepSeekApiKey) {
    return "DeepSeek API key is required";
  }

  return null;
};

// Configuration is now handled centrally via configureLlamaSettings()

// Get context and sources from queryEngine
const getContextFromQueryEngine = async (
  query: string, 
  targetPaths: string[], 
  tool?: string,
  language?: string,
  scope?: string[]
) => {
  // Use queryEngine for context retrieval with tool-specific options
  const queryResult = await queryEngine.query(
    query,
    targetPaths,
    {
      enableStreaming: true,
      topK: tool === "code-composer" ? 8 : 5,
      tool,
      language,
      scope
    }
  );

  return {
    context: queryResult.context,
    sources: queryResult.sources,
    chunks: queryResult.chunks || [],
    totalChunks: queryResult.totalChunks || 0
  };
};

// Format context from retrieved chunks similar to route-old.ts
const formatContext = (retrievedChunks: any[]): string => {
  const contextBySource: { [key: string]: string[] } = {};

  retrievedChunks.forEach((chunk: any) => {
    const sourceDoc = chunk.source || "Unknown";
    if (!contextBySource[sourceDoc]) {
      contextBySource[sourceDoc] = [];
    }
    contextBySource[sourceDoc].push(chunk.content);
  });

  const contextParts = Object.entries(contextBySource).map(
    ([source, contents]) => {
      const fileName = source.split("/").pop() || source;
      return `**From ${fileName}:**\n${contents.join("\n\n")}`;
    },
  );

  return contextParts.join("\n\n---\n\n");
};

// Generate prompt for different tools
const generatePrompt = (
  tool: string | undefined,
  context: string,
  query: string,
  documentPaths: string[],
  language?: string,
  scope?: string[],
  sourceInfo?: SourceInfo
): string => {
  let citationSection = "";
  if (tool === "code-composer") {
    const codeComposerContext: CodeComposerContext = {
      language: language || "ts",
      scope: scope || [],
      chunks: context,
      query,
      documentTitles: documentPaths.map((p: string) => p.split("/").pop()?.replace(".pdf", "") || p)
    };
    return generateCodeComposerPrompt(codeComposerContext);
  }else{
  let citationSection = "";
  if (sourceInfo && sourceInfo.citations.length) {
    const citationLines = sourceInfo.citations.map((c) => `[^${c.id}]" (Page ${c.page} of ${c.displayTitle})`).join("\n");
    citationSection = `\nWhen you use information from the documents, append a citation like [^id] right after the statement.\n\nCitations:\n${citationLines}\n
    \n Multiple of the same citation in a response is fine, but if multiple statements/paragraphs reference the same citation in a row, only append the citation once before digging into those paragraphs so that the citation is only shown once.
    `;
  }
  }

  return `${RESEARCH_SYSTEM_PROMPT}${citationSection}

Documents: ${documentPaths.map((p: string) => p.split("/").pop()).join(", ")}

Retrieved Context:
${context}

Question: ${query}`;
};

// Create source info for the UI
const createSourceInfo = (
  data: RequestData,
  retrievedData: any[],
  documentPaths: string[]
): SourceInfo => {
  const { tool, language, scope } = data;

  // Build pages map and citations list
  const pagesByDoc: Record<string, number[]> = {};

  const citations: CitationInfo[] = [];
  let citationId = 1;

  retrievedData.forEach((chunk: any) => {
    const docPath = chunk.source;
    const pageNumber = chunk.metadata?.page;
    if (pageNumber === undefined) return;

    const citationKey = `${docPath}|${pageNumber}`;
    if (!citations.some(c => `${c.path}|${c.page}` === citationKey)) {
      citations.push({ id: citationId++, page: pageNumber, path: docPath, name: docPath.split("/").pop() || docPath, displayTitle: TITLE_MAPPINGS[docPath.split("/").pop() || docPath] || docPath });
    }

    if (!pagesByDoc[docPath]) {
      pagesByDoc[docPath] = [];
    }

    // Avoid duplicate page entries
    if (!pagesByDoc[docPath].includes(pageNumber)) {
      pagesByDoc[docPath].push(pageNumber);
    }
  });

  return {
    sources: documentPaths.map((path: string) => ({
      path,
      name: path.split("/").pop() || path,
      displayTitle: TITLE_MAPPINGS[path.split("/").pop() || path] || path,
      pages: pagesByDoc[path] || [],
    })),
    isMultiDocument: documentPaths.length > 1,
    totalDocuments: documentPaths.length,
    contextNodes: retrievedData.length,
    tool: tool || "default",
    language: language || "ts",
    scope: scope || [],
    citations,
  };
};

// Handle streaming response using LlamaIndex native streaming
const handleStreamingResponse = async (
  chatStream: any,
  sourceInfo: SourceInfo,
  tool?: string
): Promise<ReadableStream> => {
  return new ReadableStream({
    async start(controller) {
      try {

        if (tool === "code-composer") {
          controller.enqueue(`__SOURCE_INFO__${JSON.stringify(sourceInfo)}\n\n`);
          let fullResponse = "";
          for await (const chunk of chatStream) {
            const content = chunk.delta;
            if (content) {
              fullResponse += content;
              controller.enqueue(content);
            }
          }

          const parsed = parseChainOfThoughtResponse(fullResponse);
          controller.enqueue(
            `\n\n__CODE_COMPOSER_META__${JSON.stringify({
              hasStructuredResponse: parsed.hasStructuredResponse,
              planLength: parsed.plan.length,
              pseudocodeLength: parsed.pseudocode.length,
              implementationLength: parsed.implementation.length
            })}\n\n`
          );
        } else {
          for await (const chunk of chatStream) {
            const content = chunk.delta;
            if (content) {
              controller.enqueue(content);
            }
          }
          controller.enqueue(`__SOURCE_INFO__${JSON.stringify(sourceInfo)}\n\n`);
        }
        controller.close();
      } catch (error) {
        controller.error(error);
      }
    },
  });
};

// Get error message for API errors
const getErrorMessage = (error: any): string => {
  if (!(error instanceof Error)) {
    return "Failed to process your question";
  }

  if (error.message.includes("401") || error.message.includes("unauthorized")) {
    return "Invalid API key. Please check your DeepSeek API key.";
  }

  if (error.message.includes("402") || error.message.includes("payment")) {
    return "Insufficient credits. Please check your DeepSeek account balance.";
  }

  return "Failed to process your question";
};

export async function POST(req: NextRequest) {
  try {
    const requestData: RequestData = await req.json();

    const validationError = validateRequest(requestData);
    if (validationError) {
      return NextResponse.json({ error: validationError }, { status: 400 });
    }

    // Ensure settings are configured
    configureLlamaSettings();

    try {
      const documentPaths = requestData.documentPaths!;

      // Use queryEngine with tool-specific options - now handles code-composer internally
      const { context, chunks } = await getContextFromQueryEngine(
        requestData.query,
        documentPaths,
        requestData.tool,
        requestData.language,
        requestData.scope
      );

      console.log(`Retrieved ${chunks.length} chunks from ${documentPaths.length} documents${requestData.tool ? ` (${requestData.tool})` : ''}`);

      const sourceInfo = createSourceInfo(requestData, chunks, documentPaths);


      // Generate enhanced prompt (includes citation instructions)
      const enhancedPrompt = generatePrompt(
        requestData.tool,
        context,
        requestData.query,
        documentPaths,
        requestData.language,
        requestData.scope,
        sourceInfo
      );

      // Create chat stream using native LlamaIndex streaming
      const deepSeekLLM = Settings.llm as DeepSeekLLM;
      const chatStream = await deepSeekLLM.chat({
        messages: [{ role: "user", content: enhancedPrompt }],
        stream: true,
      });


      // Handle streaming response using native LlamaIndex streaming
      const readableStream = await handleStreamingResponse(
        chatStream,
        sourceInfo,
        requestData.tool
      );

      return new Response(readableStream, {
        headers: {
          "Content-Type": "text/plain; charset=utf-8",
        },
      });

    } catch (processingError) {
      console.error("Error processing chat:", processingError);

      const errorMessage = getErrorMessage(processingError);
      return NextResponse.json(
        {
          error: errorMessage,
          details: processingError instanceof Error ? processingError.message : String(processingError),
        },
        { status: 500 },
      );
    }

  } catch (error) {
    console.error("Error in chat API:", error);
    return NextResponse.json(
      {
        error: "Internal server error",
        details: error instanceof Error ? error.message : String(error),
      },
      { status: 500 },
    );
  }
}