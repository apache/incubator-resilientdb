import { DeepSeekLLM } from "@llamaindex/deepseek"; // Import DeepSeekLLM and its model type
import {
    ContextChatEngine,
    LlamaCloudRetriever, // Added import
    MetadataMode,
    NodeWithScore,
    // OpenAI, // No longer using OpenAI directly here for Settings.llm
    Settings,
} from "llamaindex";
import { NextRequest, NextResponse } from "next/server";

// Configure the LLM
const deepSeekModel = (process.env.MODEL || "deepseek-chat") as "deepseek-chat" | "deepseek-coder" | undefined;

Settings.llm = new DeepSeekLLM({
    apiKey: process.env.DEEPSEEK_API_KEY || "",
    model: deepSeekModel,
});


export async function POST(req: NextRequest) {
    try {
        const { query } = await req.json();

        if (!query) {
            return NextResponse.json({ error: "Query is required" }, { status: 400 });
        }

        if (!process.env.LLAMA_CLOUD_API_KEY) {
            return NextResponse.json(
                { error: "LlamaCloud API key not configured" },
                { status: 500 }
            );
        }

        if (!process.env.LLAMA_CLOUD_PROJECT_NAME) {
            return NextResponse.json(
                { error: "LlamaCloud project name not configured" },
                { status: 500 }
            );
        }

        const retrieverOptions: any = {
            projectName: process.env.LLAMA_CLOUD_PROJECT_NAME as string,
            apiKey: process.env.LLAMA_CLOUD_API_KEY as string,
        };

        if (process.env.LLAMA_CLOUD_INDEX_NAME) {
            retrieverOptions.name = process.env.LLAMA_CLOUD_INDEX_NAME;
        } else {

            console.log("LLAMA_CLOUD_PIPELINE_NAME not set, attempting to use default pipeline or indexName if provided.");
        }

        // Initialize the retriever that will be used by the chat engine
        const retriever = new LlamaCloudRetriever(retrieverOptions);

        const chatEngine = new ContextChatEngine({
            retriever,
            systemPrompt: "Your name is Abdel.",
        });

        // 1. Retrieve source nodes using the same retriever instance
        const nodesWithScore: NodeWithScore[] = await retriever.retrieve(query);
        const sourceNodesData = nodesWithScore
            .filter((n) => n.score && n.score > 0.5) // Filter nodes by score
            .map((n) => ({
                id: n.node.id_,
                text: n.node.getContent(MetadataMode.NONE), // Send full text
                score: n.score,
            }));

        // 2. Stream the chat response
        const chatStream = await chatEngine.chat({
            message: query,
            stream: true,
        });

        const readableStream = new ReadableStream({
            async start(controller) {
                // Stream text chunks
                for await (const chunk of chatStream) {
                    controller.enqueue(chunk.response);
                }
                // After text stream, enqueue the source nodes marker and then the JSON
                if (sourceNodesData.length > 0) {
                    controller.enqueue("\n\n__SOURCE_NODES_SEPARATOR__\n\n");
                    controller.enqueue(JSON.stringify(sourceNodesData));
                }
                controller.close();
            },
        });

        return new Response(readableStream, {
            headers: {
                "Content-Type": "text/plain; charset=utf-8",
            },
        });

    } catch (error) {
        console.error("[LlamaIndex API Error]", error);
        const errorMessage = error instanceof Error ? error.message : "An unknown error occurred";
        return NextResponse.json(
            { error: "Error processing your request", details: errorMessage },
            { status: 500 }
        );
    }
}

// Optional: Add a GET handler or other methods if needed
export async function GET() {
    return NextResponse.json({ message: "LlamaIndex RAG API is running with DeepSeek and sources" });
} 