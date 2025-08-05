import { llamaService } from "@/lib/llama-service";
import { sessionManager } from "@/lib/session-manager";
import { NextRequest, NextResponse } from "next/server";
import { config } from "../../../../config/environment";

interface RequestData {
  query: string;
  documentPath?: string;
  documentPaths?: string[];
  tool?: string;
  language?: string;
  scope?: string[];
  sessionId?: string; 
}

const validateRequest = (data: RequestData): string | null => {
  const { query, documentPath, documentPaths } = data;

  if (!query) {
    return "Query is required";
  }

  if (!documentPath && !documentPaths) {
    return "Either documentPath or documentPaths is required";
  }

  if (
    documentPaths &&
    (!Array.isArray(documentPaths) || documentPaths.length === 0)
  ) {
    return "documentPaths must be a non-empty array";
  }

  if (!config.deepSeekApiKey) {
    return "DeepSeek API key is required";
  }

  return null;
};

const handleStreamingResponse = async (
  chatEngine: any,
  query: string,
  sessionId: string,
): Promise<ReadableStream> => {
  return new ReadableStream({
    async start(controller) {
      try {
        const stream = await chatEngine.chat({
          message: query,
          stream: true,
        });

        const responseChunks: string[] = [];
        let lastChunk;
        let sourceMetadata: any[] = [];

        for await (const chunk of stream) {
          lastChunk = chunk;
          const content = chunk.response || chunk.delta || "";
          if (content) {
            controller.enqueue(content);
            responseChunks.push(content);
          }
        }
        const memory = sessionManager.getSessionMemory(sessionId);
        await memory.add({ role: "assistant", content: responseChunks.join('') });
        
        const messages = await memory.get();
        console.log('Updated memory:', messages);
        
        if (lastChunk?.sourceNodes) {
          for (const sourceNode of lastChunk.sourceNodes) {
            sourceMetadata.push(sourceNode.node.metadata);
          }
        }
        controller.enqueue(
          `__SOURCE_INFO__${JSON.stringify(sourceMetadata)}\n\n`,
        );
        
        controller.close();
      } catch (error) {
        console.error("Streaming error:", error);
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

    try {
      const documentPaths = requestData.documentPaths || [
        requestData.documentPath!,
      ];

      console.log(
        `Processing query: "${requestData.query}" for documents: ${documentPaths.join(", ")}`,
      );
      
      const sessionId = requestData.sessionId || Date.now().toString();
      const memory = sessionManager.getSessionMemory(sessionId);
      
      await memory.add({ role: "user", content: requestData.query });
      
      const chatHistory = await memory.get();

      const chatEngine = await llamaService.createChatEngine(documentPaths, chatHistory);

      const stream = await handleStreamingResponse(
        chatEngine,
        requestData.query,
        sessionId,
      );

      return new Response(stream, {
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