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
          className="flex items-center gap-2 text-xs max-w-[150px] relative group"
          title={doc.displayTitle || doc.name}
        >
          <FileText className="h-3 w-3 flex-shrink-0" />
          <span className="truncate">
            {doc.displayTitle || doc.name}
          </span>
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
            src={`/api/research/files/${doc.path}#toolbar=0&navpanes=0&scrollbar=1`}
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