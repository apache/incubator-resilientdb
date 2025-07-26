"use client";

import { Badge } from "@/components/ui/badge";
import { Button } from "@/components/ui/button";
import { Card, CardContent } from "@/components/ui/card";
import { Label } from "@/components/ui/label";
import { Loader } from "@/components/ui/loader";
import { ScrollArea } from "@/components/ui/scroll-area";
import {
  Tooltip,
  TooltipContent,
  TooltipTrigger,
} from "@/components/ui/tooltip";
import { type Document } from "@/hooks/useDocuments";
import { Check, FileText, Search, X } from "lucide-react";
import { memo, useCallback, useMemo, useState } from "react";

interface MultiDocumentSelectorProps {
  className?: string;
  documents: Document[];
  selectedDocuments: Document[];
  isLoadingDocuments: boolean;
  onDocumentSelect: (doc: Document) => void;
  onDocumentDeselect: (doc: Document) => void;
  onSelectAll: () => void;
  onDeselectAll: () => void;
  onDocumentKeyDown: (e: React.KeyboardEvent, doc: Document) => void;
  maxSelections?: number;
  showSearch?: boolean;
  showSelectAll?: boolean;
  emptyMessage?: string;
}

const MultiDocumentSelector = memo<MultiDocumentSelectorProps>(
  ({
    className = "",
    documents,
    selectedDocuments,
    isLoadingDocuments,
    onDocumentSelect,
    onDocumentDeselect,
    onSelectAll,
    onDeselectAll,
    onDocumentKeyDown,
    maxSelections,
    showSearch = true,
    showSelectAll = true,
    emptyMessage = "No documents found",
  }) => {
    const [searchQuery, setSearchQuery] = useState("");

    // Filter documents based on search query
    const filteredDocuments = useMemo(() => {
      if (!searchQuery.trim()) return documents;

      const query = searchQuery.toLowerCase();
      return documents.filter(
        (doc) =>
          doc.name.toLowerCase().includes(query) ||
          (doc.displayTitle && doc.displayTitle.toLowerCase().includes(query)),
      );
    }, [documents, searchQuery]);

    // Check if document is selected
    const isDocumentSelected = useCallback(
      (doc: Document) => {
        return selectedDocuments.some((selected) => selected.id === doc.id);
      },
      [selectedDocuments],
    );

    // Handle document toggle
    const handleDocumentToggle = useCallback(
      (doc: Document) => {
        if (isDocumentSelected(doc)) {
          onDocumentDeselect(doc);
        } else {
          // Check if we've reached the maximum selections
          if (maxSelections && selectedDocuments.length >= maxSelections) {
            return; // Don't select more documents
          }
          onDocumentSelect(doc);
        }
      },
      [
        isDocumentSelected,
        onDocumentSelect,
        onDocumentDeselect,
        maxSelections,
        selectedDocuments.length,
      ],
    );

    // Handle select all toggle
    const handleSelectAllToggle = useCallback(() => {
      if (selectedDocuments.length === documents.length) {
        onDeselectAll();
      } else {
        onSelectAll();
      }
    }, [
      selectedDocuments.length,
      documents.length,
      onSelectAll,
      onDeselectAll,
    ]);

    // Calculate selection stats
    const selectionStats = useMemo(() => {
      const selected = selectedDocuments.length;
      const total = documents.length;
      const filtered = filteredDocuments.length;
      const allSelected = selected === total && total > 0;
      const someSelected = selected > 0 && selected < total;

      return { selected, total, filtered, allSelected, someSelected };
    }, [selectedDocuments.length, documents.length, filteredDocuments.length]);

    if (isLoadingDocuments) {
      return (
        <div className={`space-y-3 ${className}`}>
          <div className="flex justify-center py-6">
            <Loader />
          </div>
        </div>
      );
    }

    if (documents.length === 0) {
      return (
        <div className={`space-y-3 ${className}`}>
          <Card className="text-center py-6 border-dashed">
            <CardContent className="pt-6">
              <FileText className="h-12 w-12 mx-auto mb-2 text-muted-foreground" />
              <p className="text-sm text-muted-foreground">{emptyMessage}</p>
            </CardContent>
          </Card>
        </div>
      );
    }

    return (
      <div className={`space-y-3 ${className}`}>
        {/* Header with search and select all */}
        <div className="space-y-2">
          <div className="flex items-center justify-between">
            <Label className="text-xs font-medium text-muted-foreground uppercase tracking-wide">
              Available Documents
            </Label>
            <div className="text-xs text-muted-foreground">
              {selectionStats.selected} of {selectionStats.total} selected
            </div>
          </div>

          {/* Search input */}
          {showSearch && (
            <div className="relative">
              <Search className="absolute left-3 top-1/2 transform -translate-y-1/2 h-4 w-4 text-muted-foreground" />
              <input
                type="text"
                placeholder="Search documents..."
                value={searchQuery}
                onChange={(e) => setSearchQuery(e.target.value)}
                className="w-full pl-10 pr-4 py-2 text-sm border rounded-md bg-background focus:outline-none focus:ring-2 focus:ring-ring focus:border-transparent"
                aria-label="Search documents"
              />
              {searchQuery && (
                <button
                  onClick={() => setSearchQuery("")}
                  className="absolute right-3 top-1/2 transform -translate-y-1/2 text-muted-foreground hover:text-foreground"
                  aria-label="Clear search"
                >
                  <X className="h-4 w-4" />
                </button>
              )}
            </div>
          )}

          {/* Select All button */}
          {showSelectAll && (
            <Button
              variant="outline"
              size="sm"
              onClick={handleSelectAllToggle}
              disabled={documents.length === 0}
              className="w-full"
              aria-label={
                selectionStats.allSelected
                  ? "Deselect all documents"
                  : "Select all documents"
              }
            >
              {selectionStats.allSelected ? (
                <>
                  <X className="h-4 w-4 mr-2" />
                  Deselect All
                </>
              ) : (
                <>
                  <Check className="h-4 w-4 mr-2" />
                  Select All {maxSelections ? `(max ${maxSelections})` : ""}
                </>
              )}
            </Button>
          )}
        </div>

        {/* Document list */}
        <ScrollArea className="h-[calc(100vh-320px)]">
          <div className="space-y-2">
            {filteredDocuments.map((doc) => {
              const isSelected = isDocumentSelected(doc);
              const isDisabled =
                !isSelected &&
                maxSelections &&
                selectedDocuments.length >= maxSelections;

              return (
                <Card
                  key={doc.id}
                  variant="message"
                  className={`cursor-pointer transition-all w-full ${
                    isSelected
                      ? "bg-primary/10 border-primary/20 ring-1 ring-primary/20"
                      : isDisabled
                        ? "opacity-50 cursor-not-allowed"
                        : "hover:bg-accent/50 hover:border-border/60"
                  }`}
                  onClick={() => !isDisabled && handleDocumentToggle(doc)}
                  onKeyDown={(e) => !isDisabled && onDocumentKeyDown(e, doc)}
                  tabIndex={isDisabled ? -1 : 0}
                  role="checkbox"
                  aria-checked={isSelected}
                  aria-label={`${isSelected ? "Deselect" : "Select"} document: ${doc.displayTitle || doc.name}`}
                >
                  <CardContent
                    variant="message-compact"
                    className="w-full px-3"
                  >
                    <div className="flex items-start space-x-3 w-full">
                      {/* Checkbox */}
                      <div className="flex-shrink-0 mt-0.5">
                        <div
                          className={`w-4 h-4 rounded border-2 flex items-center justify-center transition-colors ${
                            isSelected
                              ? "bg-primary border-primary text-primary-foreground"
                              : isDisabled
                                ? "border-muted-foreground/30"
                                : "border-muted-foreground hover:border-primary"
                          }`}
                        >
                          {isSelected && <Check className="h-3 w-3" />}
                        </div>
                      </div>

                      {/* Document info */}
                      <div className="flex items-start space-x-1.5 flex-1 min-w-0">
                        <FileText className="h-4 w-4 mt-0.5 text-muted-foreground flex-shrink-0" />
                        <div className="flex-1 min-w-0 overflow-hidden">
                          <Tooltip>
                            <TooltipTrigger asChild>
                              <p className="text-sm font-medium break-words leading-tight max-h-[2.4em] overflow-hidden text-ellipsis line-clamp-2">
                                {doc.displayTitle || doc.name}
                              </p>
                            </TooltipTrigger>
                            <TooltipContent side="right">
                              <p className="max-w-xs break-words">
                                {doc.displayTitle || doc.name}
                              </p>
                            </TooltipContent>
                          </Tooltip>
                          <div className="flex flex-wrap items-center gap-1 mt-2">
                            <Badge
                              variant="secondary"
                              className="text-xs truncate max-w-[120px]"
                            >
                              {doc.name}
                            </Badge>
                            <Badge
                              variant="outline"
                              className="text-xs flex-shrink-0"
                            >
                              {(doc.size / 1024 / 1024).toFixed(1)} MB
                            </Badge>
                            {isSelected && (
                              <Badge variant="default" className="text-xs">
                                Selected
                              </Badge>
                            )}
                          </div>
                        </div>
                      </div>
                    </div>
                  </CardContent>
                </Card>
              );
            })}

            {/* No search results */}
            {searchQuery && filteredDocuments.length === 0 && (
              <Card className="text-center py-6 border-dashed">
                <CardContent className="pt-6">
                  <Search className="h-8 w-8 mx-auto mb-2 text-muted-foreground" />
                  <p className="text-sm text-muted-foreground">
                    No documents found for "{searchQuery}"
                  </p>
                  <Button
                    variant="ghost"
                    size="sm"
                    onClick={() => setSearchQuery("")}
                    className="mt-2"
                  >
                    Clear search
                  </Button>
                </CardContent>
              </Card>
            )}
          </div>
        </ScrollArea>

        {/* Selection limit warning */}
        {maxSelections && selectedDocuments.length >= maxSelections && (
          <div className="text-xs text-muted-foreground bg-muted/50 p-2 rounded">
            Maximum of {maxSelections} documents can be selected at once.
          </div>
        )}
      </div>
    );
  },
);

MultiDocumentSelector.displayName = "MultiDocumentSelector";

export { MultiDocumentSelector };
export type { MultiDocumentSelectorProps };
