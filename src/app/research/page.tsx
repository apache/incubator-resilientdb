"use client";

import type { Language } from "@/app/research/components/chat-input";
import { PreviewPanel } from "@/app/research/components/preview-panel";
import { CodeGeneration } from "@/app/research/types";
import {
  PromptInput,
  PromptInputButton,
  PromptInputModelSelect,
  PromptInputModelSelectContent,
  PromptInputModelSelectItem,
  PromptInputModelSelectTrigger,
  PromptInputModelSelectValue,
  PromptInputSubmit,
  PromptInputTextarea,
  PromptInputToolbar,
  PromptInputTools,
} from "@/components/ai-elements/prompt-input";
import { Response } from "@/components/ai-elements/response";
import { Tool, ToolHeader } from "@/components/ai-elements/tool";
import { ToolProvider, useTool } from "@/components/context/ToolContext";
import { Button } from "@/components/ui/button";
import {
  Card,
  CardContent,
  CardDescription,
  CardTitle
} from "@/components/ui/card";
import { SourceAttribution } from "@/components/ui/document-source-badge";
import { Loader } from "@/components/ui/loader";
import { MultiDocumentSelector } from "@/components/ui/multi-document-selector";
import {
  ResizableHandle,
  ResizablePanel,
  ResizablePanelGroup,
} from "@/components/ui/resizable";

import {
  Sheet,
  SheetContent,
  SheetHeader,
  SheetTitle,
  SheetTrigger,
} from "@/components/ui/sheet";
import { TooltipProvider } from "@/components/ui/tooltip";
import { Document, useDocuments } from "@/hooks/useDocuments";
import { formatToolHeader } from "@/lib/utils";
import { ChevronLeft, ChevronRight, GlobeIcon, Menu, MessageCircle, SquarePen } from "lucide-react";
import { useRouter, useSearchParams } from "next/navigation";
import { Suspense, useCallback, useEffect, useRef, useState } from "react";

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
  citations?: Record<
    number,
    import("@/components/ui/citation-badge").CitationData
  >;
  docPaths?: string[];
  toolParts?: {
    id?: string;
    type: string;
    state: "input-streaming" | "input-available" | "output-available" | "output-error";
    input?: unknown;
    output?: unknown;
    errorText?: string | null;
  }[];
  toolInserts?: { index: number; part: { id?: string; type: string; state: "input-streaming" | "input-available" | "output-available" | "output-error"; input?: unknown } }[];
}



function useSessionId() {
  const router = useRouter();
  const searchParams = useSearchParams();
  const sessionIdRef = useRef<string>(searchParams.get('session') || crypto.randomUUID());

  const resetSession = useCallback(() => {
    const newSessionId = crypto.randomUUID();
    sessionIdRef.current = newSessionId;
    router.push(`/research?session=${newSessionId}`);
  }, [router]);

  useEffect(() => {
    if (!searchParams.get('session')) {
      router.replace(`/research?session=${sessionIdRef.current}`);
    }
  }, [searchParams, router]);

  return { sessionId: sessionIdRef.current, resetSession };
}

function ResearchChatPageContent() {
  const modeOptions: Record<"research" | "code", string> = {
    research: "Research",
    code: "Code",
  };
  const {
    data: documents = [],
    isLoading: isLoadingDocuments,
    error,
  } = useDocuments();
  const [selectedDocuments, setSelectedDocuments] = useState<Document[]>([]);
  const [messages, setMessages] = useState<Message[]>([]);
  const [inputValue, setInputValue] = useState("");
  const [isLoading, setIsLoading] = useState(false);
  const [isPreparingIndex, setIsPreparingIndex] = useState(false);
  // eslint-disable-next-line @typescript-eslint/no-unused-vars
  const [indexError, setIndexError] = useState(false);
  const [isSidebarCollapsed, setIsSidebarCollapsed] = useState(false);
  const [isMobileSheetOpen, setIsMobileSheetOpen] = useState(false);
  const [codeGenerations, setCodeGenerations] = useState<CodeGeneration[]>([]);
  const { activeTool, setTool } = useTool();
  const [language, setLanguage] = useState<Language>("ts");
  const [mode, setMode] = useState<"research" | "code">("research");
  const [scrollOpacity, setScrollOpacity] = useState(0);

  const { sessionId, resetSession } = useSessionId();

  const messagesEndRef = useRef<HTMLDivElement>(null);
  const scrollContainerRef = useRef<HTMLDivElement>(null);

  // Scroll to bottom when new messages arrive
  const scrollToBottom = useCallback(() => {
    messagesEndRef.current?.scrollIntoView({ behavior: "smooth" });
  }, []);

  // Handle scroll to calculate gradient opacity
  const handleScroll = useCallback(() => {
    if (scrollContainerRef.current) {
      const { scrollTop } = scrollContainerRef.current;
      // Calculate opacity based on scroll position
      // Start fading in after 20px, fully visible after 100px
      const opacity = Math.min(Math.max((scrollTop - 20) / 80, 0), 1);
      setScrollOpacity(opacity);
    }
  }, []);

  useEffect(() => {
    scrollToBottom();
  }, [messages, scrollToBottom]);


  // Set up scroll listener
  useEffect(() => {
    const container = scrollContainerRef.current;
    if (container) {
      container.addEventListener('scroll', handleScroll);
      return () => container.removeEventListener('scroll', handleScroll);
    }
  }, [handleScroll]);

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
        setIndexError(false);
        try {
          const response = await fetch("/api/research/prepare-index", {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({
              documentPaths: selectedDocuments.map((doc) => doc.path),
            }),
          });
          if (!response.ok) {
            setIndexError(true);
          }
        } catch (error) {
          setIndexError(true);
          console.error("Error preparing document:", error);
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
    payload: {
      query: string;
      documentPaths: string[];
      tool?: string;
      language?: Language;
      scope?: string[];
    },
    earlyCodeGenerationId: string | null,
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
          prev.filter((gen) => gen.id !== earlyCodeGenerationId),
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
    let currentCodeGeneration: string | null = null;

    const isCodeComposerFlow = payload.tool === "code-composer";
    let phase: "reading-documents" | "plan" | "pseudocode" | "implementation" =
      "reading-documents";
    let prevPlainLength = 0;
    let planText = "";
    let pseudocodeText = "";
    let implementationText = "";
    let hasInitializedCodeGen = false;

    while (true) {
      const { done, value } = await reader.read();
      if (done) break;

      const chunk = decoder.decode(value, { stream: true });
      buffer += chunk;

      // Initialize code generation entry for code-composer flows
      if (isCodeComposerFlow && !hasInitializedCodeGen) {
        hasInitializedCodeGen = true;
        if (earlyCodeGenerationId) {
          currentCodeGeneration = earlyCodeGenerationId;
        } else {
          const codeGenId = Date.now().toString();
          currentCodeGeneration = codeGenId;
          const newCodeGeneration: CodeGeneration = {
            id: codeGenId,
            language: payload.language || "ts",
            query: payload.query,
            topic: "",
            plan: "",
            pseudocode: "",
            implementation: "",
            hasStructuredResponse: false,
            timestamp: new Date().toISOString(),
            isStreaming: true,
            currentSection: "reading-documents",
            sources: [],
          };
          setCodeGenerations((prev) => [...prev, newCodeGeneration]);
        }

        setMessages((prev) =>
          prev.map((msg) =>
            msg.id === assistantPlaceholderMessage.id
              ? {
                ...msg,
                content:
                  "Code generation started. Check the preview panel to see live progress. You may continue chatting after the code generation is complete.",
                isLoadingPlaceholder: false,
              }
              : msg,
          ),
        );

        setMode("research");
        setTool("default");
      }

      // Extract incremental tool call parts and capture insertion indices for interleaving
      if (buffer.includes("__TOOL_CALL__")) {
        const regex = /__TOOL_CALL__([\s\S]*?)\n\n/;
        let match: RegExpMatchArray | null;
        while ((match = buffer.match(regex))) {
          try {
            const fullMatch = match[0];
            const payloadJson = match[1];
            const matchIndex = (match as unknown as { index?: number }).index ?? buffer.indexOf("__TOOL_CALL__");

            // Compute insertion index at a safe boundary to avoid mid-word splits
            const before = buffer.slice(0, matchIndex);
            const after = buffer.slice(matchIndex + fullMatch.length);
            const joined = before + after;
            const candidateIndex = before.length; // original position of marker
            const isAlphaNum = (c: string) => /[A-Za-z0-9]/.test(c);
            const prevChar = candidateIndex > 0 ? joined[candidateIndex - 1] : "";
            const nextChar = joined[candidateIndex] ?? "";
            let insertIndex = candidateIndex;
            if (isAlphaNum(prevChar) && isAlphaNum(nextChar)) {
              // Advance to the end of the current word to avoid splitting it
              while (insertIndex < joined.length && isAlphaNum(joined[insertIndex])) {
                insertIndex += 1;
              }
            }

            // Remove the matched control block
            buffer = before + after;

            const part = JSON.parse(payloadJson);

            if (isCodeComposerFlow) {
              if (part.type === "handOff" && part.state === "input-available") {
                const toAgent = part.input?.toAgent;
                if (toAgent === "PseudoCodeAgent") {
                  phase = "pseudocode";
                } else if (toAgent === "CodeAgent") {
                  phase = "implementation";
                }
              }
              if (
                (part.type === "search_documents" || part.type === "retrieve_documents") &&
                part.state === "output-available" &&
                phase === "reading-documents"
              ) {
                phase = "plan";
                // Mark documents as processed in code generation panel
                if (currentCodeGeneration) {
                  setCodeGenerations((prev) =>
                    prev.map((gen) =>
                      gen.id === currentCodeGeneration
                        ? {
                          ...gen,
                          //   sources: (payload.documentPaths || []).map((p) => {
                          //     const normalized = p.replace(/^documents\//, "");
                          //     return {
                          //       path: normalized,
                          //       name: normalized,
                          //       displayTitle: TITLE_MAPPINGS[normalized] || normalized,
                          //     };
                          //   }),
                          // }
                          sources: (payload.documentPaths || [])
                            .filter(p => p != null)
                            .map((fileId) => {
                              // Find the document by ID to get proper display info
                              const doc = selectedDocuments.find(d => d.id === fileId);
                              return {
                                path: fileId,
                                name: doc?.name || fileId,
                                displayTitle: doc?.displayTitle || doc?.name || fileId,
                              };
                            }),}
                        : gen,
                    ),
                  );
                }
              }
              // Do not render tool inserts in chat for code generation mode
              continue;
            }

            // Regular research mode: show tool inserts inline
            setMessages((prev: Message[]) =>
              prev.map((msg): Message => {
                if (msg.id !== assistantPlaceholderMessage.id) return msg;
                const existingParts = msg.toolParts || [];
                const partIdx = existingParts.findIndex((p: any) => p.id && p.id === part.id);
                const nextToolParts = partIdx === -1
                  ? [...existingParts, part]
                  : existingParts.map((p, i) => (i === partIdx ? { ...p, ...part } : p));

                const existingInserts = msg.toolInserts ? [...msg.toolInserts] : [];
                const insIdx = existingInserts.findIndex((i) => i.part.id === part.id);
                let nextToolInserts = existingInserts;
                if (insIdx === -1) {
                  nextToolInserts = [...existingInserts, { index: insertIndex, part }];
                } else {
                  // Keep the original index; just update the part state
                  nextToolInserts[insIdx] = {
                    index: existingInserts[insIdx].index,
                    part: { ...existingInserts[insIdx].part, ...part },
                  };
                }

                return { ...msg, toolParts: nextToolParts, toolInserts: nextToolInserts };
              }),
            );
          } catch (error) {
            console.error("Failed to parse tool call part:", error);
          }
        }
      }

      // Handle code composer live streaming into the code generation tab
      if (isCodeComposerFlow && currentCodeGeneration) {
        let safeBuffer = buffer;
        const tailIdx2 = safeBuffer.lastIndexOf("__TOOL_CALL__");
        if (tailIdx2 !== -1) {
          const tail2 = safeBuffer.slice(tailIdx2);
          if (!/\n\n$/.test(tail2)) {
            safeBuffer = safeBuffer.slice(0, tailIdx2);
          }
        }

        const newText = safeBuffer.slice(prevPlainLength);
        if (newText) {
          if (phase === "plan") {
            planText += newText;
          } else if (phase === "pseudocode") {
            pseudocodeText += newText;
          } else if (phase === "implementation") {
            implementationText += newText;
          }

          const topicCandidate = planText.split("\n").map((s) => s.trim()).find((s) => s.length > 0) || "";
          setCodeGenerations((prev) =>
            prev.map((gen) =>
              gen.id === currentCodeGeneration
                ? {
                  ...gen,
                  topic: topicCandidate,
                  plan: planText,
                  pseudocode: pseudocodeText,
                  implementation: implementationText,
                  currentSection: phase,
                }
                : gen,
            ),
          );
        }
        prevPlainLength = safeBuffer.length;
        continue;
      }

      if (buffer.includes("__CODE_COMPOSER_META__")) {
        const metaMatch = buffer.match(
          /__CODE_COMPOSER_META__({[\s\S]*?})\n\n/,
        );
        if (metaMatch) {
          try {
            JSON.parse(metaMatch[1]);
            buffer = buffer.replace(/__CODE_COMPOSER_META__[\s\S]*?\n\n/, "");
          } catch (error) {
            console.error("Failed to parse code composer metadata:", error);
          }
        }
      }

      // Update message content for non-code-composer tools (regular chat)
      if (payload.tool !== "code-composer") {
        // Prevent leaking control delimiters to UI: strip any unterminated control block tail
        let safeBuffer = buffer;
        const tailIdx = safeBuffer.lastIndexOf("__TOOL_CALL__");
        if (tailIdx !== -1) {
          const tail = safeBuffer.slice(tailIdx);
          if (!/\n\n$/.test(tail)) {
            safeBuffer = safeBuffer.slice(0, tailIdx);
          }
        }
        setMessages((prev: Message[]) =>
          prev.map((msg): Message =>
            msg.id === assistantPlaceholderMessage.id
              ? {
                ...msg,
                content: safeBuffer,
                isLoadingPlaceholder: false,
                sources: [],
                citations: undefined,
              }
              : msg,
          ),
        );
      }
    }
    if (isCodeComposerFlow && currentCodeGeneration) {
      setCodeGenerations((prev) =>
        prev.map((gen) =>
          gen.id === currentCodeGeneration
            ? {
              ...gen,
              topic:
                planText
                  .split("\n")
                  .map((s) => s.trim())
                  .find((s) => s.length > 0) || "",
              plan: planText,
              pseudocode: pseudocodeText,
              implementation: implementationText,
              hasStructuredResponse: Boolean(
                planText || pseudocodeText || implementationText,
              ),
              isStreaming: false,
              currentSection: undefined,
            }
            : gen,
        ),
      );
    }
  };

  const handleSendMessage = async (payload: {
    query: string;
    documentPaths: string[];
    tool?: string;
    language?: Language;
    scope?: string[];
  }) => {
    const userMessage: Message = {
      id: crypto.randomUUID(),
      content: payload.query,
      role: "user",
      timestamp: new Date().toISOString(),
    };

    // Create a placeholder for the assistant's response
    const assistantPlaceholderMessage: Message = {
      id: crypto.randomUUID(),
      content:
        payload.tool === "code-composer"
          ? "Reading and analyzing documents to generate code. Check the preview panel to see live progress."
          : "",
      role: "assistant",
      timestamp: new Date().toISOString(),
      isLoadingPlaceholder: payload.tool !== "code-composer",
      docPaths: payload.documentPaths,
    };

    setMessages((prev) => [...prev, userMessage, assistantPlaceholderMessage]);
    setInputValue("");
    setIsLoading(true);

    let earlyCodeGenerationId: string | null = null;
    if (payload.tool === "code-composer") {
      earlyCodeGenerationId = crypto.randomUUID();

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
        currentSection: "reading-documents",
        sources: [],
      };

      setCodeGenerations((prev) => [...prev, newCodeGeneration]);
    }

    try {
      const streamingPayload = {
        ...payload,
        enableStreaming: true,
        sessionId,
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
                content:
                  "Sorry, I couldn't get a response. Please try again.",
                isLoadingPlaceholder: false,
              }
              : msg,
          ),
        );

        if (earlyCodeGenerationId) {
          setCodeGenerations((prev) =>
            prev.filter((gen) => gen.id !== earlyCodeGenerationId),
          );
        }

        throw new Error(`Failed to send message. Status: ${response.status}`);
      }

      await handleCodeComposerStream(
        response,
        assistantPlaceholderMessage,
        payload,
        earlyCodeGenerationId,
      );
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
          prev.filter((gen) => gen.id !== earlyCodeGenerationId),
        );
      }
    } finally {
      setIsLoading(false);
    }
  };

  const handleKeyDown = () => { };

  useEffect(() => {
    setMode(activeTool === "code-composer" ? "code" : "research");
  }, [activeTool]);

  const getPlaceholder = () => {
    if (isPreparingIndex) return "Preparing documents...";
    if (selectedDocuments.length === 0)
      return "Select documents to start chatting...";
    if (activeTool === "code-composer") return "Draft code from selected papers...";
    if (selectedDocuments.length === 1) {
      return `Ask questions about ${selectedDocuments[0].displayTitle || selectedDocuments[0].name
        }...`;
    }
    return `Ask questions about ${selectedDocuments.length} documents...`;
  };

  const handleSubmit = (e: React.FormEvent<HTMLFormElement>) => {
    e.preventDefault();
    if (
      !inputValue.trim() ||
      selectedDocuments.length === 0 ||
      isLoading ||
      isPreparingIndex
    ) {
      return;
    }

    const payload = {
      query: inputValue,
      documentPaths: selectedDocuments.map((doc) => doc.path),
      ...(activeTool === "code-composer" && {
        tool: "code-composer" as const,
        language,
        scope: [] as string[],
      }),
    };

    handleSendMessage(payload);
  };

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

  const handleDocumentKeyDown = useCallback(
    (e: React.KeyboardEvent, doc: Document) => {
      if (e.key === "Enter" || e.key === " ") {
        e.preventDefault();
        handleDocumentSelect(doc);
      }
    },
    [handleDocumentSelect],
  );


  const handleDocumentDeselect = useCallback((doc: Document) => {
    setSelectedDocuments((prev) => prev.filter((d) => d.id !== doc.id));
  }, []);

  const handleSelectAll = useCallback(() => {
    // `documents` may be grouped (Document[][]) or flat (Document[]).
    const allDocs =
      Array.isArray(documents) && documents.length > 0 && Array.isArray((documents as any)[0])
        ? (documents as Document[][]).flat()
        : (documents as unknown as Document[]);

    setSelectedDocuments(allDocs);
  }, [documents]);

  const handleDeselectAll = useCallback(() => {
    setSelectedDocuments([]);
  }, []);

  const handleCloseCodeGeneration = (generationId: string) => {
    setCodeGenerations((prev) => prev.filter((gen) => gen.id !== generationId));
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
          className={`hidden md:flex transition-all duration-300 border-r bg-card/20 backdrop-blur-sm rounded-none ${isSidebarCollapsed ? "w-16" : "w-72 max-w-72"
            }`}
        >
          <div className="flex flex-col w-full">
            <div className="flex flex-row items-center justify-between space-y-0 mt-[0.45rem] px-3.5">
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
            </div>

            {!isSidebarCollapsed && (
              <CardContent className="p-3 flex-1 overflow-hidden">
                <div className="flex items-center gap-2 mb-2 w-full">
                  <Button
                    size="sm"
                    variant="secondary"
                    disabled={selectedDocuments.length === 0 || messages.length === 0}
                    className="flex-1 gap-2"
                    onClick={() => {
                      resetSession();
                      setMessages([]);
                    }}
                  >
                    <SquarePen />
                    New Chat
                  </Button>
                </div>
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
                className="h-full flex flex-col rounded-none border-0 gap-0 min-h-0 bg-[#0A1023] backdrop-blur-sm"
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
                    <div
                      className="px-6 py-4 pb-12 absolute top-0 left-0 z-10 flex-shrink-0 w-full"
                      style={{
                        background: `linear-gradient(to bottom, 
                          rgba(10, 16, 35, ${scrollOpacity}) 0%, 
                          rgba(10, 16, 35, ${scrollOpacity * 1}) 30%, 
                          rgba(10, 16, 35, ${scrollOpacity * 0.8}) 50%, 
                          rgba(10, 16, 35, ${scrollOpacity * 0.6}) 65%, 
                          rgba(10, 16, 35, ${scrollOpacity * 0.4}) 80%, 
                          rgba(10, 16, 35, ${scrollOpacity * 0.2}) 90%, 
                          transparent 100%)`
                      }}
                    >
                      <h1 className="text-2xl md:text-xl font-bold tracking-tight text-white whitespace-nowrap ">
                        Nexus
                      </h1>
                    </div>

                    {/* Messages */}
                    <div className="flex-1 min-h-0 overflow-hidden">
                      <div
                        ref={scrollContainerRef}
                        className="h-full p-4 pb-0 overflow-y-auto scrollbar-none"
                        role="log"
                        aria-label="Chat messages"
                        aria-live="polite"
                        style={{
                          scrollbarWidth: 'none',
                          msOverflowStyle: 'none'
                        }}
                      >
                        <div className="min-h-full flex flex-col justify-end">
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
                                  className={`max-w-[85%] md:max-w-[80%] transition-all duration-200 ease-out ${message.role === "user"
                                      ? "bg-primary text-primary-foreground border-primary"
                                      : "bg-card"
                                    }`}
                                >
                                  <CardContent variant="message">
                                    {message.role === "user" ? (
                                      <p className="text-sm">
                                        {message.content}
                                      </p>
                                    ) : message.isLoadingPlaceholder ? (
                                      <div
                                        className="flex items-center justify-center "
                                        aria-label="AI is thinking"
                                      >
                                        <Loader
                                          size="md"
                                          variant="loading-dots"
                                        />
                                      </div>
                                    ) : (
                                      <div className="text-sm">
                                        {(() => {
                                          const content = message.content || "";
                                          const inserts = (message.toolInserts || [])
                                            .slice()
                                            .sort((a, b) => a.index - b.index);
                                          if (inserts.length === 0) {
                                            return <Response>{content}</Response>;
                                          }
                                          const nodes: React.ReactNode[] = [];
                                          let cursor = 0;
                                          inserts.forEach((ins, idx) => {
                                            const clamped = Math.min(Math.max(ins.index, 0), content.length);
                                            const text = content.slice(cursor, clamped);
                                            if (text) nodes.push(
                                              <Response key={`txt-${message.id}-${idx}`}>{text}</Response>
                                            );
                                            nodes.push(
                                              <Tool key={`tool-${message.id}-${ins.part.id || idx}`} defaultOpen={false}>
                                                <ToolHeader
                                                  type={formatToolHeader(
                                                    ins.part.type as string,
                                                    ins.part.state as any,
                                                    message.docPaths?.length
                                                  ) as any}
                                                  state={ins.part.state as any}
                                                />
                                              </Tool>
                                            );
                                            cursor = clamped;
                                          });
                                          const tail = content.slice(cursor);
                                          if (tail) nodes.push(
                                            <Response key={`txt-tail-${message.id}`}>{tail}</Response>
                                          );
                                          return <div className="space-y-2">{nodes}</div>;
                                        })()}

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
                        </div>
                      </div>
                    </div>

                    <div className="px-1.5">
                      <PromptInput onSubmit={handleSubmit} className="border-t flex-shrink-0 mt-0 relative">
                        <PromptInputTextarea
                          value={inputValue}
                          onChange={(e) => setInputValue(e.target.value)}
                          onKeyDown={handleKeyDown as any}
                          placeholder={getPlaceholder()}
                          disabled={
                            isLoading ||
                            isPreparingIndex ||
                            selectedDocuments.length === 0
                          }
                        />
                        <PromptInputToolbar className="border-t">
                          <PromptInputTools>
                            <PromptInputModelSelect
                              onValueChange={(value) => {
                                const next = value as "research" | "code";
                                setMode(next);
                                setTool(next === "code" ? "code-composer" : "default");
                              }}
                              value={mode}
                            >
                              <PromptInputModelSelectTrigger>
                                <PromptInputModelSelectValue />
                              </PromptInputModelSelectTrigger>
                              <PromptInputModelSelectContent>
                                {Object.entries(modeOptions).map(([id, name]) => (
                                  <PromptInputModelSelectItem
                                    key={id}
                                    value={id}
                                  >
                                    {name}
                                  </PromptInputModelSelectItem>
                                ))}
                              </PromptInputModelSelectContent>
                            </PromptInputModelSelect>
                            {mode === "code" && (
                              <PromptInputModelSelect
                                onValueChange={(value) => setLanguage(value as Language)}
                                value={language}
                              >
                                <PromptInputModelSelectTrigger>
                                  <PromptInputModelSelectValue />
                                </PromptInputModelSelectTrigger>
                                <PromptInputModelSelectContent>
                                  <PromptInputModelSelectItem value="ts">TypeScript</PromptInputModelSelectItem>
                                  <PromptInputModelSelectItem value="python">Python</PromptInputModelSelectItem>
                                  <PromptInputModelSelectItem value="cpp">C++</PromptInputModelSelectItem>
                                </PromptInputModelSelectContent>
                              </PromptInputModelSelect>
                            )}
                            <PromptInputButton disabled={mode === "code" || true /* TODO: update */}>
                              <GlobeIcon size={16} />
                              <span>Search</span>
                            </PromptInputButton>
                          </PromptInputTools>
                          <PromptInputSubmit
                            disabled={
                              !inputValue.trim() ||
                              isLoading ||
                              isPreparingIndex ||
                              selectedDocuments.length === 0
                            }
                            status={isLoading || isPreparingIndex ? "submitted" : ("ready" as any)}
                          />
                        </PromptInputToolbar>
                      </PromptInput>
                    </div>
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

function ResearchChatPageWrapper() {
  return (
    <Suspense fallback={<div>Loading...</div>}>
      <ResearchChatPageContent />
    </Suspense>
  );
}

export default function ResearchChatPage() {
  return (
    <ToolProvider>
      <ResearchChatPageWrapper />
    </ToolProvider>
  );
}
