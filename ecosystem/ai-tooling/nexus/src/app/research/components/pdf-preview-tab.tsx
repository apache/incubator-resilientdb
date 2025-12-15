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

import { TabsContent, TabsTrigger } from "@/components/ui/tabs";
import { Document } from "@/hooks/useDocuments";
import { FileText } from "lucide-react";

interface PDFPreviewTabProps {
  selectedDocuments: Document[];
}

export const PDFPreviewTabs: React.FC<PDFPreviewTabProps> = ({
  selectedDocuments,
}) => {
  

  return (
    <>
      {/* PDF Document Tab Triggers */}
      {selectedDocuments.map((doc) => (
        <TabsTrigger
          key={doc.id}
          value={doc.id}
          className="flex items-center gap-2 text-xs max-w-[175px] relative group"
          title={doc.displayTitle || doc.name}
        >
          <FileText className="h-3 w-3 flex-shrink-0" />
          <span className="truncate">{doc.displayTitle || doc.name}</span>
        </TabsTrigger>
      ))}
    </>
  );
};

export const PDFPreviewContent: React.FC<PDFPreviewTabProps> = ({
  selectedDocuments,
}) => {
    

  return (
    <>
      {/* PDF Document Tab Content */}
      {selectedDocuments.map((doc) => (
        <TabsContent
          key={doc.id}
          value={doc.id}
          className="h-full m-0 p-0 data-[state=active]:flex data-[state=inactive]:hidden"
          forceMount
        >
          <iframe
            src={doc.webViewLink}
            width="100%"
            height="100%"
            className="w-full h-full border-0"
            title={`Preview of ${doc.name}`}
            aria-label={`PDF preview of ${doc.displayTitle || doc.name}`}
            loading="lazy"
          />
        </TabsContent>
      ))}
    </>
  );
};
