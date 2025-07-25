import { Badge } from "@/components/ui/badge";
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from "@/components/ui/card";
import { ScrollArea } from "@/components/ui/scroll-area";
import { Tabs, TabsContent, TabsList, TabsTrigger } from "@/components/ui/tabs";
import { Clock, Code2, FileText } from "lucide-react";

interface Document {
  id: string;
  name: string;
  path: string;
  size: number;
  uploadedAt: string;
  displayTitle?: string;
}

export interface CodeGeneration {
  id: string;
  language: string;
  query: string;
  plan: string;
  pseudocode: string;
  implementation: string;
  hasStructuredResponse: boolean;
  timestamp: string;
}

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

  const formatTimestamp = (timestamp: string) => {
    return new Date(timestamp).toLocaleTimeString([], { 
      hour: '2-digit', 
      minute: '2-digit' 
    });
  };

  const getLanguageLabel = (language: string) => {
    const labels: Record<string, string> = {
      ts: "TypeScript",
      js: "JavaScript", 
      go: "Go",
      rust: "Rust",
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
        <div className="h-full flex flex-col">
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
          <CardContent className="flex-1 p-0 min-h-0">
            <Tabs
              defaultValue={hasCodeGenerations ? `code-${codeGenerations[0]?.id}` : selectedDocuments[0]?.id}
              className="h-full flex flex-col"
            >
              <div className="px-4 pt-4 pb-2">
                <TabsList className="w-full justify-start overflow-x-auto">
                  {/* Code Generation Tabs */}
                  {codeGenerations.map((generation) => (
                    <TabsTrigger
                      key={`code-${generation.id}`}
                      value={`code-${generation.id}`}
                      className="flex items-center gap-2 text-xs max-w-[180px] relative group"
                      title={`${getLanguageLabel(generation.language)} - ${generation.query}`}
                    >
                      <Code2 className="h-3 w-3 flex-shrink-0" />
                      <span className="truncate">
                        {getLanguageLabel(generation.language)}
                      </span>
                      <Badge variant="secondary" className="text-[10px] px-1 py-0 h-4 ml-1">
                        {formatTimestamp(generation.timestamp)}
                      </Badge>
                    </TabsTrigger>
                  ))}
                  
                  {/* PDF Document Tabs */}
                  {selectedDocuments.map((doc) => (
                    <TabsTrigger
                      key={doc.id}
                      value={doc.id}
                      className="flex items-center gap-2 text-xs max-w-[150px] relative group"
                      title={doc.displayTitle || doc.name}
                    >
                      <FileText className="h-3 w-3 flex-shrink-0" />
                      <span className="truncate">
                        {doc.displayTitle || doc.name}
                      </span>
                    </TabsTrigger>
                  ))}
                </TabsList>
              </div>
              
              <div className="flex-1 min-h-0">
                {/* Code Generation Content */}
                {codeGenerations.map((generation) => (
                  <TabsContent
                    key={`code-${generation.id}`}
                    value={`code-${generation.id}`}
                    className="h-full m-0 p-0 data-[state=active]:flex data-[state=inactive]:hidden"
                  >
                    <ScrollArea className="w-full h-full">
                      <div className="p-4 space-y-6 w-full max-w-full">
                        {/* Query */}
                        <div className="w-full">
                          <h3 className="text-sm font-semibold text-muted-foreground mb-2 flex items-center gap-2">
                            <Clock className="h-3 w-3" />
                            Query
                          </h3>
                          <div className="text-sm bg-muted/30 rounded-md p-3 border break-words w-full overflow-hidden">
                            {generation.query}
                          </div>
                        </div>

                        {/* Plan */}
                        {generation.plan && (
                          <div className="w-full">
                            <h3 className="text-sm font-semibold text-muted-foreground mb-2">
                              Plan
                            </h3>
                            <div className="text-sm bg-muted/30 rounded-md p-3 border whitespace-pre-wrap break-words w-full overflow-hidden">
                              {generation.plan}
                            </div>
                          </div>
                        )}

                        {/* Pseudocode */}
                        {generation.pseudocode && (
                          <div className="w-full">
                            <h3 className="text-sm font-semibold text-muted-foreground mb-2">
                              Pseudocode
                            </h3>
                            <div className="text-sm bg-muted/30 rounded-md p-3 border font-mono whitespace-pre-wrap break-words w-full overflow-hidden">
                              {generation.pseudocode}
                            </div>
                          </div>
                        )}

                        {/* Implementation */}
                        {generation.implementation && (
                          <div className="w-full">
                            <h3 className="text-sm font-semibold text-muted-foreground mb-2 flex items-center gap-2">
                              <Code2 className="h-3 w-3" />
                              Implementation ({getLanguageLabel(generation.language)})
                            </h3>
                            <div className="text-sm bg-muted/30 rounded-md p-3 border font-mono whitespace-pre-wrap break-words w-full overflow-hidden">
                              {generation.implementation}
                            </div>
                          </div>
                        )}
                      </div>
                    </ScrollArea>
                  </TabsContent>
                ))}

                {/* PDF Document Content */}
                {selectedDocuments.map((doc) => (
                  <TabsContent
                    key={doc.id}
                    value={doc.id}
                    className="h-full m-0 p-0 data-[state=active]:flex data-[state=inactive]:hidden"
                  >
                    <iframe
                      src={`/api/research/files/${doc.path}#toolbar=0&navpanes=0&scrollbar=1`}
                      className="w-full h-full border-0"
                      title={`Preview of ${doc.name}`}
                      aria-label={`PDF preview of ${doc.displayTitle || doc.name}`}
                    />
                  </TabsContent>
                ))}
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
