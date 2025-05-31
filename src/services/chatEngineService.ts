import { DeepSeekLLM } from '@llamaindex/deepseek';
import {
    ContextChatEngine,
    LlamaCloudRetriever,
    MetadataMode,
    NodeWithScore,
    Settings,
} from 'llamaindex';
import { config } from '../config/environment';

// Define the system prompt for the RAG assistant
const RAG_SYSTEM_PROMPT = `
You are ResAI, an AI research assistant specialized in Apache ResilientDB and its related blockchain technology, distributed systems, and fault-tolerant consensus protocols. Your primary role is to help students, researchers, and practitioners understand complex technical concepts related to Apache ResilientDB and blockchain systems.

## Core Expertise Areas

**Blockchain and Distributed Systems**: You have deep knowledge of blockchain architectures, consensus protocols, distributed computing, and fault-tolerant systems. You understand the evolution from traditional blockchain limitations to modern high-throughput solutions.

**Apache ResilientDB Specialization**: You are well-versed in Apache ResilientDB's scale-centric design philosophy, including its multi-threaded architecture, parallelism implementation, deep pipelining strategies, and optimization for modern hardware and global cloud infrastructure.

**Technical Components**: You can explain and help with GraphQL integration, wallet systems, SDK development, in-memory processing, durable storage solutions, RPC architectures, containerization with Docker, and secure authentication mechanisms.

## Response Guidelines

**Academic Rigor**: Provide technically accurate, well-structured explanations suitable for academic research. Support your responses with relevant technical details and context about how concepts relate to broader distributed systems principles.

**Multi-Level Explanations**: Adapt your explanations to the user's apparent expertise level. Provide foundational concepts for beginners while offering deep technical insights for advanced researchers.

**Research-Oriented**: When discussing concepts, include relevant research directions, potential applications, performance considerations, and connections to current academic literature in blockchain and distributed systems.

**Practical Implementation**: Balance theoretical knowledge with practical implementation guidance, including code examples, architectural decisions, and real-world deployment considerations.

## Key Focus Areas

- High-throughput blockchain design and performance optimization
- Fault-tolerant consensus mechanisms and their trade-offs
- Modern hardware utilization in blockchain systems
- Privacy, integrity, transparency, and accountability in decentralized systems
- Scalability challenges and solutions in blockchain networks
- Integration patterns for blockchain applications

Always encourage critical thinking about the democratic and decentralized computational paradigm that blockchain technology enables, while maintaining technical precision in your explanations.
`;

export interface SourceNode {
    id: string;
    text: string;
    score: number;
}

export interface ChatResponse {
    stream: ReadableStream;
    sourceNodes: SourceNode[];
}

class ChatEngineService {
    private chatEngine: ContextChatEngine | null = null;
    private retriever: LlamaCloudRetriever | null = null;
    private isInitialized = false;
    private initializationError: string | null = null;

    async initialize(): Promise<void> {
        try {
            this.initializationError = null;

            if (!config.deepSeekApiKey) {
                throw new Error('DeepSeek API key is required. Please set DEEPSEEK_API_KEY in your environment variables.');
            }

            // Configure DeepSeek LLM
            Settings.llm = new DeepSeekLLM({
                apiKey: config.deepSeekApiKey,
                model: config.deepSeekModel,
            });

            if (config.llamaCloudApiKey && config.llamaCloudProjectName) {
                try {
                    const retrieverOptions: any = {
                        projectName: config.llamaCloudProjectName,
                        apiKey: config.llamaCloudApiKey,
                    };

                    if (config.llamaCloudIndexName) {
                        retrieverOptions.name = config.llamaCloudIndexName;
                    }

                    if (config.llamaCloudBaseUrl) {
                        retrieverOptions.baseUrl = config.llamaCloudBaseUrl;
                    }

                    // Initialize the retriever
                    this.retriever = new LlamaCloudRetriever(retrieverOptions);

                    // Create chat engine with system prompt
                    this.chatEngine = new ContextChatEngine({
                        retriever: this.retriever,
                        systemPrompt: RAG_SYSTEM_PROMPT,
                    });

                    console.log('Chat engine with LlamaCloud initialized successfully');
                } catch (llamaError) {
                    console.warn('LlamaCloud initialization failed, falling back to DeepSeek only:', llamaError);
                    this.retriever = null;
                    this.chatEngine = null;
                }
            } else {
                console.warn('LlamaCloud configuration incomplete, using DeepSeek only');
            }

            this.isInitialized = true;
            console.log('Chat engine service initialized successfully');
        } catch (err) {
            const errorMessage = err instanceof Error ? err.message : 'Failed to initialize chat engine';
            this.initializationError = errorMessage;
            console.error('Chat engine initialization error:', err);
            throw err;
        }
    }

    async sendMessageWithStream(message: string): Promise<ChatResponse> {
        if (!this.isInitialized) {
            throw new Error('Chat engine is not initialized');
        }

        try {
            let sourceNodes: SourceNode[] = [];

            if (this.chatEngine && this.retriever) {
                // 1. Retrieve source nodes using the retriever instance
                const nodesWithScore: NodeWithScore[] = await this.retriever.retrieve(message);
                sourceNodes = nodesWithScore
                    .filter((n) => n.score && n.score > 0.59) // Filter nodes by score
                    .slice(0, 3) // Limit to maximum 3 sources
                    .map((n) => ({
                        id: n.node.id_,
                        text: n.node.getContent(MetadataMode.NONE), // Send full text
                        score: n.score || 0, // Ensure score is always a number
                    }));

                // 2. Stream the chat response
                const contextualPrompt = `${RAG_SYSTEM_PROMPT}

User question: ${message}`;

                const chatStream = await this.chatEngine.chat({
                    message: contextualPrompt,
                    stream: true,
                });

                const readableStream = new ReadableStream({
                    async start(controller) {
                        try {
                            // Stream text chunks
                            for await (const chunk of chatStream) {
                                controller.enqueue(chunk.response);
                            }
                            // After text stream, enqueue the source nodes marker and then the JSON
                            if (sourceNodes.length > 0) {
                                controller.enqueue("\n\n__SOURCE_NODES_SEPARATOR__\n\n");
                                controller.enqueue(JSON.stringify(sourceNodes));
                            }
                            controller.close();
                        } catch (error) {
                            controller.error(error);
                        }
                    },
                });

                return { stream: readableStream, sourceNodes };
            } else {
                // Fallback to direct DeepSeek LLM usage
                const deepSeekLLM = Settings.llm as DeepSeekLLM;
                if (deepSeekLLM) {
                    const contextualPrompt = `${RAG_SYSTEM_PROMPT}

User question: ${message}`;

                    const response = await deepSeekLLM.complete({ prompt: contextualPrompt });
                    const responseText = response.text || 'I apologize, but I could not generate a response.';

                    const readableStream = new ReadableStream({
                        start(controller) {
                            controller.enqueue(responseText);
                            controller.close();
                        },
                    });

                    return { stream: readableStream, sourceNodes: [] };
                } else {
                    throw new Error('No chat engine or LLM available');
                }
            }
        } catch (err) {
            console.error('Error sending message:', err);
            throw new Error('Failed to send message. Please try again.');
        }
    }

    async sendMessage(message: string): Promise<string> {
        if (!this.isInitialized) {
            throw new Error('Chat engine is not initialized');
        }

        try {
            if (this.chatEngine) {
                // Use chat method with system prompt already configured
                const response = await this.chatEngine.chat({ message });
                return response.response || 'I apologize, but I could not generate a response.';
            } else {
                // Fallback to direct DeepSeek LLM usage
                const deepSeekLLM = Settings.llm as DeepSeekLLM;
                if (deepSeekLLM) {
                    const contextualPrompt = `${RAG_SYSTEM_PROMPT}

User question: ${message}`;

                    const response = await deepSeekLLM.complete({ prompt: contextualPrompt });
                    return response.text || 'I apologize, but I could not generate a response.';
                } else {
                    throw new Error('No chat engine available');
                }
            }
        } catch (err) {
            console.error('Error sending message:', err);
            throw new Error('Failed to send message. Please try again.');
        }
    }

    getStatus() {
        return {
            isInitialized: this.isInitialized,
            error: this.initializationError,
            hasLlamaCloud: !!this.chatEngine,
            hasRetriever: !!this.retriever,
        };
    }
}

export const chatEngineService = new ChatEngineService(); 