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
