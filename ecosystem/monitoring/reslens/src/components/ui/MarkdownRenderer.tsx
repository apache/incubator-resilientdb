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
 *
 */

/* eslint-disable @typescript-eslint/no-unused-vars */
import React from "react";
import ReactMarkdown from "react-markdown";
import remarkGfm from "remark-gfm";
import rehypeRaw from "rehype-raw";
import { cn } from "@/lib/utils";
import { Tabs, TabsContent, TabsList, TabsTrigger } from "@/components/ui/tabs";

interface TechnicalMarkdownRendererProps {
  markdown: string;
  className?: string;
  title?: string;
}

export function TechnicalMarkdownRenderer({
  markdown,
  className,
  title = "AI Flamegraph Analysis",
}: TechnicalMarkdownRendererProps) {
  const headings: { level: number; text: string; id: string }[] = [];
  const lines = markdown.split("\n");
  lines.forEach((line) => {
    const headingMatch = line.match(/^(#{1,6})\s+(.+)$/);
    if (headingMatch) {
      const level = headingMatch[1].length;
      const text = headingMatch[2];
      const id = text
        .toLowerCase()
        .replace(/[^\w\s]/g, "")
        .replace(/\s+/g, "-");
      headings.push({ level, text, id });
    }
  });

  return (
    <div
      className={cn(
        "bg-slate-900 text-slate-50 rounded-lg overflow-hidden transition-all duration-300",
        "border border-slate-800 shadow-lg",
        className
      )}
    >
      <div className="sticky top-0 z-10 bg-slate-900 border-b border-slate-800 p-4 flex items-center justify-between">
        <h2 className="text-xl font-semibold text-slate-50">{title}</h2>
      </div>

      <Tabs defaultValue="content" className="w-full">
        {/* <TabsList className="bg-slate-800 mx-4 mt-4">
          <TabsTrigger value="content">Content</TabsTrigger>
          <TabsTrigger value="toc">Table of Contents</TabsTrigger>
        </TabsList> */}
        <TabsContent value="content" className="p-4 md:p-6 pt-2">
          <div className="prose prose-invert prose-slate max-w-none">
            <ReactMarkdown
              remarkPlugins={[remarkGfm]}
              rehypePlugins={[rehypeRaw]}
              components={{
                h1: ({ node, ...props }) => {
                  const id = props.children
                    ? props.children
                        .toString()
                        .toLowerCase()
                        .replace(/[^\w\s]/g, "")
                        .replace(/\s+/g, "-")
                    : "";
                  return (
                    <h1
                      id={id}
                      className="text-2xl font-bold mt-8 mb-4 pb-2 border-b border-slate-700"
                      {...props}
                    />
                  );
                },
                h2: ({ node, ...props }) => {
                  const id = props.children
                    ? props.children
                        .toString()
                        .toLowerCase()
                        .replace(/[^\w\s]/g, "")
                        .replace(/\s+/g, "-")
                    : "";
                  return (
                    <h2
                      id={id}
                      className="text-xl font-semibold mt-6 mb-3"
                      {...props}
                    />
                  );
                },
                h3: ({ node, ...props }) => {
                  const id = props.children
                    ? props.children
                        .toString()
                        .toLowerCase()
                        .replace(/[^\w\s]/g, "")
                        .replace(/\s+/g, "-")
                    : "";
                  return (
                    <h3
                      id={id}
                      className="text-lg font-medium mt-5 mb-2"
                      {...props}
                    />
                  );
                },
                p: ({ node, ...props }) => (
                  <p className="my-3 leading-relaxed" {...props} />
                ),
                ul: ({ node, ...props }) => (
                  <ul className="my-3 pl-6 list-disc" {...props} />
                ),
                ol: ({ node, ...props }) => (
                  <ol className="my-3 pl-6 list-decimal" {...props} />
                ),
                li: ({ node, ...props }) => <li className="my-1" {...props} />,
                blockquote: ({ node, ...props }) => (
                  <blockquote
                    className="border-l-4 border-slate-600 pl-4 italic my-4"
                    {...props}
                  />
                ),
                code: ({ node, className, children, ...props }) => {
                  const match = /language-(\w+)/.exec(className || "");
                  const language = match ? match[1] : "text";

                  return (
                    <code
                      className="bg-slate-800 text-slate-200 px-1 py-0.5 rounded text-sm font-mono"
                      {...props}
                    >
                      {children}
                    </code>
                  );
                },
                strong: ({ node, ...props }) => (
                  <strong className="font-bold text-slate-100" {...props} />
                ),
                em: ({ node, ...props }) => (
                  <em className="italic text-slate-300" {...props} />
                ),
                a: ({ node, ...props }) => (
                  <a
                    className="text-blue-400 hover:text-blue-300 underline underline-offset-2"
                    target="_blank"
                    rel="noopener noreferrer"
                    {...props}
                  />
                ),
                table: ({ node, ...props }) => (
                  <div className="overflow-x-auto my-4">
                    <table className="min-w-full border-collapse" {...props} />
                  </div>
                ),
                thead: ({ node, ...props }) => (
                  <thead className="bg-slate-800" {...props} />
                ),
                tbody: ({ node, ...props }) => (
                  <tbody className="divide-y divide-slate-800" {...props} />
                ),
                tr: ({ node, ...props }) => (
                  <tr className="border-b border-slate-800" {...props} />
                ),
                th: ({ node, ...props }) => (
                  <th
                    className="px-4 py-2 text-left text-sm font-medium text-slate-300"
                    {...props}
                  />
                ),
                td: ({ node, ...props }) => (
                  <td className="px-4 py-2 text-sm" {...props} />
                ),
              }}
            >
              {markdown}
            </ReactMarkdown>
          </div>
        </TabsContent>
      </Tabs>
    </div>
  );
}
