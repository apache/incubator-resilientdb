import { CodeComposerAgent } from "@/lib/code-composer-agent";
import { CodeComposerContext, generateCodeComposerPrompt, parseChainOfThoughtResponse } from "@/lib/code-composer-prompts";
import { documentIndexManager } from "@/lib/document-index-manager";
import { DeepSeekLLM } from "@llamaindex/deepseek";
import { HuggingFaceEmbedding } from "@llamaindex/huggingface";
import { MetadataMode, Settings } from "llamaindex";
import { NextRequest, NextResponse } from "next/server";
import { config } from "../../../../config/environment";

// simple system prompt for document q&a
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

interface SourceInfo {
  sources: Array<{
    path: string;
    name: string;
    displayTitle: string;
  }>;
  isMultiDocument: boolean;
  totalDocuments: number;
  contextNodes: number;
  tool: string;
  language: string;
  scope: string[];
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

const configureSettings = (): void => {
  Settings.llm = new DeepSeekLLM({
    apiKey: config.deepSeekApiKey,
    model: config.deepSeekModel,
  });

  try {
    Settings.embedModel = new HuggingFaceEmbedding() as any;
  } catch (error) {
    console.warn("Failed to initialize HuggingFace embedding:", error);
    Settings.embedModel = new HuggingFaceEmbedding() as any;
  }
};

const getDocumentIndex = async (targetPaths: string[]) => {
  const hasAllIndices = documentIndexManager.hasAllIndices(targetPaths);
  if (!hasAllIndices) {
    throw new Error("Some document indices not found. Please prepare all selected documents again");
  }

  const documentIndex = await documentIndexManager.getCombinedIndex(targetPaths);
  if (!documentIndex) {
    throw new Error("Failed to create combined index. Please try selecting documents again");
  }

  console.log(`Using combined index for ${targetPaths.length} documents: ${targetPaths.join(", ")}`);
  return documentIndex;
};

const retrieveAndRankContext = async (documentIndex: any, query: string, tool?: string, requestData?: RequestData) => {
  const topK = tool === "code-composer" ? 8 : 8;
  const retriever = documentIndex.asRetriever({ similarityTopK: topK });
  const initialNodes = await retriever.retrieve({ query });

  let retrievedNodes;
  if (tool === "code-composer") {
    try {
      // Use AI SDK agent for intelligent context ranking
      const deepSeekLLM = Settings.llm as DeepSeekLLM;
      const codeComposerAgent = new CodeComposerAgent(deepSeekLLM, {
        maxTokens: 32000,
        implementationWeight: 1.5,
        theoreticalWeight: 1.0,
        qualityWeight: 2.0
      });

      retrievedNodes = await codeComposerAgent.rerank(initialNodes, query, {
        language: requestData?.language || 'ts',
        scope: requestData?.scope || [],
      });

      console.log("code composer agent stats:", codeComposerAgent.getStats());
    } catch (error) {
      console.error("CodeComposerAgent failed, falling back to CodeReranker:", error);

      // // Fallback to original reranker
      // const codeReranker = new CodeReranker({
      //   maxTokens: 32000,
      //   implementationWeight: 1.5,
      //   theoreticalWeight: 1.0,
      //   qualityWeight: 2.0,
      //   codeBlockBonus: 1.5
      // });

      // retrievedNodes = codeReranker.rerank(initialNodes);
      // console.log("code reranker stats (fallback):", codeReranker.getStats(initialNodes, retrievedNodes));
    }
  } else {
    retrievedNodes = initialNodes;
  }

  return retrievedNodes;
};

const formatContext = (retrievedNodes: any[]): string => {
  const contextBySource: { [key: string]: string[] } = {};

  retrievedNodes.forEach((node: any) => {
    const sourceDoc = node.node.metadata?.source_document || "Unknown";
    if (!contextBySource[sourceDoc]) {
      contextBySource[sourceDoc] = [];
    }
    contextBySource[sourceDoc].push(node.node.getContent(MetadataMode.ALL));
  });

  const contextParts = Object.entries(contextBySource).map(
    ([source, contents]) => {
      const fileName = source.split("/").pop() || source;
      return `**From ${fileName}:**\n${contents.join("\n\n")}`;
    },
  );

  return contextParts.join("\n\n---\n\n");
};

const generatePrompt = (
  tool: string | undefined,
  context: string,
  query: string,
  targetPaths: string[],
  language?: string,
  scope?: string[]
): string => {
  if (tool === "code-composer") {
    const codeComposerContext: CodeComposerContext = {
      language: language || "ts",
      scope: scope || [],
      chunks: context,
      query,
      documentTitles: targetPaths.map((p: string) => p.split("/").pop()?.replace(".pdf", "") || p)
    };
    return generateCodeComposerPrompt(codeComposerContext);
  }

  return `${RESEARCH_SYSTEM_PROMPT}

Documents: ${targetPaths.map((p: string) => p.split("/").pop()).join(", ")}

Retrieved Context:
${context}

Question: ${query}`;
};

const createSourceInfo = (
  data: RequestData,
  retrievedNodes: any[],
  targetPaths: string[]
): SourceInfo => {
  const { documentPath, documentPaths, tool, language, scope } = data;
  const sourcePaths = documentPaths || (documentPath ? [documentPath] : []);

  return {
    sources: sourcePaths.map((path: string) => ({
      path,
      name: path.split("/").pop() || path,
      displayTitle: path.split("/").pop()?.replace(".pdf", "") || path,
    })),
    isMultiDocument: !!documentPaths,
    totalDocuments: documentPaths ? documentPaths.length : 1,
    contextNodes: retrievedNodes.length,
    tool: tool || "default",
    language: language || "ts",
    scope: scope || []
  };
};

const handleStreamingResponse = async (
  chatStream: any,
  sourceInfo: SourceInfo,
  tool?: string
): Promise<ReadableStream> => {
  return new ReadableStream({
    async start(controller) {
      try {
        controller.enqueue(`__SOURCE_INFO__${JSON.stringify(sourceInfo)}\n\n`);

        if (tool === "code-composer") {
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
        }

        controller.close();
      } catch (error) {
        controller.error(error);
      }
    },
  });
};

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

    configureSettings();

    try {
      const targetPaths = requestData.documentPaths!;

      const documentIndex = await getDocumentIndex(targetPaths);

      const retrievedNodes = await retrieveAndRankContext(
        documentIndex,
        requestData.query,
        requestData.tool,
        requestData
      );

      // format context
      const context = formatContext(retrievedNodes);

      console.log(`Retrieved ${retrievedNodes.length} nodes from ${targetPaths.length} documents`);

      const enhancedPrompt = generatePrompt(
        requestData.tool,
        context,
        requestData.query,
        targetPaths,
        requestData.language,
        requestData.scope
      );

      // create chat stream
      const deepSeekLLM = Settings.llm as DeepSeekLLM;
      const chatStream = await deepSeekLLM.chat({
        messages: [{ role: "user", content: enhancedPrompt }],
        stream: true,
      });

      // create source info
      const sourceInfo = createSourceInfo(requestData, retrievedNodes, targetPaths);

      // handle streaming response
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
