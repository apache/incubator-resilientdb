import { NextRequest, NextResponse } from "next/server";
import { config } from "../../../../config/environment";
import { documentIndexManager } from "../../../../lib/document-index-manager";

export async function POST(req: NextRequest) {
    try {
        const { documentPath } = await req.json();

        if (!documentPath) {
            return NextResponse.json({ error: "Document path is required" }, { status: 400 });
        }

        if (!config.deepSeekApiKey) {
            return NextResponse.json(
                { error: "DeepSeek API key is required" },
                { status: 500 }
            );
        }

        if (!config.llamaCloudApiKey) {
            return NextResponse.json(
                { error: "LlamaCloud API key is required" },
                { status: 500 }
            );
        }

        try {
            await documentIndexManager.prepareIndex(documentPath);

            return NextResponse.json({
                success: true,
                message: "Index prepared successfully"
            });

        } catch (processingError) {
            console.error("Error preparing index:", processingError);

            // provide helpful error messages
            let errorMessage = "Failed to prepare document index";
            if (processingError instanceof Error) {
                if (processingError.message.includes("401") || processingError.message.includes("unauthorized")) {
                    errorMessage = "Invalid API key. Please check your LlamaCloud or DeepSeek API keys.";
                } else if (processingError.message.includes("402") || processingError.message.includes("payment")) {
                    errorMessage = "Insufficient credits. Please check your LlamaCloud account balance.";
                }
            }

            return NextResponse.json(
                {
                    error: errorMessage,
                    details: processingError instanceof Error ? processingError.message : String(processingError)
                },
                { status: 500 }
            );
        }

    } catch (error) {
        console.error("Error in prepare-index API:", error);
        return NextResponse.json(
            { error: "Internal server error", details: error instanceof Error ? error.message : String(error) },
            { status: 500 }
        );
    }
}
