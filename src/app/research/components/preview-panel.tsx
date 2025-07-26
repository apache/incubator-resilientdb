import { Card, CardContent, CardDescription, CardHeader, CardTitle } from "@/components/ui/card";
import { Tabs, TabsList } from "@/components/ui/tabs";
import { FileText } from "lucide-react";
import { useEffect, useRef, useState } from "react";
import { CodeGeneration, Document } from "../types";
import { CodeGenerationContent, CodeGenerationTabs } from "./code-generation-tab";
import { PDFPreviewContent, PDFPreviewTabs } from "./pdf-preview-tab";

interface PreviewPanelProps {
  selectedDocuments: Document[];
  codeGenerations?: CodeGeneration[];
  className?: string;
}

const PreviewPanel: React.FC<PreviewPanelProps> = ({ 
  selectedDocuments, 
  codeGenerations = [], 
  className = "" 
}) => {
  const hasCodeGenerations = codeGenerations.length > 0;
  const hasPdfs = selectedDocuments.length > 0;
  
  const [activeTab, setActiveTab] = useState<string>("");
  const previousCodeGenerationsRef = useRef<CodeGeneration[]>([]);
  const userHasInteractedRef = useRef(false);

  const getDefaultTabValue = () => {
    if (hasCodeGenerations) {
      const streamingGen = codeGenerations.find(gen => gen.isStreaming);
      if (streamingGen) return `code-${streamingGen.id}`;
      
      return `code-${codeGenerations[codeGenerations.length - 1].id}`;
    }
    return selectedDocuments[0]?.id;
  };

  const handleTabChange = (value: string) => {
    userHasInteractedRef.current = true;
    setActiveTab(value);
  };

  useEffect(() => {
    if (!activeTab || (!hasCodeGenerations && !hasPdfs)) {
      const defaultTab = getDefaultTabValue();
      if (defaultTab) {
        setActiveTab(defaultTab);
        userHasInteractedRef.current = false;
      }
    }
  }, [hasCodeGenerations, hasPdfs, activeTab]);

  useEffect(() => {
    const previousGenerations = previousCodeGenerationsRef.current;
    const newGenerations = codeGenerations.filter(
      gen => !previousGenerations.find(prev => prev.id === gen.id)
    );

    if (newGenerations.length > 0 && !userHasInteractedRef.current) {
      const latestGeneration = newGenerations[newGenerations.length - 1];
      setActiveTab(`code-${latestGeneration.id}`);
    }

    previousCodeGenerationsRef.current = [...codeGenerations];
  }, [codeGenerations]);

  useEffect(() => {
    userHasInteractedRef.current = false;
  }, [selectedDocuments]);

  const formatTimestamp = (timestamp: string) => {
    return new Date(timestamp).toLocaleTimeString([], { 
      hour: '2-digit', 
      minute: '2-digit' 
    });
  };

  const getLanguageLabel = (language: string) => {
    const labels: Record<string, string> = {
      ts: "TypeScript",
      cpp: "C++",
      python: "Python"
    };
    return labels[language] || language.toUpperCase();
  };

  return (
    <Card
      className={`w-full h-full bg-card/40 backdrop-blur-sm rounded-none border-0 min-h-0 flex ${className}`}
      role="complementary"
      aria-label="Preview panel"
    >
      {hasPdfs || hasCodeGenerations ? (
        <div className="h-full flex flex-col w-full min-w-0">
          <CardHeader className="border-b flex-shrink-0">
            <CardTitle className="text-lg truncate">
              {hasCodeGenerations && hasPdfs ? "Preview & Code" : hasCodeGenerations ? "Code Generations" : "PDF Preview"}
            </CardTitle>
            <CardDescription>
              {hasPdfs && (
                <>
                  {selectedDocuments.length === 1
                    ? "1 document selected"
                    : `${selectedDocuments.length} documents selected`}
                </>
              )}
              {hasCodeGenerations && hasPdfs && " • "}
              {hasCodeGenerations && (
                <>
                  {codeGenerations.length === 1
                    ? "1 code generation"
                    : `${codeGenerations.length} code generations`}
                </>
              )}
            </CardDescription>
          </CardHeader>
          <CardContent className="flex-1 p-0 min-h-0 w-full min-w-0">
            <Tabs
              value={activeTab}
              onValueChange={handleTabChange}
              className="h-full flex flex-col w-full min-w-0"
            >
              <div className="px-4 pt-4 pb-2">
                <TabsList className="w-full justify-start overflow-x-auto">
                  {/* Code Generation Tabs */}
                  <CodeGenerationTabs
                    codeGenerations={codeGenerations}
                    getLanguageLabel={getLanguageLabel}
                    formatTimestamp={formatTimestamp}
                  />
                  
                  {/* PDF Document Tabs */}
                  <PDFPreviewTabs
                    selectedDocuments={selectedDocuments}
                  />
                </TabsList>
              </div>
              
              <div className="flex-1 min-h-0 w-full min-w-0">
                {/* Code Generation Content */}
                <CodeGenerationContent
                  codeGenerations={codeGenerations}
                  getLanguageLabel={getLanguageLabel}
                  formatTimestamp={formatTimestamp}
                />

                {/* PDF Document Content */}
                <PDFPreviewContent
                  selectedDocuments={selectedDocuments}
                />
              </div>
            </Tabs>
          </CardContent>
        </div>
      ) : (
        <CardContent className="h-full flex items-center justify-center">
          <Card className="text-center">
            <CardContent className="pt-6">
              <FileText
                className="h-16 w-16 mx-auto mb-4 text-muted-foreground"
                aria-hidden="true"
              />
              <CardDescription>
                PDF preview and code generations will appear here
              </CardDescription>
            </CardContent>
          </Card>
        </CardContent>
      )}
    </Card>
  );
};

export { PreviewPanel };
