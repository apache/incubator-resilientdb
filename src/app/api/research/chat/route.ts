import { CodeComposerContext, generateCodeComposerPrompt, parseChainOfThoughtResponse } from "@/lib/code-composer-prompts";
import { CodeReranker } from "@/lib/code-reranker";
import { documentIndexManager } from "@/lib/document-index-manager";
import { DeepSeekLLM } from "@llamaindex/deepseek";
import { HuggingFaceEmbedding } from "@llamaindex/huggingface";
import { MetadataMode, Settings } from "llamaindex";
import { NextRequest, NextResponse } from "next/server";
import { config } from "../../../../config/environment";

// simple system prompt for document q&a
const RESEARCH_SYSTEM_PROMPT = `
You are ResAI, an AI research assistant specialized in Apache ResilientDB and its related blockchain technology, distributed systems, and fault-tolerant consensus protocols. Your primary role is to help students, researchers, and practitioners understand complex technical concepts related to Apache ResilientDB and blockchain systems, who can answer questions about documents. 
You have access to the content of a document and can provide accurate, detailed answers based on that content.
When asked about the document, always base your responses on the information provided in the document. When possible, cite sections, pages, or other specific information from the document.
If you cannot find specific information in the document, say so clearly.
Please favor referring to the document by its title, instead of the file name.
If asked about something that is not in the document, give a brief answer and try to guide the user to ask about something that is in the document.
`;

export async function POST(req: NextRequest) {
  try {
    const { query, documentPath, documentPaths, tool, language, scope } = await req.json();

    if (!query) {
      return NextResponse.json({ error: "Query is required" }, { status: 400 });
    }

    // Support both single document (backward compatibility) and multiple documents
    if (!documentPath && !documentPaths) {
      return NextResponse.json(
        {
          error: "Either documentPath or documentPaths is required",
        },
        { status: 400 },
      );
    }

    // Validate documentPaths if provided
    if (
      documentPaths &&
      (!Array.isArray(documentPaths) || documentPaths.length === 0)
    ) {
      return NextResponse.json(
        {
          error: "documentPaths must be a non-empty array",
        },
        { status: 400 },
      );
    }

    if (!config.deepSeekApiKey) {
      return NextResponse.json(
        { error: "DeepSeek API key is required" },
        { status: 500 },
      );
    }

    // configure DeepSeek LLM
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

    try {
      let documentIndex: any;
      let contextInfo: string;
      let retrievedNodes: any[];
      let enhancedPrompt: string;

      const targetPaths = documentPaths;

      // Check if all indices exist
      const hasAllIndices = documentIndexManager.hasAllIndices(targetPaths);
      if (!hasAllIndices) {
        return NextResponse.json(
          {
            error: "Some document indices not found",
            message: "Please prepare all selected documents again",
          },
          { status: 400 },
        );
      }

      // Get combined index for all documents
      documentIndex = await documentIndexManager.getCombinedIndex(targetPaths);

      if (!documentIndex) {
        return NextResponse.json(
          {
            error: "Failed to create combined index",
            message: "Please try selecting documents again",
          },
          { status: 400 },
        );
      }

      console.log(
        `Using combined index for ${targetPaths.length} documents: ${targetPaths.join(", ")}`,
      );

      // create retriever and get context from combined index
      const topK = tool === "code-composer" ? 20 : 15; // More context for code generation
      const retriever = documentIndex.asRetriever({ similarityTopK: topK });
      let initialNodes = await retriever.retrieve({ query });

      // Apply code reranking if in code-composer mode
      if (tool === "code-composer") {
        const codeReranker = new CodeReranker({
          maxTokens: 3000, // Token budget limit
          boostFactor: 1.5
        });
        retrievedNodes = codeReranker.rerank(initialNodes);
        
        console.log("Code reranker stats:", codeReranker.getStats(initialNodes, retrievedNodes));
      } else {
        retrievedNodes = initialNodes;
      }

      // Group context by source document for better organization
      const contextBySource: { [key: string]: string[] } = {};
      retrievedNodes.forEach((node: any) => {
        const sourceDoc = node.node.metadata?.source_document || "Unknown";
        if (!contextBySource[sourceDoc]) {
          contextBySource[sourceDoc] = [];
        }
        contextBySource[sourceDoc].push(node.node.getContent(MetadataMode.ALL));
      });

      // Format context with source attribution
      const contextParts = Object.entries(contextBySource).map(
        ([source, contents]) => {
          const fileName = source.split("/").pop() || source;
          return `**From ${fileName}:**\n${contents.join("\n\n")}`;
        },
      );

      const context = contextParts.join("\n\n---\n\n");
      contextInfo = `Documents: ${targetPaths.map((p: string) => p.split("/").pop()).join(", ")}`;

      console.log(
        `Retrieved ${retrievedNodes.length} nodes from ${targetPaths.length} documents`,
      );

      // create enhanced prompt based on tool mode
      if (tool === "code-composer") {
        const codeComposerContext: CodeComposerContext = {
          language: language || "ts",
          scope: scope || [],
          chunks: context,
          query,
          documentTitles: targetPaths.map((p: string) => p.split("/").pop()?.replace(".pdf", "") || p)
        };
        enhancedPrompt = generateCodeComposerPrompt(codeComposerContext);
      } else {
        // Standard research prompt
        enhancedPrompt = `${RESEARCH_SYSTEM_PROMPT}

Documents: ${targetPaths.map((p: string) => p.split("/").pop()).join(", ")}

Retrieved Context:
${context}

Question: ${query}`;
      }

      // use DeepSeek LLM directly for streaming
      const deepSeekLLM = Settings.llm as DeepSeekLLM;
      const chatStream = await deepSeekLLM.chat({
        messages: [{ role: "user", content: enhancedPrompt }],
        stream: true,
      });

      // Prepare source information for client
      const sourceInfo = {
        sources: (documentPaths || [documentPath]).map((path: string) => ({
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

      const readableStream = new ReadableStream({
        async start(controller) {
          try {
            // First, send source information as a special message
            controller.enqueue(
              `__SOURCE_INFO__${JSON.stringify(sourceInfo)}\n\n`,
            );

            // Handle different streaming based on tool mode
            if (tool === "code-composer") {
              // For code composer, collect full response first to parse chain-of-thought
              let fullResponse = "";
              for await (const chunk of chatStream) {
                const content = chunk.delta;
                if (content) {
                  fullResponse += content;
                  controller.enqueue(content); // Still stream for real-time feedback
                }
              }

              // Parse chain-of-thought structure
              const parsed = parseChainOfThoughtResponse(fullResponse);
              
              // Send structured metadata for UI processing
              controller.enqueue(
                `\n\n__CODE_COMPOSER_META__${JSON.stringify({
                  hasStructuredResponse: parsed.hasStructuredResponse,
                  planLength: parsed.plan.length,
                  pseudocodeLength: parsed.pseudocode.length,
                  implementationLength: parsed.implementation.length
                })}\n\n`
              );
            } else {
              // Standard streaming for research mode
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

      return new Response(readableStream, {
        headers: {
          "Content-Type": "text/plain; charset=utf-8",
        },
      });
    } catch (processingError) {
      console.error("Error processing chat:", processingError);

      // provide helpful error messages
      let errorMessage = "Failed to process your question";
      if (processingError instanceof Error) {
        if (
          processingError.message.includes("401") ||
          processingError.message.includes("unauthorized")
        ) {
          errorMessage = "Invalid API key. Please check your DeepSeek API key.";
        } else if (
          processingError.message.includes("402") ||
          processingError.message.includes("payment")
        ) {
          errorMessage =
            "Insufficient credits. Please check your DeepSeek account balance.";
        }
      }

      return NextResponse.json(
        {
          error: errorMessage,
          details:
            processingError instanceof Error
              ? processingError.message
              : String(processingError),
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
