"use client";

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

import { Badge } from "@/components/ui/badge";
import {
  Tooltip,
  TooltipContent,
  TooltipTrigger,
} from "@/components/ui/tooltip";
import { ExternalLink, FileText } from "lucide-react";
import { memo, useCallback } from "react";

interface DocumentSource {
  path: string;
  name: string;
  displayTitle?: string;
  pages?: number[];
}

interface DocumentSourceBadgeProps {
  source: DocumentSource;
  variant?: "default" | "secondary" | "outline";
  size?: "sm" | "md";
  showIcon?: boolean;
  showTooltip?: boolean;
  clickable?: boolean;
  onClick?: (source: DocumentSource) => void;
  className?: string;
}

const DocumentSourceBadge = memo<DocumentSourceBadgeProps>(
  ({
    source,
    variant = "secondary",
    size = "sm",
    showIcon = true,
    showTooltip = true,
    clickable = false,
    onClick,
    className = "",
  }) => {
    const handleClick = useCallback(() => {
      if (clickable && onClick) {
        onClick(source);
      }
    }, [clickable, onClick, source]);

    const handleKeyDown = useCallback(
      (e: React.KeyboardEvent) => {
        if (clickable && onClick && (e.key === "Enter" || e.key === " ")) {
          e.preventDefault();
          onClick(source);
        }
      },
      [clickable, onClick, source],
    );

    const displayName = source.displayTitle || source.name;
    const shortName =
      displayName.length > 20
        ? displayName.substring(0, 20) + "..."
        : displayName;

    const badgeContent = (
      <Badge
        variant={variant}
        className={`text-xs ${size === "sm" ? "px-1.5 py-0.5" : "px-2 py-1"} ${
          clickable
            ? "cursor-pointer hover:bg-primary/20 transition-colors"
            : ""
        } ${className}`}
        onClick={clickable ? handleClick : undefined}
        onKeyDown={clickable ? handleKeyDown : undefined}
        tabIndex={clickable ? 0 : undefined}
        role={clickable ? "button" : undefined}
        aria-label={clickable ? `View ${displayName}` : undefined}
      >
        {showIcon && (
          <FileText
            className={`${size === "sm" ? "h-2.5 w-2.5" : "h-3 w-3"} mr-1`}
          />
        )}
        <span className="truncate">{shortName}</span>
        {clickable && (
          <ExternalLink
            className={`${size === "sm" ? "h-2.5 w-2.5" : "h-3 w-3"} ml-1 opacity-60`}
          />
        )}
      </Badge>
    );

    if (showTooltip) {
      return (
        <Tooltip>
          <TooltipTrigger asChild>{badgeContent}</TooltipTrigger>
          <TooltipContent className="w-64">
            <div className="text-center">
              <p className="font-medium">{displayName}</p>
              <p className="text-xs text-muted-foreground mt-1">
                {source.name}
              </p>
              {source.pages && source.pages.length > 0 && (
                <p className="text-xs text-muted-foreground mt-1">
                  {source.pages.length} pages ({source.pages.sort((a, b) => a - b).join(", ")})
                </p>
              )}
              {clickable && (
                <p className="text-xs text-muted-foreground mt-1">
                  Click to view document
                </p>
              )}
            </div>
          </TooltipContent>
        </Tooltip>
      );
    }

    return badgeContent;
  },
);

DocumentSourceBadge.displayName = "DocumentSourceBadge";

// Multi-source badge component for showing multiple sources
interface MultiDocumentSourceBadgeProps {
  sources: DocumentSource[];
  maxVisible?: number;
  variant?: "default" | "secondary" | "outline";
  size?: "sm" | "md";
  showIcon?: boolean;
  clickable?: boolean;
  onSourceClick?: (source: DocumentSource) => void;
  className?: string;
}

const MultiDocumentSourceBadge = memo<MultiDocumentSourceBadgeProps>(
  ({
    sources,
    maxVisible = 3,
    variant = "secondary",
    size = "sm",
    showIcon = true,
    clickable = false,
    onSourceClick,
    className = "",
  }) => {
    const visibleSources = sources.slice(0, maxVisible);
    const hiddenCount = sources.length - maxVisible;

    if (sources.length === 0) {
      return null;
    }

    return (
      <div className={`flex flex-wrap gap-1 ${className}`}>
        {visibleSources.map((source, index) => (
          <DocumentSourceBadge
            key={`${source.path}-${index}`}
            source={source}
            variant={variant}
            size={size}
            showIcon={showIcon && index === 0} // Only show icon on first badge
            showTooltip={true}
            clickable={clickable}
            onClick={onSourceClick}
          />
        ))}
        {hiddenCount > 0 && (
          <Tooltip>
            <TooltipTrigger asChild>
              <Badge
                variant="outline"
                className={`text-xs ${size === "sm" ? "px-1.5 py-0.5" : "px-2 py-1"}`}
              >
                +{hiddenCount}
              </Badge>
            </TooltipTrigger>
            <TooltipContent>
              <div className="space-y-1">
                <p className="font-medium">Additional sources:</p>
                {sources.slice(maxVisible).map((source, index) => (
                 <>
                  <p key={index} className="text-xs text-muted-foreground">
                    {source.displayTitle || source.name}
                  </p>
                  </>
                ))}
              </div>
            </TooltipContent>
          </Tooltip>
        )}
      </div>
    );
  },
);

MultiDocumentSourceBadge.displayName = "MultiDocumentSourceBadge";

// Source attribution component for chat messages
interface SourceAttributionProps {
  sources: DocumentSource[];
  className?: string;
  showLabel?: boolean;
  clickable?: boolean;
  onSourceClick?: (source: DocumentSource) => void;
}

const SourceAttribution = memo<SourceAttributionProps>(
  ({
    sources,
    className = "",
    showLabel = true,
    clickable = false,
    onSourceClick,
  }) => {
    if (sources.length === 0) {
      return null;
    }

    return (
      <div
        className={`flex items-center gap-2 text-xs text-muted-foreground ${className}`}
      >
        {showLabel && (
          <span className="font-medium">
            {sources.length === 1 ? "Source:" : "Sources:"}
          </span>
        )}
        <MultiDocumentSourceBadge
          sources={sources}
          maxVisible={3}
          variant="outline"
          size="sm"
          showIcon={true}
          clickable={clickable}
          onSourceClick={onSourceClick}
        />
      </div>
    );
  },
);

SourceAttribution.displayName = "SourceAttribution";

export { DocumentSourceBadge, MultiDocumentSourceBadge, SourceAttribution };
export type {
  DocumentSource,
  DocumentSourceBadgeProps,
  MultiDocumentSourceBadgeProps,
  SourceAttributionProps
};

