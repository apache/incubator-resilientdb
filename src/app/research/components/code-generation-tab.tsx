import {
  Accordion,
  AccordionContent,
  AccordionItem,
  AccordionTrigger,
} from "@/components/ui/accordion";
import { Badge } from "@/components/ui/badge";
import { Button } from "@/components/ui/button";
import { Loader } from "@/components/ui/loader";
import { MarkdownRenderer } from "@/components/ui/markdown-renderer";
import { ScrollArea } from "@/components/ui/scroll-area";
import { TabsContent, TabsTrigger } from "@/components/ui/tabs";
import { Check, Clock, Code2, Copy, Zap } from "lucide-react";
import { useState } from "react";
import { CodeGeneration } from "../types";

interface CodeSectionProps {
  generation: CodeGeneration;
}

const stripMarkdownFormatting = (content: string): string => {
  if (!content) return '';
  
  return content
    .replace(/```[\w]*\n?/g, '')
    .replace(/\n?```/g, '')
    .replace(/`([^`]+)`/g, '$1')
    .trim();
};

const useCopyToClipboard = () => {
  const [copiedStates, setCopiedStates] = useState<Record<string, boolean>>({});

  const copyToClipboard = async (text: string, key: string) => {
    try {
      const cleanText = stripMarkdownFormatting(text);
      await navigator.clipboard.writeText(cleanText);
      setCopiedStates(prev => ({ ...prev, [key]: true }));
      setTimeout(() => {
        setCopiedStates(prev => ({ ...prev, [key]: false }));
      }, 2000);
    } catch (err) {
      console.error('Failed to copy text: ', err);
    }
  };

  return { copyToClipboard, copiedStates };
};

const TopicSection: React.FC<CodeSectionProps> = ({ generation }) => (
  <div className="w-full min-w-0 mb-2">
    <h3 className="text-sm font-semibold text-muted-foreground mb-2 flex items-center gap-2">
      <Zap className="h-3 w-3" />
      Topic
      {generation.isStreaming && generation.currentSection === 'topic' && (
        <Loader size="sm" className="text-yellow-500 w-3 h-3 ml-1" />
      )}
    </h3>
    <div >
      {generation.topic ? (
        <span className="font-medium text-foreground">{generation.topic}</span>
      ) : (generation.isStreaming && generation.currentSection === 'topic' ? (
        <span className="text-muted-foreground italic">Generating topic...</span>
      ) : (
        <span className="text-muted-foreground italic">Topic will appear here</span>
      ))}
    </div>
  </div>
);

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

interface AccordionSectionsProps extends CodeSectionProps {
  getLanguageLabel: (language: string) => string;
}

const AccordionSections: React.FC<AccordionSectionsProps> = ({ generation, getLanguageLabel }) => {
  const { copyToClipboard, copiedStates } = useCopyToClipboard();

  const handleCopy = (content: string, section: string, event: React.MouseEvent) => {
    event.stopPropagation();
    copyToClipboard(content, `${generation.id}-${section}`);
  };

  const CopyButton = ({ content, section, className = "" }: { content: string; section: string; className?: string }) => {
    const isLoading = generation.isStreaming && generation.currentSection === section;
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
      {/* Plan Section */}
      <AccordionItem value="plan">
        <AccordionTrigger className="text-sm font-semibold text-muted-foreground hover:no-underline">
          <div className="flex items-center gap-2">
            Plan
            {generation.isStreaming && generation.currentSection === 'plan' ? (
              <Loader size="sm" className="text-yellow-500 w-3 h-3 ml-2" />
            ) : (
              <CopyButton content={generation.plan || ""} section="plan" />
            )}
          </div>
        </AccordionTrigger>
        <AccordionContent>
          <div className="text-sm bg-muted/30 rounded-md p-3 border break-words w-full min-w-0 overflow-hidden min-h-[60px] flex items-start">
            {generation.plan ? (
              <MarkdownRenderer content={generation.plan} />
            ) : (generation.isStreaming && generation.currentSection === 'plan' ? (
              <span className="text-muted-foreground italic">Generating plan...</span>
            ) : (
              <span className="text-muted-foreground italic">Plan will appear here</span>
            ))}
          </div>
        </AccordionContent>
      </AccordionItem>

      {/* Pseudocode Section */}
      <AccordionItem value="pseudocode">
        <AccordionTrigger className="text-sm font-semibold text-muted-foreground hover:no-underline">
          <div className="flex items-center gap-2">
            Pseudocode
            {generation.isStreaming && generation.currentSection === 'pseudocode' ? (
              <Loader size="sm" className="text-yellow-500" />
            ) : (
              <CopyButton content={generation.pseudocode || ""} section="pseudocode" />
            )}
          </div>
        </AccordionTrigger>
        <AccordionContent>
          <div className="text-sm bg-muted/30 rounded-md p-3 border font-mono whitespace-pre-wrap break-words w-full min-w-0 overflow-hidden min-h-[60px] flex items-start">
            {generation.pseudocode ? (
              <MarkdownRenderer content={generation.pseudocode} />
            ) : (generation.isStreaming && generation.currentSection === 'pseudocode' ? (
              <span className="text-muted-foreground italic font-sans">Generating pseudocode...</span>
            ) : (
              <span className="text-muted-foreground italic font-sans">Pseudocode will appear here</span>
            ))}
          </div>
        </AccordionContent>
      </AccordionItem>

      {/* Implementation Section */}
      <AccordionItem value="implementation">
        <AccordionTrigger className="text-sm font-semibold text-muted-foreground hover:no-underline">
          <div className="flex items-center gap-2">
            <Code2 className="h-3 w-3" />
            Implementation ({getLanguageLabel(generation.language)})
              {generation.isStreaming && generation.currentSection === 'implementation' ? (
              <Loader size="sm" className="text-yellow-500" />
            ) : (
              <CopyButton content={generation.implementation || ""} section="implementation" />
            )}
          </div>
        </AccordionTrigger>
        <AccordionContent>
          <div className="text-sm bg-muted/30 rounded-md p-3 border font-mono whitespace-pre-wrap break-words w-full min-w-0 overflow-hidden min-h-[100px] flex items-start">
            {generation.implementation ? (
              <MarkdownRenderer content={generation.implementation} />
            ) : (generation.isStreaming && generation.currentSection === 'implementation' ? (
              <span className="text-muted-foreground italic font-sans">Generating implementation...</span>
            ) : (
              <span className="text-muted-foreground italic font-sans">Implementation will appear here</span>
            ))}
          </div>
        </AccordionContent>
      </AccordionItem>
    </Accordion>
  );
};

interface CodeGenerationTabProps {
  codeGenerations: CodeGeneration[];
  getLanguageLabel: (language: string) => string;
  formatTimestamp: (timestamp: string) => string;
}

export const CodeGenerationTabs: React.FC<CodeGenerationTabProps> = ({
  codeGenerations,
  getLanguageLabel,
  formatTimestamp,
}) => {
  return (
    <>
      {/* Code Generation Tab Triggers */}
      {codeGenerations.map((generation) => (
        <TabsTrigger
          key={`code-${generation.id}`}
          value={`code-${generation.id}`}
          className="flex items-center gap-2 text-xs max-w-[180px] relative group"
          title={`${getLanguageLabel(generation.language)} - ${generation.query}`}
        >
          {generation.isStreaming ? (
            <Zap className="h-3 w-3 flex-shrink-0 text-yellow-500 animate-pulse" />
          ) : (
            <Code2 className="h-3 w-3 flex-shrink-0" />
          )}
          <span className="truncate">
            {getLanguageLabel(generation.language)}
          </span>
          {generation.isStreaming ? (
            <Badge variant="default" className="text-[10px] px-1 py-0 h-4 ml-1 bg-yellow-500/20 text-yellow-700 border-yellow-500/30">
              Live
            </Badge>
          ) : (
            <Badge variant="secondary" className="text-[10px] px-1 py-0 h-4 ml-1">
              {formatTimestamp(generation.timestamp)}
            </Badge>
          )}
        </TabsTrigger>
      ))}
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
              <TopicSection generation={generation} />
              <AccordionSections generation={generation} getLanguageLabel={getLanguageLabel} />
            </div>
          </ScrollArea>
        </TabsContent>
      ))}
    </>
  );
};