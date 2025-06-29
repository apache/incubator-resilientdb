"use client";

import { Button } from "@/components/ui/button";
import { Loader } from "@/components/ui/loader";
import { MarkdownRenderer } from "@/components/ui/markdown-renderer";
import { useCallback, useEffect, useRef, useState } from "react";

interface Document {
  id: string;
  name: string;
  path: string;
  size: number;
  uploadedAt: string;
  displayTitle?: string;
}

interface Message {
  id: string;
  content: string;
  role: "user" | "assistant";
  timestamp: string;
  isLoadingPlaceholder?: boolean;
}

// Filename to title mapping - you can extend this as needed
const getDisplayTitle = (filename: string): string => {
  const titleMappings: Record<string, string> = {
    "resilientdb.pdf": "ResilientDB: Global Scale Resilient Blockchain Fabric",
    "rcc.pdf": "Resilient Concurrent Consensus for High-Throughput Secure Transaction Processing",
  };
  
  const lowerFilename = filename.toLowerCase();
  return titleMappings[lowerFilename] || filename.replace('.pdf', '');
};

export default function ResearchChatPage() {
  const [documents, setDocuments] = useState<Document[]>([]);
  const [selectedDocument, setSelectedDocument] = useState<Document | null>(null);
  const [messages, setMessages] = useState<Message[]>([]);
  const [inputValue, setInputValue] = useState("");
  const [isLoading, setIsLoading] = useState(false);
  const [isLoadingDocuments, setIsLoadingDocuments] = useState(true);
  const [isPreparingIndex, setIsPreparingIndex] = useState(false);
  const [isSidebarCollapsed, setIsSidebarCollapsed] = useState(false);
  
  const messagesEndRef = useRef<HTMLDivElement>(null);

  // Scroll to bottom when new messages arrive
  const scrollToBottom = useCallback(() => {
    messagesEndRef.current?.scrollIntoView({ behavior: "smooth" });
  }, []);

  useEffect(() => {
    scrollToBottom();
  }, [messages, scrollToBottom]);

  // Load available documents
  useEffect(() => {
    const loadDocuments = async () => {
      try {
        const response = await fetch("/api/research/documents");
        if (response.ok) {
          const docs = await response.json();
          // Add display titles to documents
          const docsWithTitles = docs.map((doc: Document) => ({
            ...doc,
            displayTitle: getDisplayTitle(doc.name)
          }));
          setDocuments(docsWithTitles);
        }
      } catch (error) {
        console.error("Failed to load documents:", error);
      } finally {
        setIsLoadingDocuments(false);
      }
    };

    loadDocuments();
  }, []);

  // Prepare index when document changes
  useEffect(() => {
    const prepareDocumentIndex = async () => {
      if (selectedDocument) {
        setIsPreparingIndex(true);
        setMessages([
          {
            id: Date.now().toString(),
            content: `📄 **${selectedDocument.displayTitle || selectedDocument.name}** has been selected. Preparing document for questions...`,
            role: "assistant",
            timestamp: new Date().toISOString(),
          },
        ]);

        try {
          const response = await fetch("/api/research/prepare-index", {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({ documentPath: selectedDocument.path }),
          });

          if (response.ok) {
            setMessages([
              {
                id: Date.now().toString(),
                content: `✅ **${selectedDocument.displayTitle || selectedDocument.name}** is ready! You can now ask questions about this document.`,
                role: "assistant",
                timestamp: new Date().toISOString(),
              },
            ]);
          } else {
            const error = await response.json();
            setMessages([
              {
                id: Date.now().toString(),
                content: ` Failed to prepare document: ${error.error || "Unknown error"}`,
                role: "assistant",
                timestamp: new Date().toISOString(),
              },
            ]);
          }
        } catch (error) {
          console.error("Error preparing document:", error);
          setMessages([
            {
              id: Date.now().toString(),
              content: ` Error preparing document. Please try selecting it again.`,
              role: "assistant",
              timestamp: new Date().toISOString(),
            },
          ]);
        } finally {
          setIsPreparingIndex(false);
        }
      }
    };

    prepareDocumentIndex();
  }, [selectedDocument]);

  const handleSendMessage = async () => {
    if (!inputValue.trim() || !selectedDocument || isLoading || isPreparingIndex) return;

    const userMessage: Message = {
      id: Date.now().toString(),
      content: inputValue,
      role: "user",
      timestamp: new Date().toISOString(),
    };

    // Create a placeholder for the assistant's response
    const assistantPlaceholderMessage: Message = {
      id: (Date.now() + 1).toString(), // Ensure unique ID
      content: "",
      role: "assistant",
      timestamp: new Date().toISOString(),
      isLoadingPlaceholder: true,
    };

    setMessages((prev) => [...prev, userMessage, assistantPlaceholderMessage]);
    const currentQuery = inputValue; // Store inputValue before clearing
    setInputValue("");
    setIsLoading(true);

    try {
      const response = await fetch("/api/research/chat", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({
          query: currentQuery, // Use stored query
          documentPath: selectedDocument.path,
        }),
      });

      if (!response.ok) {
        // If the response is not OK, update the placeholder to show an error
        setMessages((prev) =>
          prev.map((msg) =>
            msg.id === assistantPlaceholderMessage.id
              ? { ...msg, content: "Sorry, I couldn't get a response. Please try again.", isLoadingPlaceholder: false }
              : msg
          )
        );
        throw new Error(`Failed to send message. Status: ${response.status}`);
      }

      const reader = response.body?.getReader();
      if (!reader) {
        // If no reader, update placeholder to show an error
        setMessages((prev) =>
          prev.map((msg) =>
            msg.id === assistantPlaceholderMessage.id
              ? { ...msg, content: "Sorry, there was an issue with the response stream.", isLoadingPlaceholder: false }
              : msg
          )
        );
        throw new Error("No response reader available");
      }
      
      // Remove the isLoading flag from the placeholder once we start receiving data
      // and prepare to fill its content.
      // We find it by ID and update it.
      setMessages((prevMessages) =>
        prevMessages.map((msg) =>
          msg.id === assistantPlaceholderMessage.id
            ? { ...msg, isLoadingPlaceholder: false, content: "" } // Clear content, remove placeholder flag
            : msg
        )
      );

      const decoder = new TextDecoder();
      let buffer = "";

      while (true) {
        const { done, value } = await reader.read();
        if (done) break;

        buffer += decoder.decode(value, { stream: true });
        
        setMessages((prev) =>
          prev.map((msg) =>
            msg.id === assistantPlaceholderMessage.id
              ? { ...msg, content: buffer, isLoadingPlaceholder: false } // Update content, ensure placeholder is false
              : msg
          )
        );
      }
    } catch (error) {
      console.error("Chat error:", error);
      // If an error occurred and it wasn't handled by updating the placeholder already,
      // ensure the placeholder is removed or updated to an error message.
      setMessages((prev) =>
        prev.map((msg) =>
          msg.id === assistantPlaceholderMessage.id && msg.isLoadingPlaceholder
            ? { ...msg, content: "Sorry, an error occurred. Please try again.", isLoadingPlaceholder: false }
            : msg
        )
      );
    } finally {
      setIsLoading(false);
    }
  };

  const handleKeyDown = (e: React.KeyboardEvent) => {
    if (e.key === "Enter" && !e.shiftKey) {
      e.preventDefault();
      handleSendMessage();
    }
  };

  return (
    <div className="flex h-screen bg-background">
      {/* Left Sidebar: Document Selection */}
      <div className={`transition-all duration-300 border-r border-border bg-card/20 backdrop-blur-sm ${
        isSidebarCollapsed ? "w-12" : "w-64"
      }`}>
        <div className="p-3 border-b border-border flex items-center justify-between">
          {!isSidebarCollapsed && (
            <h2 className="text-lg font-bold">Research Library</h2>
          )}
          <Button
            variant="ghost"
            size="sm"
            onClick={() => setIsSidebarCollapsed(!isSidebarCollapsed)}
            className="p-1 h-8 w-8 hover:bg-accent/50 z-[9999]"
            aria-label={isSidebarCollapsed ? "Expand sidebar" : "Collapse sidebar"}
          >
            {isSidebarCollapsed ? (
              <svg className="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M9 5l7 7-7 7" />
              </svg>
            ) : (
              <svg className="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M15 19l-7-7 7-7" />
              </svg>
            )}
          </Button>
        </div>
        
        {!isSidebarCollapsed && (
          <div className="p-3">
            {isLoadingDocuments ? (
              <div className="flex justify-center py-6">
                <Loader />
              </div>
            ) : documents.length === 0 ? (
              <div className="text-center py-6 text-muted-foreground">
                <div className="text-xl mb-2">📚</div>
                <p className="text-sm">No documents found</p>
              </div>
            ) : (
              <div className="space-y-2">
                <label className="text-xs font-medium text-muted-foreground uppercase tracking-wide">Available Documents</label>
                <div className="space-y-1.5">
                  {documents.map((doc) => (
                    <button
                      key={doc.id}
                      onClick={() => setSelectedDocument(doc)}
                      className={`w-full p-2.5 text-left rounded-md border transition-all hover:bg-accent/50 ${
                        selectedDocument?.id === doc.id
                          ? "bg-primary/10 border-primary/20 ring-1 ring-primary/20"
                          : "bg-background border-border hover:border-border/60"
                      }`}
                      title={doc.displayTitle || doc.name}
                    >
                      <div className="text-sm font-medium">{doc.displayTitle || doc.name}</div>
                      <div className="text-xs text-muted-foreground truncate">
                        {doc.name} • {(doc.size / 1024 / 1024).toFixed(1)} MB
                      </div>
                    </button>
                  ))}
                </div>
              </div>
            )}
          </div>
        )}
        
        {isSidebarCollapsed && selectedDocument && (
          <div className="p-2 flex justify-center">
            <div
              className="w-8 h-8 rounded bg-primary/10 border border-primary/20 flex items-center justify-center cursor-pointer hover:bg-primary/20 transition-colors"
              onClick={() => setIsSidebarCollapsed(false)}
              title={selectedDocument.displayTitle || selectedDocument.name}
            >
              <div className="text-xs font-bold text-primary">
                {(selectedDocument.displayTitle || selectedDocument.name).charAt(0).toUpperCase()}
              </div>
            </div>
          </div>
        )}
      </div>

      {/* Main Content Area */}
      <div className="flex-1 flex">
        {/* Chat Interface */}
        <div className="flex-1 flex flex-col">
          {!selectedDocument ? (
            <div className="flex-1 flex items-center justify-center">
              <div className="text-center">
                <div className="text-4xl mb-4">💬</div>
                <h3 className="text-xl font-bold mb-2">Select a Document</h3>
                <p className="text-muted-foreground">
                  Choose a PDF from the sidebar to start chatting.
                </p>
              </div>
            </div>
          ) : (
            <>
              {/* Chat Header */}
              <div className="border-b border-border p-4">
                <h3 className="font-semibold">Chat with {selectedDocument.displayTitle || selectedDocument.name}</h3>
                <p className="text-sm text-muted-foreground">Ask questions about this document</p>
              </div>

              {/* Messages */}
              <div className="flex-1 overflow-y-auto p-4 space-y-4">
                {messages.map((message) => (
                  <div
                    key={message.id}
                    className={`flex ${message.role === "user" ? "justify-end" : "justify-start"}`}
                  >
                    <div
                      className={`max-w-[80%] p-4 py-2.5 rounded-lg shadow-sm transition-all duration-200 ease-out ${
                        message.role === "user"
                          ? "bg-primary text-primary-foreground"
                          : "bg-card border border-border"
                      }`}
                    >
                      {message.role === "user" ? (
                        <div className="text-md">
                            {message.content}
                        </div>
                      ) : message.isLoadingPlaceholder ? (
                        <div className="flex items-center justify-center p-1">
                          <Loader size="lg" variant="loading-dots" />
                        </div>
                      ) : (
                        <div className="text-md font-medium">
                          <MarkdownRenderer content={message.content} />
                        </div>
                      )}
                    </div>
                  </div>
                ))}
                <div ref={messagesEndRef} />
              </div>

              {/* Input */}
              <div className="border-t border-border p-4">
                <div className="flex space-x-2">
                  <textarea
                    value={inputValue}
                    onChange={(e) => setInputValue(e.target.value)}
                    onKeyDown={handleKeyDown}
                    placeholder={isPreparingIndex ? "Preparing document..." : "Ask questions about this document..."}
                    className="flex-1 p-3 border border-border rounded-lg bg-background resize-none"
                    rows={2}
                    disabled={isLoading || isPreparingIndex}
                  />
                  <Button
                    onClick={handleSendMessage}
                    disabled={!inputValue.trim() || isLoading || isPreparingIndex}
                    className="px-6"
                  >
                    {isLoading ? <Loader /> : isPreparingIndex ? <Loader /> : "Send"}
                  </Button>
                </div>
              </div>
            </>
          )}
        </div>

        {/* PDF Preview */}
        <div className="w-2/5 border-l border-border bg-card/10 backdrop-blur-sm">
          {selectedDocument ? (
            <div className="h-full flex flex-col">
              <div className="border-b border-border p-4">
                <h3 className="font-semibold">{selectedDocument.displayTitle || selectedDocument.name}</h3>
                <p className="text-sm text-muted-foreground">PDF Preview</p>
              </div>
              <div className="flex-1">
                <iframe
                  src={`/api/research/files/${selectedDocument.path}#toolbar=0&navpanes=0&scrollbar=1`}
                  className="w-full h-full border-0"
                  title={`Preview of ${selectedDocument.name}`}
                />
              </div>
            </div>
          ) : (
            <div className="h-full flex items-center justify-center">
              <div className="text-center text-muted-foreground">
                <div className="text-4xl mb-4">📄</div>
                <p>PDF preview will appear here</p>
              </div>
            </div>
          )}
        </div>
      </div>
    </div>
  );
} 