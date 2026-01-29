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

import { clsx, type ClassValue } from "clsx";
import { twMerge } from "tailwind-merge";
import { TOOL_CALL_MAPPINGS } from "./constants";

export function cn(...inputs: ClassValue[]) {
  return twMerge(clsx(inputs));
}

export function cleanUpImplementation(implementation: string) {
  implementation = implementation.replace(/__CODE_COMPOSER_META__[\s\S]*$/, '').trim();
  
  // remove closing explanation after code blocks (starts after final ``` and contains explanatory text)
  const lastCodeBlockEnd = implementation.lastIndexOf('```');
  if (lastCodeBlockEnd !== -1) {
    const afterCodeBlock = implementation.substring(lastCodeBlockEnd + 3).trim();
    // if there's substantial explanatory text after the code block, remove it
    if (afterCodeBlock.length > 50 && afterCodeBlock.includes('This implementation')) {
      implementation = implementation.substring(0, lastCodeBlockEnd + 3).trim();
    }
  }
  // remove any trailing markdown code block markers
  implementation = implementation.replace(/```\s*$/, '').trim();

  return implementation;
} 

export type ToolState = "input-streaming" | "input-available" | "output-available" | "output-error";

export function formatToolHeader(toolType: string, state: ToolState, docCount?: number) {
  const mapping = TOOL_CALL_MAPPINGS[toolType];
  const base = mapping
    ? state === "input-available"
      ? mapping.input
      : state === "output-available"
        ? mapping.output
        : mapping.error
    : toolType;

  if (typeof docCount === "number" && toolType === "search_documents") {
    const plural = docCount === 1 ? "document" : "documents";
    return `${base} ${docCount} ${plural}`;
  }
  return base;
}