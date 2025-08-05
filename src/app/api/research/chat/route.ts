import { llamaService } from "@/lib/llama-service";
import { NextRequest, NextResponse } from "next/server";
import { config } from "../../../../config/environment";

interface RequestData {
  query: string;
  documentPath?: string;
  documentPaths?: string[];
  tool?: string;
  language?: string;
  scope?: string[];
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
): Promise<ReadableStream> => {
  return new ReadableStream({
    async start(controller) {
      try {
        // const sourceNodes = await retriever.retrieve(query);
        // const citationLines = sourceNodes.map((node: any) => `[^${node.id}]" (Page ${node.metadata.page} of ${node.metadata.source_document})`).join("\n");

        const stream = await chatEngine.chat({
          message: query,
          stream: true,
        });

        let lastChunk;
        const sourceMetadata: any[] = [];

        for await (const chunk of stream) {
          lastChunk = chunk;
          const content = chunk.response || chunk.delta || "";
          if (content) {
            controller.enqueue(content);
          }
        }

        if (lastChunk?.sourceNodes) {
          for (const sourceNode of lastChunk.sourceNodes) {
            sourceMetadata.push(sourceNode.node.metadata);
          }
        }
        controller.enqueue(
          `__SOURCE_INFO__${JSON.stringify(sourceMetadata)}\n\n`,
        );
        console.log(sourceMetadata);
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

      const chatEngine = await llamaService.createChatEngine(documentPaths);

      const stream = await handleStreamingResponse(
        chatEngine,
        requestData.query,
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