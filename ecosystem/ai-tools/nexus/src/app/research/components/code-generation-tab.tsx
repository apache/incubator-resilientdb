/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

import { Response } from "@/components/ai-elements/response";
import {
  Accordion,
  AccordionContent,
  AccordionItem,
  AccordionTrigger,
} from "@/components/ui/accordion";
import { Badge } from "@/components/ui/badge";
import { Button } from "@/components/ui/button";
import { Loader } from "@/components/ui/loader";
import { ScrollArea } from "@/components/ui/scroll-area";
import { TabsContent, TabsTrigger } from "@/components/ui/tabs";
import { BookOpen, Check, Clock, Code2, Copy, X, Zap } from "lucide-react";
import { useState } from "react";
import { CodeGeneration } from "../types";

interface CodeSectionProps {
  generation: CodeGeneration;
}

const stripMarkdownFormatting = (content: string): string => {
  if (!content) return "";

  return content
    .replace(/```[\w]*\n?/g, "")
    .replace(/\n?```/g, "")
    .replace(/`([^`]+)`/g, "$1")
    .trim();
};

const useCopyToClipboard = () => {
  const [copiedStates, setCopiedStates] = useState<Record<string, boolean>>({});

  const copyToClipboard = async (text: string, key: string) => {
    try {
      const cleanText = stripMarkdownFormatting(text);
      await navigator.clipboard.writeText(cleanText);
      setCopiedStates((prev) => ({ ...prev, [key]: true }));
      setTimeout(() => {
        setCopiedStates((prev) => ({ ...prev, [key]: false }));
      }, 2000);
    } catch (err) {
      console.error("Failed to copy text: ", err);
    }
  };

  return { copyToClipboard, copiedStates };
};

// Topic section removed per spec

const QuerySection: React.FC<CodeSectionProps> = ({ generation }) => (
  <div className="w-full min-w-0">
    <h3 className="text-sm font-semibold text-muted-foreground mb-2 flex items-center gap-2">
      <Clock className="h-3 w-3" />
      Query
    </h3>
    <div className="text-sm bg-muted/30 rounded-md p-3 border break-words w-full min-w-0 overflow-hidden">
      <span className="font-medium text-foreground">{generation.query}</span>
    </div>
  </div>
);

const ReadingDocumentsSection: React.FC<CodeSectionProps> = ({
  generation,
}) => (
  <div className="w-full min-w-0 mb-4">
    <h3 className="text-sm font-semibold text-muted-foreground mb-1.5 flex items-center gap-2">
      <BookOpen className="h-3 w-3" />
      Reading Documents
      {(!generation.sources || generation.sources.length === 0) && (
        <Loader size="sm" className="text-yellow-500 w-3 h-3 ml-1" />
      )}
      {generation.sources && generation.sources.length > 0 && (
        <span className="text-green-600 text-xs font-medium">
          ✓ Documents processed
        </span>
      )}
    </h3>
    <div>
      {!generation.sources || generation.sources.length === 0 ? (
        <Loader
          variant="text-shimmer"
          text="Analyzing and ranking document content..."
          size="sm"
          className="italic"
        />
      ) : null}
    </div>
  </div>
);

interface AccordionSectionsProps extends CodeSectionProps {
  getLanguageLabel: (language: string) => string;
}

const AccordionSections: React.FC<AccordionSectionsProps> = ({
  generation,
  getLanguageLabel,
}) => {
  const { copyToClipboard, copiedStates } = useCopyToClipboard();

  const handleCopy = (
    content: string,
    section: string,
    event: React.MouseEvent,
  ) => {
    event.stopPropagation();
    copyToClipboard(content, `${generation.id}-${section}`);
  };

  const CopyButton = ({
    content,
    section,
    className = "",
  }: {
    content: string;
    section: string;
    className?: string;
  }) => {
    const isLoading =
      generation.isStreaming && generation.currentSection === section;
    const isCopied = copiedStates[`${generation.id}-${section}`];

    return (
      <Button
        variant="ghost"
        size="sm"
        className={`h-6 w-6 p-0 ${className}`}
        onClick={(e) => handleCopy(content, section, e)}
        disabled={!content || isLoading}
        title={isCopied ? "Copied!" : `Copy ${section}`}
      >
        {isCopied ? (
          <Check className="h-3 w-3 text-green-500" />
        ) : (
          <Copy className="h-3 w-3" />
        )}
      </Button>
    );
  };

  return (
    <Accordion
      type="multiple"
      defaultValue={["plan", "pseudocode", "implementation"]}
      className="w-full"
    >
      {/* Plan Section - Only render if content exists or is being generated */}
      {(generation.plan || (generation.isStreaming && generation.currentSection === "plan")) && (
        <AccordionItem value="plan">
          <AccordionTrigger className="text-sm font-semibold text-muted-foreground hover:no-underline">
            <div className="flex items-center gap-2">
              Plan
              {generation.isStreaming &&
              generation.currentSection === "plan" ? (
                <Loader size="sm" className="text-yellow-500 w-3 h-3 ml-2" />
              ) : (
                <CopyButton content={generation.plan || ""} section="plan" />
              )}
            </div>
          </AccordionTrigger>
          <AccordionContent>
            <div className="text-sm bg-muted/30 rounded-md p-3 border break-words w-full min-w-0 overflow-hidden min-h-[60px] flex items-start">
              {generation.plan ? (
                <Response>{generation.plan}</Response>
              ) : (
                <span className="text-muted-foreground italic">
                  Generating plan...
                </span>
              )}
            </div>
          </AccordionContent>
        </AccordionItem>
      )}

      {/* Pseudocode Section - Only render if content exists or is being generated */}
      {(generation.pseudocode ||
        (generation.isStreaming &&
          generation.currentSection === "pseudocode")) && (
        <AccordionItem value="pseudocode">
          <AccordionTrigger className="text-sm font-semibold text-muted-foreground hover:no-underline">
            <div className="flex items-center gap-2">
              Pseudocode
              {generation.isStreaming &&
              generation.currentSection === "pseudocode" ? (
                <Loader size="sm" className="text-yellow-500" />
              ) : (
                <CopyButton
                  content={generation.pseudocode || ""}
                  section="pseudocode"
                />
              )}
            </div>
          </AccordionTrigger>
          <AccordionContent>
            <div className="text-sm bg-muted/30 rounded-md p-3 border font-mono whitespace-pre-wrap break-words w-full min-w-0 overflow-hidden min-h-[60px] flex items-start">
              {generation.pseudocode ? (
                <Response>{generation.pseudocode}</Response>
              ) : (
                <span className="text-muted-foreground italic font-sans">
                  Generating pseudocode...
                </span>
              )}
            </div>
          </AccordionContent>
        </AccordionItem>
      )}

      {/* Implementation Section - Only render if content exists or is being generated */}
      {(generation.implementation || (generation.isStreaming && generation.currentSection === "implementation")) && (
        <AccordionItem value="implementation">
          <AccordionTrigger className="text-sm font-semibold text-muted-foreground hover:no-underline">
            <div className="flex items-center gap-2">
              <Code2 className="h-3 w-3" />
              Implementation ({getLanguageLabel(generation.language)})
              {generation.isStreaming &&
              generation.currentSection === "implementation" ? (
                <Loader size="sm" className="text-yellow-500" />
              ) : (
                <CopyButton
                  content={generation.implementation || ""}
                  section="implementation"
                />
              )}
            </div>
          </AccordionTrigger>
          <AccordionContent>
            <div className="text-sm bg-muted/30 rounded-md p-3 border font-mono whitespace-pre-wrap break-words w-full min-w-0 overflow-hidden min-h-[100px] flex items-start">
              {generation.implementation ? (
                <Response>{generation.implementation}</Response>
              ) : (
                <span className="text-muted-foreground italic font-sans">
                  Generating implementation...
                </span>
              )}
            </div>
          </AccordionContent>
        </AccordionItem>
      )}
    </Accordion>
  );
};

interface CodeGenerationTabProps {
  codeGenerations: CodeGeneration[];
  getLanguageLabel: (language: string) => string;
  onCloseTab?: (generationId: string) => void;
}

export const CodeGenerationTabs: React.FC<CodeGenerationTabProps> = ({
  codeGenerations,
  getLanguageLabel,
  onCloseTab,
}) => {
  const handleCloseTab = (generationId: string, event: React.MouseEvent) => {
    event.stopPropagation();
    event.preventDefault();
    onCloseTab?.(generationId);
  };

  return (
    <>
      {/* Code Generation Tab Triggers */}
      {codeGenerations.map((generation) => {
        const displayText = getLanguageLabel(generation.language);

        return (
          <TabsTrigger
            key={`code-${generation.id}`}
            value={`code-${generation.id}`}
            className="flex items-center text-xs max-w-[175px] relative group pr-4"
            title={`${getLanguageLabel(generation.language)} - ${generation.query}`}
          >
            {generation.isStreaming ? (
              <Zap className="h-3 w-3 flex-shrink-0 text-yellow-500 animate-pulse" />
            ) : (
              <Code2 className="h-3 w-3 flex-shrink-0" />
            )}
            <span className="truncate flex-1">{displayText}</span>
            {generation.isStreaming ? (
              <Badge
                variant="default"
                className="text-[10px] px-1 py-0 h-4 ml-1 bg-yellow-500/20 text-yellow-700 border-yellow-500/30"
              >
                Live
              </Badge>
            ) : (
              <></>
            )}
            {onCloseTab && !generation.isStreaming && (
              <Button
                variant="ghost"
                size="sm"
                className="absolute right-1 h-4 w-4 p-0 opacity-0 group-hover:opacity-100 transition-opacity hover:bg-destructive hover:text-destructive-foreground"
                onClick={(e) => handleCloseTab(generation.id, e)}
                title="Close tab"
              >
                <X className="h-3 w-3" />
              </Button>
            )}
          </TabsTrigger>
        );
      })}
    </>
  );
};

export const CodeGenerationContent: React.FC<CodeGenerationTabProps> = ({
  codeGenerations,
  getLanguageLabel,
}) => {
  return (
    <>
      {/* Code Generation Tab Content */}
      {codeGenerations.map((generation) => (
        <TabsContent
          key={`code-${generation.id}`}
          value={`code-${generation.id}`}
          className="h-full m-0 p-0 data-[state=active]:flex data-[state=inactive]:hidden w-full min-w-0"
        >
          <ScrollArea className="w-full h-full min-w-0">
            <div className="p-4 space-y-6 w-full min-w-0">
              <QuerySection generation={generation} />
              <ReadingDocumentsSection generation={generation} />
              <AccordionSections
                generation={generation}
                getLanguageLabel={getLanguageLabel}
              />
            </div>
          </ScrollArea>
        </TabsContent>
      ))}
    </>
  );
};
