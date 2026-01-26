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

import { useTool } from "@/components/context/ToolContext";
import { Button } from "@/components/ui/button";
import { CardContent } from "@/components/ui/card";
import { Label } from "@/components/ui/label";
import { Loader } from "@/components/ui/loader";
import {
  Select,
  SelectContent,
  SelectItem,
  SelectTrigger,
  SelectValue,
} from "@/components/ui/select";
import { Separator } from "@/components/ui/separator";
import { Switch } from "@/components/ui/switch";
import { Textarea } from "@/components/ui/textarea";
import { Document } from "@/hooks/useDocuments";
import { Send } from "lucide-react";
import { useState } from "react";

export type Language = "ts" | "python" | "cpp";

interface ChatInputProps {
  inputValue: string;
  setInputValue: (value: string) => void;
  onSendMessage: (payload: {
    query: string;
    documentPaths: string[];
    tool?: string;
    language?: Language;
    scope?: string[];
  }) => void;
  isLoading: boolean;
  isPreparingIndex: boolean;
  selectedDocuments: Document[];
  onKeyDown: (e: React.KeyboardEvent<HTMLTextAreaElement>) => void;
}

export const ChatInput: React.FC<ChatInputProps> = ({
  inputValue,
  setInputValue,
  onSendMessage,
  isLoading,
  isPreparingIndex,
  selectedDocuments,
  onKeyDown,
}) => {
  const { activeTool, setTool } = useTool();
  const [language, setLanguage] = useState<Language>("ts");

  const handleSendMessage = () => {
    if (
      !inputValue.trim() ||
      selectedDocuments.length === 0 ||
      isLoading ||
      isPreparingIndex
    ) {
      return;
    }


    const payload = {
      query: inputValue,
      documentPaths: selectedDocuments.map((doc) => doc.path),
      ...(activeTool === "code-composer" && {
        tool: "code-composer",
        language,
        scope: [], // TODO: Implement scope collection
      }),
    };

    onSendMessage(payload);
  };

  const handleToolChange = (tool: string) => {
    if (tool === "code-composer") {
      setTool("code-composer");
    } else {
      setTool("default");
    }
  };

  const getPlaceholder = () => {
    if (isPreparingIndex) return "Preparing documents...";
    if (selectedDocuments.length === 0)
      return "Select documents to start chatting...";
    if (activeTool === "code-composer")
      return "Draft code from selected papers...";
    if (selectedDocuments.length === 1) {
      return `Ask questions about ${selectedDocuments[0].displayTitle || selectedDocuments[0].name}...`;
    }
    return `Ask questions about ${selectedDocuments.length} documents...`;
  };

  return (

    <div className="border-t flex-shrink-0">
      <CardContent className="p-4" role="form" aria-label="Send message">
        <div className="flex space-x-2">
          <div className="flex-1 space-y-2">
            <Label htmlFor="message-input" className="sr-only">
              Type your message about the document
            </Label>
            <Textarea
              id="message-input"
              value={inputValue}
              onChange={(e) => setInputValue(e.target.value)}
              onKeyDown={onKeyDown}
              placeholder={getPlaceholder()}
              className="resize-none"
              rows={2}
              disabled={
                isLoading || isPreparingIndex || selectedDocuments.length === 0
              }
              aria-describedby="message-input-help"
            />
            <p id="message-input-help" className="sr-only">
              Press Enter to send your message, or Shift+Enter for a new line
            </p>
            <div className="flex items-center justify-between">
              <div className="flex items-center space-x-2">
                <div className="flex items-center space-x-2">
                  <Switch
                    checked={activeTool === "code-composer"}
                    onCheckedChange={(checked) =>
                      handleToolChange(checked ? "code-composer" : "default")
                    }
                    aria-label="Toggle code composer"
                  />
                  <Label
                    htmlFor="code-composer-switch"
                    className="text-sm flex items-center gap-2"
                  >
                    Code Composer
                  </Label>
                </div>
                {activeTool === "code-composer" && (
                  <>
                    <Separator
                      orientation="vertical"
                      className="!h-4 bg-gray-400"
                    />
                    <Label
                      htmlFor="language-select"
                      className="text-sm text-gray-400"
                    >
                      Language:
                    </Label>
                    <Select
                      value={language}
                      onValueChange={(value: Language) => setLanguage(value)}
                    >
                      <SelectTrigger className="w-32 !h-8 text-sm" size="sm">
                        <SelectValue />
                      </SelectTrigger>
                      <SelectContent>
                        <SelectItem value="ts">TypeScript</SelectItem>
                        <SelectItem value="python">Python</SelectItem>
                        <SelectItem value="cpp">C++</SelectItem>
                      </SelectContent>
                    </Select>
                  </>
                )}
              </div>
            </div>
          </div>
          <Button
            onClick={handleSendMessage}
            disabled={
              !inputValue.trim() ||
              isLoading ||
              isPreparingIndex ||
              selectedDocuments.length === 0
            }
            className="px-4"
            size="lg"
            aria-label="Send message"
          >
            {isLoading || isPreparingIndex ? (
              <Loader size="sm" aria-label="Sending..." />
            ) : (
              <Send className="h-4 w-4" aria-hidden="true" />
            )}
          </Button>
        </div>
      </CardContent>
    </div>
  );
};
