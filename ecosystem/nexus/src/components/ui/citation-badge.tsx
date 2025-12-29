"use client";

import { Badge } from "@/components/ui/badge";
import {
  Tooltip,
  TooltipContent,
  TooltipTrigger,
} from "@/components/ui/tooltip";

export interface CitationData {
  id: number;
  page: number;
  displayTitle: string;
}

interface CitationBadgeProps {
  id: number;
  data?: CitationData;
}

export const CitationBadge: React.FC<CitationBadgeProps> = ({ id, data }) => {
  if (!data) {
    return (
      <Badge
        variant="secondary"
        className="align-super mx-0.5 select-none text-[10px] px-1 py-0.5"
      >
        {id}
      </Badge>
    );
  }

  return (
    <Tooltip>
      <TooltipTrigger asChild>
        <Badge
          variant="secondary"
          className="align-super mx-0.5 cursor-help select-none text-[10px] px-1 py-0.5"
        >
          {id}
        </Badge>
      </TooltipTrigger>
      <TooltipContent className="w-64">
        <p className="font-medium mb-1">{data.displayTitle}</p>
        <p className="text-xs">Page {data.page}</p>
      </TooltipContent>
    </Tooltip>
  );
};
