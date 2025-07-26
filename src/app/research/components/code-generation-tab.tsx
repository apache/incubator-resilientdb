import { Badge } from "@/components/ui/badge";
import { Loader } from "@/components/ui/loader";
import { MarkdownRenderer } from "@/components/ui/markdown-renderer";
import { ScrollArea } from "@/components/ui/scroll-area";
import { TabsContent, TabsTrigger } from "@/components/ui/tabs";
import { Clock, Code2, Zap } from "lucide-react";
import { CodeGeneration } from "../types";

interface CodeSectionProps {
  generation: CodeGeneration;
}

const QuerySection: React.FC<CodeSectionProps> = ({ generation }) => (
  <div className="w-full min-w-0">
    <h3 className="text-sm font-semibold text-muted-foreground mb-2 flex items-center gap-2">
      <Clock className="h-3 w-3" />
      Query
    </h3>
    <div className="text-sm bg-muted/30 rounded-md p-3 border break-words w-full min-w-0 overflow-hidden">
      <MarkdownRenderer content={generation.query} />
    </div>
  </div>
);

const PlanSection: React.FC<CodeSectionProps> = ({ generation }) => (
  <div className="w-full min-w-0">
    <h3 className="text-sm font-semibold text-muted-foreground mb-2 flex items-center gap-2">
      Plan
      {generation.isStreaming && generation.currentSection === 'plan' && (
        <Loader size="sm" className="text-yellow-500" />
      )}
    </h3>
    <div className="text-sm bg-muted/30 rounded-md p-3 border whitespace-pre-wrap break-words w-full min-w-0 overflow-hidden min-h-[60px] flex items-start">
      {generation.plan ? (
        <MarkdownRenderer content={generation.plan} />
      ) : (generation.isStreaming && generation.currentSection === 'plan' ? (
        <span className="text-muted-foreground italic">Generating plan...</span>
      ) : (
        <span className="text-muted-foreground italic">Plan will appear here</span>
      ))}
    </div>
  </div>
);

const PseudocodeSection: React.FC<CodeSectionProps> = ({ generation }) => (
  <div className="w-full min-w-0">
    <h3 className="text-sm font-semibold text-muted-foreground mb-2 flex items-center gap-2">
      Pseudocode
      {generation.isStreaming && generation.currentSection === 'pseudocode' && (
        <Loader size="sm" className="text-yellow-500" />
      )}
    </h3>
    <div className="text-sm bg-muted/30 rounded-md p-3 border font-mono whitespace-pre-wrap break-words w-full min-w-0 overflow-hidden min-h-[60px] flex items-start">
      {generation.pseudocode ? (
        <MarkdownRenderer content={generation.pseudocode} />
      ) : (generation.isStreaming && generation.currentSection === 'pseudocode' ? (
        <span className="text-muted-foreground italic font-sans">Generating pseudocode...</span>
      ) : (
        <span className="text-muted-foreground italic font-sans">Pseudocode will appear here</span>
      ))}
    </div>
  </div>
);

interface ImplementationSectionProps extends CodeSectionProps {
  getLanguageLabel: (language: string) => string;
}

const ImplementationSection: React.FC<ImplementationSectionProps> = ({ generation, getLanguageLabel }) => (
  <div className="w-full min-w-0">
    <h3 className="text-sm font-semibold text-muted-foreground mb-2 flex items-center gap-2">
      <Code2 className="h-3 w-3" />
      Implementation ({getLanguageLabel(generation.language)})
      {generation.isStreaming && generation.currentSection === 'implementation' && (
        <Loader size="sm" className="text-yellow-500" />
      )}
    </h3>
    <div className="text-sm bg-muted/30 rounded-md p-3 border font-mono whitespace-pre-wrap break-words w-full min-w-0 overflow-hidden min-h-[100px] flex items-start">
      {generation.implementation ? (
        <MarkdownRenderer content={generation.implementation} />
      ) : (generation.isStreaming && generation.currentSection === 'implementation' ? (
        <span className="text-muted-foreground italic font-sans">Generating implementation...</span>
      ) : (
        <span className="text-muted-foreground italic font-sans">Implementation will appear here</span>
      ))}
    </div>
  </div>
);

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
              <PlanSection generation={generation} />
              <PseudocodeSection generation={generation} />
              <ImplementationSection 
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