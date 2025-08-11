import { CodeAgent, NexusAgent } from "@/lib/agent";
import { agentStreamEvent, agentToolCallEvent, agentToolCallResultEvent, AgentWorkflow } from "@llamaindex/workflow";
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



const handleAgentStreamingResponse = async (
  agentWorkflow: AgentWorkflow,
  query: string,
  sessionId: string,
): Promise<ReadableStream> => {
  return new ReadableStream({
    async start(controller) {
      try {
        try {
          const beforeMemory = NexusAgent.getInstance().getMemory(sessionId);
          if (beforeMemory) {
            console.log("[Memory][Before]", await beforeMemory.get());
          }
        } catch {}
        const stream = await agentWorkflow.runStream(query);

        let completeResponse = "";

        for await (const event of stream) {
          if (agentToolCallEvent.include(event)) {
            const { toolId, toolName, toolKwargs } = event.data as any;
            const callPart = {
              id: toolId,
              type: toolName,
              state: "input-available" as const,
              input: toolKwargs,
            };
            controller.enqueue(`__TOOL_CALL__${JSON.stringify(callPart)}\n\n`);
          }
          if (agentToolCallResultEvent.include(event)) {
            const { toolId, toolName, toolOutput } = event.data as any;
            const state = toolOutput && toolOutput.error ? "output-error" : "output-available";
            const updatePart = {
              id: toolId,
              type: toolName,
              state,
            };
            // Do not include output or errorText per requirement
            controller.enqueue(`__TOOL_CALL__${JSON.stringify(updatePart)}\n\n \n\n \n\n`);
          }
          if (agentStreamEvent.include(event)) {
            const content = event.data.delta || "";
            if (content) {
              controller.enqueue(content);
              completeResponse += content;
            }
          }
        }

        try {
          const afterMemory = NexusAgent.getInstance().getMemory(sessionId);
          if (afterMemory) {
            console.log("[Memory][After]", await afterMemory.getLLM());
          }
        } catch {}

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

    try {
      const documentPaths = requestData.documentPaths || [
        requestData.documentPath!,
      ];

      console.log(
        `Processing query: "${requestData.query}" for documents: ${documentPaths.join(", ")}`,
      );

      // Selected language passed to code agent
      const selectedLanguage = requestData.language || "ts";
      
      const sessionId = requestData.sessionId || Date.now().toString();
      
      // Create or reuse agent for session
      const isCodeComposer = requestData.tool === "code-composer";
      let agentWorkflow: AgentWorkflow;

      if (isCodeComposer) {
        // Use the simple CodeAgent placeholder that always returns a fixed message
        const codeAgentFactory = new CodeAgent(selectedLanguage);
        agentWorkflow = await codeAgentFactory.createAgent(documentPaths, sessionId);
      } else {
        const nexusAgent = await NexusAgent.create();
        agentWorkflow = await nexusAgent.createAgent(documentPaths, sessionId);
      }

      const stream = await handleAgentStreamingResponse(
        agentWorkflow,
        requestData.query,
        sessionId,
      );

      return new Response(stream, {
        headers: {
          "Content-Type": "text/plain; charset=utf-8",
          "X-Session-Id": sessionId,
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