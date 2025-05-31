import { NextRequest, NextResponse } from "next/server";
import { chatEngineService } from "../../../services/chatEngineService";

// Initialize the chat engine service on first import
let isServiceInitialized = false;

async function ensureServiceInitialized() {
    if (!isServiceInitialized) {
        try {
            await chatEngineService.initialize();
            isServiceInitialized = true;
        } catch (error) {
            console.error("Failed to initialize chat engine service:", error);
            throw error;
        }
    }
}

export async function POST(req: NextRequest) {
    try {
        const { query } = await req.json();

        if (!query) {
            return NextResponse.json({ error: "Query is required" }, { status: 400 });
        }

        // Ensure the service is initialized
        await ensureServiceInitialized();

        // Check service status
        const status = chatEngineService.getStatus();
        if (!status.isInitialized) {
            return NextResponse.json(
                { error: "Chat engine service is not initialized", details: status.error },
                { status: 500 }
            );
        }

        // Send message with streaming support
        const chatResponse = await chatEngineService.sendMessageWithStream(query);

        return new Response(chatResponse.stream, {
            headers: {
                "Content-Type": "text/plain; charset=utf-8",
            },
        });

    } catch (error) {
        console.error("[Chat Engine Service Error]", error);
        const errorMessage = error instanceof Error ? error.message : "An unknown error occurred";
        return NextResponse.json(
            { error: "Error processing your request", details: errorMessage },
            { status: 500 }
        );
    }
}

// Optional: Add a GET handler to check service status
export async function GET() {
    try {
        await ensureServiceInitialized();
        const status = chatEngineService.getStatus();
        return NextResponse.json({
            message: "Chat Engine Service with DeepSeek and LlamaCloud is running",
            status: status
        });
    } catch (error) {
        const errorMessage = error instanceof Error ? error.message : "Service initialization failed";
        return NextResponse.json(
            { error: "Service not available", details: errorMessage },
            { status: 500 }
        );
    }
} 