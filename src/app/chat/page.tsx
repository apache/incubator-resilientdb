"use client";

import { AnimatedAIChat } from "@/components/ui/animated-ai-chat";
import { Loader } from "@/components/ui/loader";
import { MarkdownRenderer } from "@/components/ui/markdown-renderer";
import { useEffect, useRef, useState } from "react";

interface SourceNode {
  id: string;
  text: string;
  score?: number;
  // metadata?: any; // Optionally include if you send it from the backend
}

interface Message {
  id: string;
  sender: "user" | "ai";
  content: string;
  isStreaming?: boolean;
  sources?: SourceNode[];
}

export default function Home() {
  const [messages, setMessages] = useState<Message[]>([]);
  const [isLoading, setIsLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [selectedSource, setSelectedSource] = useState<SourceNode | null>(null);
  const [isSidePanelOpen, setIsSidePanelOpen] = useState(false);

  const messagesEndRef = useRef<null | HTMLDivElement>(null);

  const scrollToBottom = () => {
    messagesEndRef.current?.scrollIntoView({ behavior: "smooth" });
  };

  useEffect(() => {
    scrollToBottom();
  }, [messages]);

  const handleSubmit = async (query: string) => {
    if (!query.trim()) return;

    const userMessage: Message = {
      id: Date.now().toString() + "-user",
      sender: "user",
      content: query,
    };
    setMessages((prevMessages) => [...prevMessages, userMessage]);
    setIsLoading(true);
    setError(null);

    const aiMessageId = Date.now().toString() + "-ai";
    setMessages((prevMessages) => [
      ...prevMessages,
      {
        id: aiMessageId,
        sender: "ai",
        content: "",
        isStreaming: true,
      },
    ]);

    try {
      const res = await fetch("/api/chat", {
        method: "POST",
        headers: {
          "Content-Type": "application/json",
        },
        body: JSON.stringify({ query }),
      });

      if (!res.ok) {
        const errorData = await res.json();
        const errorMessage = errorData.error || `Error: ${res.status}`;
        setMessages((prevMessages) =>
          prevMessages.map((msg) =>
            msg.id === aiMessageId
              ? { ...msg, content: `Error: ${errorMessage}`, isStreaming: false }
              : msg
          )
        );
        setError(errorMessage);
        throw new Error(errorMessage);
      }

      if (!res.body) {
        const errorMessage = "Response body is null";
        setMessages((prevMessages) =>
          prevMessages.map((msg) =>
            msg.id === aiMessageId
              ? { ...msg, content: `Error: ${errorMessage}`, isStreaming: false }
              : msg
          )
        );
        setError(errorMessage);
        throw new Error(errorMessage);
      }

      const reader = res.body.getReader();
      const decoder = new TextDecoder();
      let rawResponseData = "";
      const separator = "\n\n__SOURCE_NODES_SEPARATOR__\n\n";

      while (true) {
        const { value, done: readerDone } = await reader.read();
        if (readerDone) break;
        
        rawResponseData += decoder.decode(value, { stream: true }); // Continue decoding as stream
        
        // Update UI with text part as it streams
        const separatorIndex = rawResponseData.indexOf(separator);
        const currentTextContent = separatorIndex !== -1 ? rawResponseData.substring(0, separatorIndex) : rawResponseData;
        
        setMessages((prevMessages) =>
          prevMessages.map((msg) =>
            msg.id === aiMessageId
              ? { ...msg, content: currentTextContent, isStreaming: true }
              : msg
          )
        );
      }
      // The stream is complete, TextDecoder's internal buffer is flushed by not passing stream: true in last call implicitly
      // or if the stream naturally ends.

      // Process the full rawResponseData after stream completion
      const parts = rawResponseData.split(separator);
      const finalContent = parts[0];
      let finalSources: SourceNode[] = [];

      if (parts.length > 1 && parts[1]) {
        try {
          finalSources = JSON.parse(parts[1]);
        } catch (e) {
          console.error("Failed to parse source nodes JSON:", e, "JSON string:", parts[1]);
          // Optionally, set a user-facing error or a default source indicating parsing failure
        }
      }
      
      setMessages((prevMessages) =>
        prevMessages.map((msg) =>
          msg.id === aiMessageId
            ? { ...msg, content: finalContent, sources: finalSources, isStreaming: false }
            : msg
        )
      );

    } catch (err) {
      console.error(err);
      const finalErrorMessage = err instanceof Error ? err.message : "An unexpected error occurred.";
      setMessages((prevMessages) =>
        prevMessages.map((msg) =>
          msg.id === aiMessageId && msg.content === ""
            ? { ...msg, content: `Error: ${finalErrorMessage}`, isStreaming: false }
            : msg
        )
      );
      if (!error) setError(finalErrorMessage);
    } finally {
      setIsLoading(false);
    }
  };

  return (
    <div className="flex flex-col text-white font-sans min-h-screen">
      <main className=" overflow-y-auto py-6 flex justify-center items-center mt-[60px] flex-grow">
        <div className="w-full max-w-5xl px-4 space-y-4">
          <div className={`rounded-xl border border-white/10 h-[65vh] overflow-y-auto ${messages.length > 0 ? 'bg-black/20 backdrop-blur-sm' : 'hidden'} p-6 space-y-4`}>
            {messages.map((msg) => (
              <div
                key={msg.id}
                className={`flex ${msg.sender === "user" ? "justify-end" : "justify-start"}`}
              >
                <div
                  className={`max-w-xl lg:max-w-2xl px-4 py-3 rounded-xl shadow-md 
                    ${msg.sender === "user"
                      ? "bg-muted text-white rounded-br-none"
                      : " text-gray-200 rounded-bl-none"}`}
                >
                  {msg.sender === "ai" ? (
                    <>
                      {msg.content ? (
                        <MarkdownRenderer content={msg.content} />
                      ) : msg.isStreaming ? (
                        <Loader variant="loading-dots" text="Thinking" size="sm" className="text-gray-300" />
                      ) : null}
                    </>
                  ) : (
                    <div className="whitespace-pre-wrap break-words">{msg.content}</div>
                  )}
                  {msg.isStreaming && msg.sender === "ai" && msg.content && (
                    <span className="inline-block w-1.5 h-1.5 ml-1 bg-gray-400 rounded-full animate-pulse"></span>
                  )}
                  {/* Display Source Nodes */}
                  {msg.sender === "ai" && msg.sources && msg.sources.length > 0 && (
                    <div className="mt-3 pt-3 border-t border-gray-600">
                      <h4 className="text-xs font-semibold text-gray-400 mb-2">Sources:</h4>
                      <ul className="space-y-1.5">
                        {msg.sources.map((source, index) => (
                          <li 
                            key={source.id || `source-${index}`}
                            className="p-2 bg-gray-600 bg-opacity-50 rounded-md text-xs text-gray-300 hover:bg-gray-500 hover:bg-opacity-50 transition-colors cursor-pointer"
                            title={source.text}
                            onClick={() => {
                              setSelectedSource(source);
                              setIsSidePanelOpen(true);
                            }}
                          >
                            <p className="font-medium text-gray-200 mb-0.5 truncate">
                              Source {index + 1} (Score: {source.score !== undefined ? source.score.toFixed(4) : 'N/A'})
                            </p>
                            <p className="italic truncate	">{source.text}</p>
                          </li>
                        ))}
                      </ul>
                    </div>
                  )}
                </div>
              </div>
            ))}
            <div ref={messagesEndRef} />
          </div>
        </div>
      </main>

      {isSidePanelOpen && selectedSource && (
        <div className="fixed top-0 right-0 h-full w-1/3  shadow-xl z-100 p-6 overflow-y-auto text-white flex flex-col">
          <div className="flex justify-between items-center mb-4">
            <h3 className="text-lg font-semibold">Source Document</h3>
            <button 
              onClick={() => {
                setIsSidePanelOpen(false);
                setSelectedSource(null);
              }}
              className="text-gray-400 hover:text-white transition-colors"
            >
              <svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" strokeWidth={1.5} stroke="currentColor" className="w-6 h-6">
                <path strokeLinecap="round" strokeLinejoin="round" d="M6 18L18 6M6 6l12 12" />
              </svg>
            </button>
          </div>
          <div className="prose prose-invert max-w-none flex-grow">
            <p className="text-sm text-gray-400 mb-1">ID: {selectedSource.id}</p>
            {selectedSource.score !== undefined && (
                <p className="text-sm text-gray-400 mb-3">Score: {selectedSource.score.toFixed(4)}</p>
            )}
            <MarkdownRenderer content={selectedSource.text} className="text-sm" />
          </div>
        </div>
      )}

      {/* Replace with AnimatedAIChat component */}
      <footer className="sticky bottom-0 z-10">
        {error && (
          <div className="mb-3 p-3 text-sm text-red-300 bg-red-700 bg-opacity-50 border border-red-600 rounded-lg">
            <p>{error}</p>
          </div>
        )}
        <div className="w-full flex justify-center items-center">
          <div className="w-full max-w-3xl px-4 pb-6">
            <AnimatedAIChat onSubmit={handleSubmit} compact={messages.length > 0} />
          </div>
        </div>
      </footer>
    </div>
  );
}
