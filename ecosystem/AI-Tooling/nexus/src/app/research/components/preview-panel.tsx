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

import {
  Card,
  CardContent,
  CardDescription,
  CardTitle,
} from "@/components/ui/card";
import { Tabs, TabsList } from "@/components/ui/tabs";
import { Document } from "@/hooks/useDocuments";
import { FileText } from "lucide-react";
import { useEffect, useRef, useState } from "react";
import { CodeGeneration } from "../types";
import {
  CodeGenerationContent,
  CodeGenerationTabs,
} from "./code-generation-tab";
import { PDFPreviewContent, PDFPreviewTabs } from "./pdf-preview-tab";

interface PreviewPanelProps {
  selectedDocuments: Document[];
  codeGenerations?: CodeGeneration[];
  className?: string;
  onCloseCodeGeneration?: (generationId: string) => void;
}

const PreviewPanel: React.FC<PreviewPanelProps> = ({
  selectedDocuments,
  codeGenerations = [],
  className = "",
  onCloseCodeGeneration,
}) => {
  const hasCodeGenerations = codeGenerations.length > 0;
  const hasPdfs = selectedDocuments.length > 0;

  const [activeTab, setActiveTab] = useState<string>("");
  const previousCodeGenerationsRef = useRef<CodeGeneration[]>([]);
  const userHasInteractedRef = useRef(false);

  const handleTabChange = (value: string) => {
    userHasInteractedRef.current = true;
    setActiveTab(value);
  };

  const handleCloseCodeGeneration = (generationId: string) => {
    // If we're closing the currently active tab, switch to another tab
    if (activeTab === `code-${generationId}`) {
      const remainingGenerations = codeGenerations.filter(
        (gen) => gen.id !== generationId,
      );
      if (remainingGenerations.length > 0) {
        // Switch to the most recent remaining generation
        setActiveTab(
          `code-${remainingGenerations[remainingGenerations.length - 1].id}`,
        );
      } else if (selectedDocuments.length > 0) {
        // Switch to first PDF document if available
        setActiveTab(selectedDocuments[0].id);
      }
    }

    // Call the parent handler to actually remove the generation
    onCloseCodeGeneration?.(generationId);
  };

  useEffect(() => {
    if (!activeTab || (!hasCodeGenerations && !hasPdfs)) {
      let defaultTab: string | undefined;
      
      if (hasCodeGenerations) {
        const streamingGen = codeGenerations.find((gen) => gen.isStreaming);
        if (streamingGen) {
          defaultTab = `code-${streamingGen.id}`;
        } else {
          defaultTab = `code-${codeGenerations[codeGenerations.length - 1].id}`;
        }
      } else {
        defaultTab = selectedDocuments[0]?.id;
      }
      
      if (defaultTab) {
        setActiveTab(defaultTab);
        userHasInteractedRef.current = false;
      }
    }
  }, [hasCodeGenerations, hasPdfs, activeTab, codeGenerations, selectedDocuments]);

  useEffect(() => {
    const previousGenerations = previousCodeGenerationsRef.current;
    const newGenerations = codeGenerations.filter(
      (gen) => !previousGenerations.find((prev) => prev.id === gen.id),
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

  const getLanguageLabel = (language: string) => {
    const labels: Record<string, string> = {
      ts: "TypeScript",
      cpp: "C++",
      python: "Python",
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
            <CardTitle className="text-lg truncate px-3 pt-2 pb-0">
              {hasCodeGenerations && hasPdfs
                ? "Preview & Code"
                : hasCodeGenerations
                  ? "Code Generations"
                  : "PDF Preview"}
            </CardTitle>
          <CardContent className="flex-1 p-0 min-h-0 w-full min-w-0">
            <Tabs
              value={activeTab}
              onValueChange={handleTabChange}
              className="h-full flex flex-col w-full min-w-0"
            >
              <div className="px-2 pt-2 pb-0">
                <TabsList className="w-full justify-start overflow-x-auto">
                  {/* Code Generation Tabs */}
                  <CodeGenerationTabs
                    codeGenerations={codeGenerations}
                    getLanguageLabel={getLanguageLabel}
                    onCloseTab={handleCloseCodeGeneration}
                  />

                  {/* PDF Document Tabs */}
                  <PDFPreviewTabs selectedDocuments={selectedDocuments} />
                </TabsList>
              </div>

              <div className="flex-1 min-h-0 w-full min-w-0">
                {/* Code Generation Content */}
                <CodeGenerationContent
                  codeGenerations={codeGenerations}
                  getLanguageLabel={getLanguageLabel}
                />

                {/* PDF Document Content */}
                <PDFPreviewContent selectedDocuments={selectedDocuments} />
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
