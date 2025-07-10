"use client";

import { Badge } from "@/components/ui/badge";
import { Button } from "@/components/ui/button";
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from "@/components/ui/card";
import { Label } from "@/components/ui/label";
import { Loader } from "@/components/ui/loader";
import { MarkdownRenderer } from "@/components/ui/markdown-renderer";
import { ScrollArea } from "@/components/ui/scroll-area";
import { Separator } from "@/components/ui/separator";
import { Sheet, SheetContent, SheetHeader, SheetTitle, SheetTrigger } from "@/components/ui/sheet";
import { Textarea } from "@/components/ui/textarea";
import { Tooltip, TooltipContent, TooltipProvider, TooltipTrigger } from "@/components/ui/tooltip";
import { ChevronLeft, ChevronRight, FileText, Menu, MessageCircle, Send } from "lucide-react";
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
  const [isMobileSheetOpen, setIsMobileSheetOpen] = useState(false);
  
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

  const handleDocumentKeyDown = (e: React.KeyboardEvent, doc: Document) => {
    if (e.key === "Enter" || e.key === " ") {
      e.preventDefault();
      handleDocumentSelect(doc);
    }
  };

  const handleDocumentSelect = (doc: Document) => {
    setSelectedDocument(doc);
    setIsMobileSheetOpen(false); // Close mobile sheet when document is selected
  };

  // Document Selection Component - reusable for both desktop and mobile
  const DocumentSelection = ({ className = "" }: { className?: string }) => (
    <div className={`space-y-3 ${className}`}>
      {isLoadingDocuments ? (
        <div className="flex justify-center py-6">
          <Loader />
        </div>
      ) : documents.length === 0 ? (
        <Card className="text-center py-6 border-dashed">
          <CardContent className="pt-6">
            <FileText className="h-12 w-12 mx-auto mb-2 text-muted-foreground" />
            <p className="text-sm text-muted-foreground">No documents found</p>
          </CardContent>
        </Card>
      ) : (
        <>
          <Label className="text-xs font-medium text-muted-foreground uppercase tracking-wide">
            Available Documents
          </Label>
          <ScrollArea className="h-[calc(100vh-240px)]">
            <div className="space-y-2">
              {documents.map((doc) => (
                <Card
                  key={doc.id}
                  variant="message"
                  className={`cursor-pointer transition-all hover:bg-accent/50 w-full ${
                    selectedDocument?.id === doc.id
                      ? "bg-primary/10 border-primary/20 ring-1 ring-primary/20"
                      : "hover:border-border/60"
                  }`}
                  onClick={() => handleDocumentSelect(doc)}
                  onKeyDown={(e) => handleDocumentKeyDown(e, doc)}
                  tabIndex={0}
                  role="option"
                  aria-selected={selectedDocument?.id === doc.id}
                >
                  <CardContent variant="message-compact" className="w-full px-3">
                    <div className="flex items-start space-x-1.5 w-full">
                      <FileText className="h-4 w-3 mt-0.5 text-muted-foreground flex-shrink-0" />
                      <div className="flex-1 min-w-0 overflow-hidden">
                        <Tooltip>
                          <TooltipTrigger asChild>
                            <p className="text-sm font-medium break-words leading-tight max-h-[2.4em] overflow-hidden text-ellipsis line-clamp-2">
                              {doc.displayTitle || doc.name}
                            </p>
                          </TooltipTrigger>
                          <TooltipContent side="right">
                            <p className="max-w-xs break-words">{doc.displayTitle || doc.name}</p>
                          </TooltipContent>
                        </Tooltip>
                        <div className="flex flex-wrap items-center gap-1 mt-2">
                          <Badge variant="secondary" className="text-xs truncate max-w-[120px]">
                            {doc.name}
                          </Badge>
                          <Badge variant="outline" className="text-xs flex-shrink-0">
                            {(doc.size / 1024 / 1024).toFixed(1)} MB
                          </Badge>
                        </div>
                      </div>
                    </div>
                  </CardContent>
                </Card>
              ))}
            </div>
          </ScrollArea>
        </>
      )}
    </div>
  );

  return (
    <TooltipProvider>
      <div className="flex h-screen bg-background">
        {/* Mobile Sheet for Document Selection */}
        <Sheet open={isMobileSheetOpen} onOpenChange={setIsMobileSheetOpen}>
          <SheetTrigger asChild>
            <Button
              variant="outline"
              size="sm"
              className="md:hidden fixed top-4 left-4 z-50"
              aria-label="Open document library"
            >
              <Menu className="h-4 w-4" />
            </Button>
          </SheetTrigger>
          <SheetContent side="left" className="w-80 p-0" aria-describedby="mobile-sheet-description">
            <SheetHeader className="p-4 border-b">
              <SheetTitle>Research Library</SheetTitle>
              <p id="mobile-sheet-description" className="text-sm text-muted-foreground sr-only">
                Select a document to start chatting with AI about its contents
              </p>
            </SheetHeader>
            <div className="p-4">
              <DocumentSelection />
            </div>
          </SheetContent>
        </Sheet>

        {/* Desktop Sidebar: Document Selection */}
        <Card className={`hidden md:flex transition-all duration-300 border-r bg-card/20 backdrop-blur-sm rounded-none ${
          isSidebarCollapsed ? "w-16" : "w-72 max-w-72"
        }`}>
          <div className="flex flex-col w-full">
            <CardHeader className="p-3 border-b flex flex-row items-center justify-between space-y-0">
              {!isSidebarCollapsed && (
                <CardTitle className="text-lg">Research Library</CardTitle>
              )}
              <Button
                variant="ghost"
                size="sm"
                onClick={() => setIsSidebarCollapsed(!isSidebarCollapsed)}
                className="p-1 h-8 w-8 hover:bg-accent/50"
                aria-label={isSidebarCollapsed ? "Expand sidebar" : "Collapse sidebar"}
              >
                {isSidebarCollapsed ? (
                  <ChevronRight className="h-4 w-4" />
                ) : (
                  <ChevronLeft className="h-4 w-4" />
                )}
              </Button>
            </CardHeader>
            
            {!isSidebarCollapsed && (
              <CardContent className="p-3 flex-1 overflow-hidden">
                <DocumentSelection />
              </CardContent>
            )}
            
            {isSidebarCollapsed && selectedDocument && (
              <CardContent className="p-3 flex justify-center">
                <Button
                  variant="outline"
                  size="sm"
                  className="w-10 h-10 p-0 rounded-full"
                  onClick={() => setIsSidebarCollapsed(false)}
                  aria-label={`Expand sidebar to see ${selectedDocument.displayTitle || selectedDocument.name}`}
                  title={selectedDocument.displayTitle || selectedDocument.name}
                >
                  <span className="text-sm font-semibold">
                    {(selectedDocument.displayTitle || selectedDocument.name).charAt(0).toUpperCase()}
                  </span>
                </Button>
              </CardContent>
            )}
          </div>
        </Card>

        {/* Main Content Area */}
        <div className="flex-1 flex flex-col md:flex-row overflow-hidden">
          {/* Chat Interface */}
          <Card className="flex-1 flex flex-col rounded-none border-0 min-h-0" role="main" aria-label="Chat interface">
            {!selectedDocument ? (
              <CardContent className="flex-1 flex items-center justify-center p-4">
                <Card className="text-center max-w-md">
                  <CardContent className="pt-6">
                    <MessageCircle className="h-16 w-16 mx-auto mb-4 text-muted-foreground" aria-hidden="true" />
                    <CardTitle className="text-xl mb-2">Select a Document</CardTitle>
                    <CardDescription className="mb-4">
                      Choose a PDF from the library to start chatting.
                    </CardDescription>
                    <Button
                      variant="outline"
                      onClick={() => setIsMobileSheetOpen(true)}
                      className="md:hidden"
                      aria-label="Browse documents to select one for chatting"
                    >
                      <Menu className="h-4 w-4 mr-2" aria-hidden="true" />
                      Browse Documents
                    </Button>
                  </CardContent>
                </Card>
              </CardContent>
            ) : (
              <>
                {/* Chat Header */}
                <CardHeader className="border-b flex-shrink-0">
                  <div className="flex items-center justify-between">
                    <div className="flex-1 min-w-0">
                      <CardTitle className="text-lg truncate">
                        Chat with {selectedDocument.displayTitle || selectedDocument.name}
                      </CardTitle>
                      <CardDescription>Ask questions about this document</CardDescription>
                    </div>
                    <Button
                      variant="outline"
                      size="sm"
                      onClick={() => setIsMobileSheetOpen(true)}
                      className="md:hidden ml-2"
                      aria-label="Change document"
                    >
                      <Menu className="h-4 w-4" aria-hidden="true" />
                    </Button>
                  </div>
                </CardHeader>

                {/* Messages */}
                <div className="flex-1 min-h-0 overflow-hidden">
                  <ScrollArea className="h-full p-4" role="log" aria-label="Chat messages" aria-live="polite">
                    <div className="space-y-3">
                      {messages.map((message) => (
                        <div
                          key={message.id}
                          className={`flex ${message.role === "user" ? "justify-end" : "justify-start"}`}
                          role="article"
                          aria-label={`${message.role === "user" ? "Your message" : "AI response"}`}
                        >
                          <Card
                            variant="message"
                            className={`max-w-[85%] md:max-w-[80%] transition-all duration-200 ease-out ${
                              message.role === "user"
                                ? "bg-primary text-primary-foreground border-primary"
                                : "bg-card"
                            }`}
                          >
                            <CardContent variant="message">
                              {message.role === "user" ? (
                                <p className="text-sm leading-relaxed">{message.content}</p>
                              ) : message.isLoadingPlaceholder ? (
                                <div className="flex items-center justify-center py-2" aria-label="AI is thinking">
                                  <Loader size="md" variant="loading-dots" />
                                </div>
                              ) : (
                                <div className="text-sm">
                                  <MarkdownRenderer content={message.content} />
                                </div>
                              )}
                            </CardContent>
                          </Card>
                        </div>
                      ))}
                      <div ref={messagesEndRef} />
                    </div>
                  </ScrollArea>
                </div>

                {/* Input */}
                <CardContent className="border-t p-4 flex-shrink-0" role="form" aria-label="Send message">
                  <div className="flex space-x-2">
                    <div className="flex-1 space-y-2">
                      <Label htmlFor="message-input" className="sr-only">
                        Type your message about the document
                      </Label>
                      <Textarea
                        id="message-input"
                        value={inputValue}
                        onChange={(e) => setInputValue(e.target.value)}
                        onKeyDown={handleKeyDown}
                        placeholder={
                          isPreparingIndex 
                            ? "Preparing document..." 
                            : "Ask questions about this document..."
                        }
                        className="resize-none"
                        rows={2}
                        disabled={isLoading || isPreparingIndex}
                        aria-describedby="message-input-help"
                      />
                      <p id="message-input-help" className="sr-only">
                        Press Enter to send your message, or Shift+Enter for a new line
                      </p>
                    </div>
                    <Button
                      onClick={handleSendMessage}
                      disabled={!inputValue.trim() || isLoading || isPreparingIndex}
                      className="px-4"
                      size="lg"
                      aria-label="Send message"
                    >
                      {isLoading || isPreparingIndex ? (
                        <Loader size="sm" aria-label="Sending..." />
                      ) : (
                        <Send className="h-4 w-4" aria-hidden="true" />
                      )}
                    </Button>
                  </div>
                </CardContent>
              </>
            )}
          </Card>

          <Separator orientation="vertical" className="hidden md:block" />

          {/* PDF Preview - Hidden on mobile when no document selected */}
          <Card className={`w-full md:w-2/5 bg-card/10 backdrop-blur-sm rounded-none border-0 min-h-0 ${
            !selectedDocument ? "hidden md:flex" : "hidden md:flex"
          }`} role="complementary" aria-label="PDF preview">
            {selectedDocument ? (
              <div className="h-full flex flex-col">
                <CardHeader className="border-b flex-shrink-0">
                  <CardTitle className="text-lg truncate">
                    {selectedDocument.displayTitle || selectedDocument.name}
                  </CardTitle>
                  <CardDescription>PDF Preview</CardDescription>
                </CardHeader>
                <CardContent className="flex-1 p-0 min-h-0">
                  <iframe
                    src={`/api/research/files/${selectedDocument.path}#toolbar=0&navpanes=0&scrollbar=1`}
                    className="w-full h-full border-0"
                    title={`Preview of ${selectedDocument.name}`}
                    aria-label={`PDF preview of ${selectedDocument.displayTitle || selectedDocument.name}`}
                  />
                </CardContent>
              </div>
            ) : (
              <CardContent className="h-full flex items-center justify-center">
                <Card className="text-center">
                  <CardContent className="pt-6">
                    <FileText className="h-16 w-16 mx-auto mb-4 text-muted-foreground" aria-hidden="true" />
                    <CardDescription>PDF preview will appear here</CardDescription>
                  </CardContent>
                </Card>
              </CardContent>
            )}
          </Card>
        </div>
      </div>
    </TooltipProvider>
  );
} 