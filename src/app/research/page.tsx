"use client";

import { ChatInput, type Language } from "@/app/research/components/chat-input";
import { PreviewPanel } from "@/app/research/components/preview-panel";
import { CodeGeneration } from "@/app/research/types";
import { ToolProvider } from "@/components/context/ToolContext";
import { Button } from "@/components/ui/button";
import {
  Card,
  CardContent,
  CardDescription,
  CardHeader,
  CardTitle,
} from "@/components/ui/card";
import { SourceAttribution } from "@/components/ui/document-source-badge";
import { Loader } from "@/components/ui/loader";
import { MarkdownRenderer } from "@/components/ui/markdown-renderer";
import { MultiDocumentSelector } from "@/components/ui/multi-document-selector";
import {
  ResizableHandle,
  ResizablePanel,
  ResizablePanelGroup,
} from "@/components/ui/resizable";
import { ScrollArea } from "@/components/ui/scroll-area";
import {
  Sheet,
  SheetContent,
  SheetHeader,
  SheetTitle,
  SheetTrigger,
} from "@/components/ui/sheet";
import { TooltipProvider } from "@/components/ui/tooltip";
import { Document, useDocuments } from "@/hooks/useDocuments";
import { parseChainOfThoughtResponse } from "@/lib/code-composer-prompts";
import { cleanUpImplementation } from "@/lib/utils";
import {
  ChevronLeft,
  ChevronRight,
  Menu,
  MessageCircle
} from "lucide-react";
import { useCallback, useEffect, useRef, useState } from "react";

interface Message {
  id: string;
  content: string;
  role: "user" | "assistant";
  timestamp: string;
  isLoadingPlaceholder?: boolean;
  sources?: {
    path: string;
    name: string;
    displayTitle?: string;
  }[];
}

// Helper functions for code composer streaming
const getCurrentSection = (fullResponse: string): 'reading-documents' | 'topic' | 'plan' | 'pseudocode' | 'implementation' => {
  const topicIndex = fullResponse.indexOf('## TOPIC');
  const planIndex = fullResponse.indexOf('## PLAN');
  const pseudocodeIndex = fullResponse.indexOf('## PSEUDOCODE');
  const implementationIndex = fullResponse.indexOf('## IMPLEMENTATION');
  
  // If we haven't received any structured content yet, we're still reading documents
  if (topicIndex === -1 && planIndex === -1 && pseudocodeIndex === -1 && implementationIndex === -1) {
    return 'reading-documents';
  }
  
  if (implementationIndex !== -1 && fullResponse.length > implementationIndex + 18) {
    return 'implementation';
  }
  if (pseudocodeIndex !== -1 && fullResponse.length > pseudocodeIndex + 15) {
    return 'pseudocode';
  }
  if (planIndex !== -1 && fullResponse.length > planIndex + 8) {
    return 'plan';
  }
  return 'topic';
};

const extractSectionsFromStream = (fullResponse: string) => {
  const topicStart = fullResponse.indexOf('## TOPIC');
  const planStart = fullResponse.indexOf('## PLAN');
  const pseudocodeStart = fullResponse.indexOf('## PSEUDOCODE');
  const implementationStart = fullResponse.indexOf('## IMPLEMENTATION');
  
  let topic = '';
  let plan = '';
  let pseudocode = '';
  let implementation = '';
  
  if (topicStart !== -1) {
    const topicEnd = planStart !== -1 ? planStart : fullResponse.length;
    topic = fullResponse.substring(topicStart + 9, topicEnd).trim();
  }
  
  if (planStart !== -1) {
    const planEnd = pseudocodeStart !== -1 ? pseudocodeStart : fullResponse.length;
    plan = fullResponse.substring(planStart + 8, planEnd).trim();
  }
  
  if (pseudocodeStart !== -1) {
    const pseudocodeEnd = implementationStart !== -1 ? implementationStart : fullResponse.length;
    pseudocode = fullResponse.substring(pseudocodeStart + 15, pseudocodeEnd).trim();
  }
  
  if (implementationStart !== -1) {
    implementation = fullResponse.substring(implementationStart + 18).trim();
    
  
    implementation = cleanUpImplementation(implementation);
    
  }
  
  return { topic, plan, pseudocode, implementation };
};

function ResearchChatPageContent() {
  const { data: documents = [], isLoading: isLoadingDocuments, error } = useDocuments();
  const [selectedDocuments, setSelectedDocuments] = useState<Document[]>([]);
  const [messages, setMessages] = useState<Message[]>([]);
  const [inputValue, setInputValue] = useState("");
  const [isLoading, setIsLoading] = useState(false);
  const [isPreparingIndex, setIsPreparingIndex] = useState(false);
  const [isSidebarCollapsed, setIsSidebarCollapsed] = useState(false);
  const [isMobileSheetOpen, setIsMobileSheetOpen] = useState(false);
  const [codeGenerations, setCodeGenerations] = useState<CodeGeneration[]>([]);

  const messagesEndRef = useRef<HTMLDivElement>(null);

  // Scroll to bottom when new messages arrive
  const scrollToBottom = useCallback(() => {
    messagesEndRef.current?.scrollIntoView({ behavior: "smooth" });
  }, []);

  useEffect(() => {
    scrollToBottom();
  }, [messages, scrollToBottom]);

  // Handle document loading errors
  useEffect(() => {
    if (error) {
      console.error("Failed to load documents:", error);
    }
  }, [error]);

  // Prepare index when document changes
  useEffect(() => {
    const prepareDocumentIndex = async () => {
      if (selectedDocuments.length > 0) {
        setIsPreparingIndex(true);

        const documentCount = selectedDocuments.length;
        const documentMessage =
          documentCount === 1
            ? `📄 **${selectedDocuments[0].displayTitle || selectedDocuments[0].name}** has been selected. Preparing document for questions...`
            : `📄 **${documentCount} documents** have been selected. Preparing documents for questions...`;

        setMessages([
          {
            id: Date.now().toString(),
            content: documentMessage,
            role: "assistant",
            timestamp: new Date().toISOString(),
          },
        ]);

        try {
          const response = await fetch("/api/research/prepare-index", {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({
              documentPaths: selectedDocuments.map((doc) => doc.path),
            }),
          });

          if (response.ok) {
            const readyMessage =
              documentCount === 1
                ? `✅ **${selectedDocuments[0].displayTitle || selectedDocuments[0].name}** is ready! You can now ask questions about this document.`
                : `✅ **${documentCount} documents** are ready! You can now ask questions about these documents.`;

            setMessages([
              {
                id: Date.now().toString(),
                content: readyMessage,
                role: "assistant",
                timestamp: new Date().toISOString(),
              },
            ]);
          } else {
            const error = await response.json();
            setMessages([
              {
                id: Date.now().toString(),
                content: `❌ Failed to prepare documents: ${error.error || "Unknown error"}`,
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
              content: `❌ Error preparing documents. Please try again.`,
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
  }, [selectedDocuments]);

  const handleCodeComposerStream = async (
    response: Response,
    assistantPlaceholderMessage: Message,
    payload: { query: string; documentPaths: string[]; tool?: string; language?: Language; scope?: string[] },
    earlyCodeGenerationId: string | null
  ) => {
    const reader = response.body?.getReader();
    if (!reader) {
      // If no reader, update placeholder to show an error
      setMessages((prev) =>
        prev.map((msg) =>
          msg.id === assistantPlaceholderMessage.id
            ? {
                ...msg,
                content: "Sorry, there was an issue with the response stream.",
                isLoadingPlaceholder: false,
              }
            : msg,
        ),
      );
      
      // Remove the early code generation if it was created
      if (earlyCodeGenerationId) {
        setCodeGenerations((prev) => 
          prev.filter(gen => gen.id !== earlyCodeGenerationId)
        );
      }
      
      throw new Error("No response reader available");
    }
    

    // Remove the isLoading flag from the placeholder once we start receiving data
    setMessages((prevMessages) =>
      prevMessages.map((msg) =>
        msg.id === assistantPlaceholderMessage.id
          ? { ...msg, isLoadingPlaceholder: false, content: "" }
          : msg,
      ),
    );

    const decoder = new TextDecoder();
    let buffer = "";
    let sourceInfo: any = null;
    let fullResponse = "";
    let currentCodeGeneration: string | null = null;

    while (true) {
      const { done, value } = await reader.read();
      if (done) break;

      const chunk = decoder.decode(value, { stream: true });
      buffer += chunk;
      fullResponse += chunk;

      // Check if we have source information at the beginning
      if (buffer.includes("__SOURCE_INFO__") && !sourceInfo) {
        const sourceInfoMatch = buffer.match(
          /__SOURCE_INFO__({[\s\S]*?})\n\n/,
        );
        if (sourceInfoMatch) {
          try {
            sourceInfo = JSON.parse(sourceInfoMatch[1]);
            buffer = buffer.replace(/__SOURCE_INFO__[\s\S]*?\n\n/, "");
            
            if (sourceInfo.tool === "code-composer") {
              if (earlyCodeGenerationId) {
                currentCodeGeneration = earlyCodeGenerationId;
                
                setCodeGenerations((prev) =>
                  prev.map((gen) =>
                    gen.id === earlyCodeGenerationId
                      ? {
                          ...gen,
                          language: sourceInfo.language || gen.language,
                          sources: sourceInfo?.sources || [],
                        }
                      : gen
                  )
                );
              } else {
                const codeGenId = Date.now().toString();
                currentCodeGeneration = codeGenId;
                
                const newCodeGeneration: CodeGeneration = {
                  id: codeGenId,
                  language: sourceInfo.language || "ts",
                  query: payload.query,
                  topic: "",
                  plan: "",
                  pseudocode: "",
                  implementation: "",
                  hasStructuredResponse: false,
                  timestamp: new Date().toISOString(),
                  isStreaming: true,
                  currentSection: 'reading-documents',
                  sources: sourceInfo?.sources || [],
                };

                setCodeGenerations((prev) => [...prev, newCodeGeneration]);
              }
              
              setMessages((prev) =>
                prev.map((msg) =>
                  msg.id === assistantPlaceholderMessage.id
                    ? {
                        ...msg,
                        content: "Code generation started. Check the preview panel to see live progress.",
                        isLoadingPlaceholder: false,
                        sources: sourceInfo?.sources || [],
                      }
                    : msg,
                ),
              );
              continue;
            }
          } catch (error) {
            console.error("Failed to parse source info:", error);
          }
        }
      }

      // Handle code composer live streaming
      if (sourceInfo?.tool === "code-composer" && currentCodeGeneration) {
        const currentSection = getCurrentSection(fullResponse);
        const { topic, plan, pseudocode, implementation } = extractSectionsFromStream(fullResponse);
        
        setCodeGenerations((prev) =>
          prev.map((gen) =>
            gen.id === currentCodeGeneration
              ? {
                  ...gen,
                  topic,
                  plan,
                  pseudocode,
                  implementation,
                  currentSection,
                }
              : gen
          )
        );
        continue;
      }

      if (buffer.includes("__CODE_COMPOSER_META__")) {
        const metaMatch = buffer.match(
          /__CODE_COMPOSER_META__({[\s\S]*?})\n\n/,
        );
        if (metaMatch) {
          try {
            const metadata = JSON.parse(metaMatch[1]);
            buffer = buffer.replace(/__CODE_COMPOSER_META__[\s\S]*?\n\n/, "");
          } catch (error) {
            console.error("Failed to parse code composer metadata:", error);
          }
        }
      }

      if (sourceInfo?.tool !== "code-composer") {
        setMessages((prev) =>
          prev.map((msg) =>
            msg.id === assistantPlaceholderMessage.id
              ? {
                  ...msg,
                  content: buffer,
                  isLoadingPlaceholder: false,
                  sources: sourceInfo?.sources || [],
                }
              : msg,
          ),
        );
      }
    }

    if (sourceInfo?.tool === "code-composer") {
      try {
        const parsed = parseChainOfThoughtResponse(fullResponse);
        
        let cleanImplementation = parsed.implementation;
        if (cleanImplementation) {
          cleanImplementation = cleanImplementation.replace(/__CODE_COMPOSER_META__[\s\S]*$/, '').trim();
          
          const lastCodeBlockEnd = cleanImplementation.lastIndexOf('```');
          if (lastCodeBlockEnd !== -1) {
            const afterCodeBlock = cleanImplementation.substring(lastCodeBlockEnd + 3).trim();
            if (afterCodeBlock.length > 50 && afterCodeBlock.includes('This implementation')) {
              cleanImplementation = cleanImplementation.substring(0, lastCodeBlockEnd + 3).trim();
            }
          }
        }
        
        setCodeGenerations((prev) => 
          prev.map((gen) => 
            gen.isStreaming ? {
              ...gen,
              topic: parsed.topic,
              plan: parsed.plan,
              pseudocode: parsed.pseudocode,
              implementation: cleanImplementation,
              hasStructuredResponse: parsed.hasStructuredResponse,
              isStreaming: false,
              currentSection: undefined,
            } : gen
          )
        );
      } catch (error) {
        console.error("Failed to parse code generation:", error);
      }
    }
  };

  const handleSendMessage = async (payload: { 
    query: string; 
    documentPaths: string[]; 
    tool?: string; 
    language?: Language; 
    scope?: string[] 
  }) => {
    const userMessage: Message = {
      id: Date.now().toString(),
      content: payload.query,
      role: "user",
      timestamp: new Date().toISOString(),
    };

    // Create a placeholder for the assistant's response
    const assistantPlaceholderMessage: Message = {
      id: (Date.now() + 1).toString(), // Ensure unique ID
      content: payload.tool === "code-composer" 
        ? "Reading and analyzing documents to generate code. Check the preview panel to see live progress."
        : "",
      role: "assistant",
      timestamp: new Date().toISOString(),
      isLoadingPlaceholder: payload.tool !== "code-composer",
    };

    setMessages((prev) => [...prev, userMessage, assistantPlaceholderMessage]);
    setInputValue("");
    setIsLoading(true);

    let earlyCodeGenerationId: string | null = null;
    if (payload.tool === "code-composer") {
      earlyCodeGenerationId = Date.now().toString();
      
      const newCodeGeneration: CodeGeneration = {
        id: earlyCodeGenerationId,
        language: payload.language || "ts",
        query: payload.query,
        topic: "",
        plan: "",
        pseudocode: "",
        implementation: "",
        hasStructuredResponse: false,
        timestamp: new Date().toISOString(),
        isStreaming: true,
        currentSection: 'reading-documents',
        sources: [],
      };

      setCodeGenerations((prev) => [...prev, newCodeGeneration]);
    }

    try {
      // Enable streaming for all research queries
      const streamingPayload = {
        ...payload,
        enableStreaming: true, // Enable streaming for real-time responses
      };
      
      const response = await fetch("/api/research/chat", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify(streamingPayload),
      });

      if (!response.ok) {
        setMessages((prev) =>
          prev.map((msg) =>
            msg.id === assistantPlaceholderMessage.id
              ? {
                  ...msg,
                  content: "Sorry, I couldn't get a response. Please try again.",
                  isLoadingPlaceholder: false,
                }
              : msg,
          ),
        );
        
        if (earlyCodeGenerationId) {
          setCodeGenerations((prev) => 
            prev.filter(gen => gen.id !== earlyCodeGenerationId)
          );
        }
        
        throw new Error(`Failed to send message. Status: ${response.status}`);
      }

      await handleCodeComposerStream(response, assistantPlaceholderMessage, payload, earlyCodeGenerationId);
    } catch (error) {
      console.error("Chat error:", error);
      setMessages((prev) =>
        prev.map((msg) =>
          msg.id === assistantPlaceholderMessage.id && msg.isLoadingPlaceholder
            ? {
                ...msg,
                content: "Sorry, an error occurred. Please try again.",
                isLoadingPlaceholder: false,
              }
            : msg,
        ),
      );
      
      if (earlyCodeGenerationId) {
        setCodeGenerations((prev) => 
          prev.filter(gen => gen.id !== earlyCodeGenerationId)
        );
      }
    } finally {
      setIsLoading(false);
    }
  };

  const handleKeyDown = (e: React.KeyboardEvent<HTMLTextAreaElement>) => {
    // This will be handled by ChatInput component
  };

  const handleDocumentKeyDown = useCallback(
    (e: React.KeyboardEvent, doc: Document) => {
      if (e.key === "Enter" || e.key === " ") {
        e.preventDefault();
        handleDocumentSelect(doc);
      }
    },
    [],
  );

  const handleDocumentSelect = useCallback((doc: Document) => {
    setSelectedDocuments((prev) => {
      const index = prev.findIndex((d) => d.id === doc.id);
      if (index === -1) {
        return [...prev, doc];
      }
      return prev;
    });
    setIsMobileSheetOpen(false);
  }, []);

  const handleDocumentDeselect = useCallback((doc: Document) => {
    setSelectedDocuments((prev) => prev.filter((d) => d.id !== doc.id));
  }, []);

  const handleSelectAll = useCallback(() => {
    setSelectedDocuments(documents);
  }, [documents]);

  const handleDeselectAll = useCallback(() => {
    setSelectedDocuments([]);
  }, []);

  const handleCloseCodeGeneration = (generationId: string) => {
    setCodeGenerations((prev) => prev.filter(gen => gen.id !== generationId));
  };

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
          <SheetContent
            side="left"
            className="w-80 p-0"
            aria-describedby="mobile-sheet-description"
          >
            <SheetHeader className="border-b">
              <SheetTitle>Research Library</SheetTitle>
              <p
                id="mobile-sheet-description"
                className="text-sm text-muted-foreground sr-only"
              >
                Select a document to start chatting with AI about its contents
              </p>
            </SheetHeader>
            <div className="p-4">
              <MultiDocumentSelector
                documents={documents}
                selectedDocuments={selectedDocuments}
                isLoadingDocuments={isLoadingDocuments}
                onDocumentSelect={handleDocumentSelect}
                onDocumentDeselect={handleDocumentDeselect}
                onSelectAll={handleSelectAll}
                onDeselectAll={handleDeselectAll}
                onDocumentKeyDown={handleDocumentKeyDown}
                showSearch={true}
                showSelectAll={true}
              />
            </div>
          </SheetContent>
        </Sheet>

        {/* Desktop Sidebar: Document Selection */}
        <Card
          className={`hidden md:flex transition-all duration-300 border-r bg-card/20 backdrop-blur-sm rounded-none ${
            isSidebarCollapsed ? "w-16" : "w-72 max-w-72"
          }`}
        >
          <div className="flex flex-col w-full">
            <CardHeader className="border-b flex flex-row items-center justify-between space-y-0 mt-[0.45rem]">
              {!isSidebarCollapsed && (
                <CardTitle className="text-lg">Research Library</CardTitle>
              )}
              <Button
                variant="ghost"
                size="sm"
                onClick={() => setIsSidebarCollapsed(!isSidebarCollapsed)}
                className="p-1 h-8 w-8 hover:bg-accent/50"
                aria-label={
                  isSidebarCollapsed ? "Expand sidebar" : "Collapse sidebar"
                }
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
                <MultiDocumentSelector
                  documents={documents}
                  selectedDocuments={selectedDocuments}
                  isLoadingDocuments={isLoadingDocuments}
                  onDocumentSelect={handleDocumentSelect}
                  onDocumentDeselect={handleDocumentDeselect}
                  onSelectAll={handleSelectAll}
                  onDeselectAll={handleDeselectAll}
                  onDocumentKeyDown={handleDocumentKeyDown}
                  showSearch={true}
                  showSelectAll={true}
                />
              </CardContent>
            )}

            {isSidebarCollapsed && selectedDocuments.length > 0 && (
              <CardContent className="p-3 flex justify-center">
                <Button
                  variant="outline"
                  size="sm"
                  className="w-10 h-10 p-0 rounded-full"
                  onClick={() => setIsSidebarCollapsed(false)}
                  aria-label={`Expand sidebar to see ${selectedDocuments.length} selected documents`}
                  title={`${selectedDocuments.length} documents selected`}
                >
                  <span className="text-sm font-semibold">
                    {selectedDocuments.length > 0
                      ? selectedDocuments.length.toString()
                      : "0"}
                  </span>
                </Button>
              </CardContent>
            )}
          </div>
        </Card>

        {/* Main Content Area */}
        <div className="flex-1 overflow-hidden">
          <ResizablePanelGroup direction="horizontal" className="h-full">
            {/* Chat Interface */}
            <ResizablePanel defaultSize={60} minSize={30}>
              <Card
                className="h-full flex flex-col rounded-none border-0 gap-0 min-h-0 bg-card/60 backdrop-blur-sm"
                role="main"
                aria-label="Chat interface"
              >
            {selectedDocuments.length === 0 ? (
              <CardContent className="flex-1 flex items-center justify-center p-4">
                <Card className="text-center max-w-md">
                  <CardContent className="pt-6">
                    <MessageCircle
                      className="h-16 w-16 mx-auto mb-4 text-muted-foreground"
                      aria-hidden="true"
                    />
                    <CardTitle className="text-xl mb-2">
                      Select Documents
                    </CardTitle>
                    <CardDescription className="mb-4">
                      Choose documents from the library to start chatting.
                    </CardDescription>
                    <Button
                      variant="outline"
                      onClick={() => setIsMobileSheetOpen(true)}
                      className="md:hidden"
                      aria-label="Browse documents to select for chatting"
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
                        Chat with{" "}
                        {selectedDocuments.length === 1
                          ? selectedDocuments[0].displayTitle ||
                            selectedDocuments[0].name
                          : `${selectedDocuments.length} Documents`}
                      </CardTitle>
                      <CardDescription>
                        Ask questions about{" "}
                        {selectedDocuments.length === 1
                          ? "this document"
                          : "these documents"}
                      </CardDescription>
                    </div>
                    <div className="flex items-center space-x-2">
                      <Button
                        variant="outline"
                        size="sm"
                        onClick={() => setIsMobileSheetOpen(true)}
                        className="md:hidden"
                        aria-label="Change document"
                      >
                        <Menu className="h-4 w-4" aria-hidden="true" />
                      </Button>
                    </div>
                  </div>
                </CardHeader>

                {/* Messages */}
                <div className="flex-1 min-h-0 overflow-hidden">
                  <ScrollArea
                    className="h-full p-4"
                    role="log"
                    aria-label="Chat messages"
                    aria-live="polite"
                  >
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
                                <p className="text-sm leading-relaxed">
                                  {message.content}
                                </p>
                              ) : message.isLoadingPlaceholder ? (
                                <div
                                  className="flex items-center justify-center py-2"
                                  aria-label="AI is thinking"
                                >
                                  <Loader size="md" variant="loading-dots" />
                                </div>
                              ) : (
                                <div className="text-sm">
                                  <MarkdownRenderer content={message.content} />
                                  {message.sources &&
                                    message.sources.length > 0 && (
                                      <SourceAttribution
                                        sources={message.sources}
                                        className="mt-2 pt-2 border-t border-border/20"
                                        showLabel={true}
                                        clickable={false}
                                      />
                                    )}
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
                <ChatInput
                  inputValue={inputValue}
                  setInputValue={setInputValue}
                  onSendMessage={handleSendMessage}
                  isLoading={isLoading}
                  isPreparingIndex={isPreparingIndex}
                  selectedDocuments={selectedDocuments}
                  onKeyDown={handleKeyDown}
                />
              </>
            )}
              </Card>
            </ResizablePanel>

            <ResizableHandle withHandle />

            {/* PDF Preview Panel */}
            <ResizablePanel defaultSize={40} minSize={40}>
              <PreviewPanel 
                selectedDocuments={selectedDocuments} 
                codeGenerations={codeGenerations}
                onCloseCodeGeneration={handleCloseCodeGeneration}
              />
            </ResizablePanel>
          </ResizablePanelGroup>
        </div>
      </div>
    </TooltipProvider>
  );
}

export default function ResearchChatPage() {
  return (
    <ToolProvider>
      <ResearchChatPageContent />
    </ToolProvider>
  );
}