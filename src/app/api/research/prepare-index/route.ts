import { NextRequest, NextResponse } from "next/server";
import { config } from "../../../../config/environment";
import { documentIndexManager } from "../../../../lib/document-index-manager";

export async function POST(req: NextRequest) {
    try {
        const { documentPath, documentPaths } = await req.json();

        // Support both single document (backward compatibility) and multiple documents
        if (!documentPath && !documentPaths) {
            return NextResponse.json({ 
                error: "Either documentPath or documentPaths is required" 
            }, { status: 400 });
        }

        // Validate documentPaths if provided
        if (documentPaths && (!Array.isArray(documentPaths) || documentPaths.length === 0)) {
            return NextResponse.json({ 
                error: "documentPaths must be a non-empty array" 
            }, { status: 400 });
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
            // Handle both single and multiple document preparation
            if (documentPaths) {
                // Multi-document mode
                await documentIndexManager.prepareMultipleIndices(documentPaths);
                
                return NextResponse.json({
                    success: true,
                    message: `Indices prepared successfully for ${documentPaths.length} documents`,
                    documentCount: documentPaths.length,
                    documentPaths: documentPaths
                });
            } else {
                // Single document mode (backward compatibility)
                await documentIndexManager.prepareIndex(documentPath);

                return NextResponse.json({
                    success: true,
                    message: "Index prepared successfully",
                    documentCount: 1,
                    documentPaths: [documentPath]
                });
            }

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
