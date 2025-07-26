"use client";

import { cn } from "@/lib/utils";
import React from "react";
import ReactMarkdown from "react-markdown";
import rehypeHighlight from "rehype-highlight";
import remarkGfm from "remark-gfm";

interface MarkdownRendererProps {
  content: string;
  className?: string;
}

export const MarkdownRenderer: React.FC<MarkdownRendererProps> = ({
  content,
  className,
}) => {
  return (
    <div className={cn("prose prose-invert w-full", className)}>
      <ReactMarkdown
        remarkPlugins={[remarkGfm]}
        rehypePlugins={[rehypeHighlight]}
        components={{
          // Custom styling for different markdown elements
          h1: ({ children }) => (
            <h1 className="text-2xl font-bold mb-4 text-white">{children}</h1>
          ),
          h2: ({ children }) => (
            <h2 className="text-xl font-semibold mb-3 text-white">
              {children}
            </h2>
          ),
          h3: ({ children }) => (
            <h3 className="text-lg font-medium mb-2 text-white">{children}</h3>
          ),
          p: ({ children }) => (
            <p className="mb-3 text-gray-200 leading-relaxed">{children}</p>
          ),
          hr: ({ children }) => <hr className="my-4" />,
          ul: ({ children }) => (
            <ul className="list-disc list-inside mb-3 text-gray-200 space-y-1">
              {children}
            </ul>
          ),
          ol: ({ children }) => (
            <ol className="list-decimal mb-3 ml-6 text-gray-200 space-y-1">
              {children}
            </ol>
          ),
          li: ({ children }) => <li className="text-gray-200">{children}</li>,
          blockquote: ({ children }) => (
            <blockquote className="border-l-4 border-gray-500 pl-4 italic text-gray-300 mb-3">
              {children}
            </blockquote>
          ),
          code: ({ inline, className, children, ...props }: any) => {
            return inline ? (
              <code
                className="bg-gray-700 px-1.5 py-0.5 rounded text-sm font-mono text-gray-200 break-words"
                {...props}
              >
                {children}
              </code>
            ) : (
              <code className={cn("break-words whitespace-pre-wrap", className)} {...props}>
                {children}
              </code>
            );
          },
          pre: ({ children }) => (
            <pre className="bg-gray-800 rounded-lg p-4 mb-3 text-gray-200 w-full max-w-full overflow-hidden whitespace-pre-wrap break-words">
              {children}
            </pre>
          ),
          a: ({ href, children }) => (
            <a
              href={href}
              className="text-blue-400 hover:text-blue-300 underline transition-colors"
              target="_blank"
              rel="noopener noreferrer"
            >
              {children}
            </a>
          ),
          table: ({ children }) => (
            <div className="overflow-x-auto mb-3">
              <table className="min-w-full border-collapse border border-gray-600">
                {children}
              </table>
            </div>
          ),
          th: ({ children }) => (
            <th className="border border-gray-600 px-3 py-2 bg-gray-700 text-white font-semibold text-left">
              {children}
            </th>
          ),
          td: ({ children }) => (
            <td className="border border-gray-600 px-3 py-2 text-gray-200">
              {children}
            </td>
          ),
          strong: ({ children }) => (
            <strong className="font-bold text-white">{children}</strong>
          ),
          em: ({ children }) => (
            <em className="italic text-gray-200">{children}</em>
          ),
        }}
      >
        {content}
      </ReactMarkdown>
    </div>
  );
};
