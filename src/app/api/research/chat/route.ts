import { DeepSeekLLM } from '@llamaindex/deepseek';
import {
    MetadataMode,
    Settings,
} from 'llamaindex';
import { NextRequest, NextResponse } from "next/server";
import { config } from "../../../../config/environment";
import { documentIndexManager } from "../../../../lib/document-index-manager";

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
        const { query, documentPath } = await req.json();

        if (!query) {
            return NextResponse.json({ error: "Query is required" }, { status: 400 });
        }

        if (!documentPath) {
            return NextResponse.json({ error: "Document path is required" }, { status: 400 });
        }

        if (!config.deepSeekApiKey) {
            return NextResponse.json(
                { error: "DeepSeek API key is required" },
                { status: 500 }
            );
        }

        // configure DeepSeek LLM
        Settings.llm = new DeepSeekLLM({
            apiKey: config.deepSeekApiKey,
            model: config.deepSeekModel,
        });

        try {
            // get the pre-prepared index for this document
            const documentIndex = await documentIndexManager.getIndex(documentPath);

            if (!documentIndex) {
                return NextResponse.json({
                    error: "Document index not found",
                    message: "Please select the document again to prepare the index"
                }, { status: 400 });
            }

            console.log(`Using prepared index for ${documentPath}`);

            // create retriever and manually get context
            const retriever = documentIndex.asRetriever({ similarityTopK: 10 });

            // retrieve relevant context for the query
            const retrievedNodes = await retriever.retrieve({ query });
            const context = retrievedNodes.map((node: any) => node.node.getContent(MetadataMode.ALL)).join('\n\n');

            console.log(`Retrieved ${retrievedNodes.length} nodes for context`);

            // create enhanced prompt with retrieved context
            const enhancedPrompt = `${RESEARCH_SYSTEM_PROMPT}

Document: ${documentPath.split('/').pop()}

Retrieved Context:
${context}

Question: ${query}`;

            // use DeepSeek LLM directly for streaming
            const deepSeekLLM = Settings.llm as DeepSeekLLM;
            const chatStream = await deepSeekLLM.chat({
                messages: [{ role: 'user', content: enhancedPrompt }],
                stream: true
            });

            const readableStream = new ReadableStream({
                async start(controller) {
                    try {
                        // stream chat response chunks
                        for await (const chunk of chatStream) {
                            const content = chunk.delta;
                            if (content) {
                                controller.enqueue(content);
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
                if (processingError.message.includes("401") || processingError.message.includes("unauthorized")) {
                    errorMessage = "Invalid API key. Please check your DeepSeek API key.";
                } else if (processingError.message.includes("402") || processingError.message.includes("payment")) {
                    errorMessage = "Insufficient credits. Please check your DeepSeek account balance.";
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
        console.error("Error in chat API:", error);
        return NextResponse.json(
            { error: "Internal server error", details: error instanceof Error ? error.message : String(error) },
            { status: 500 }
        );
    }
} 