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

'use client';

/* TODO BUILD THIS OUT */

import { useEffect, useState, useRef } from 'react';
import { javascript } from '@codemirror/lang-javascript';
import { IconPlayerPlay, IconTemplate, IconTrash, IconMessage, IconPlayerStop, IconMaximize, IconMinimize } from '@tabler/icons-react';
import { vscodeDark } from '@uiw/codemirror-theme-vscode';
import CodeMirror from '@uiw/react-codemirror';
import { Box, Button, Group, Paper, Select, Stack, Text, Tooltip, Textarea, ActionIcon, Modal } from '@mantine/core';
import ReactMarkdown from 'react-markdown';
import { Prism as SyntaxHighlighter } from 'react-syntax-highlighter';
import { vscDarkPlus } from 'react-syntax-highlighter/dist/cjs/styles/prism';

interface Template {
  value: string;
  label: string;
  code: string;
}

interface Message {
  role: 'user' | 'assistant';
  content: string;
}

interface CodeProps {
  node?: any;
  inline?: boolean;
  className?: string;
  children?: React.ReactNode;
  [key: string]: any;
}

const EXAMPLE_TEMPLATES = [
  {
    value: 'default',
    label: 'Welcome - Start Here',
    code: `/**
 * Welcome to the TypeScript Playground!
 * 
 * This interactive environment allows you to test and experiment with TypeScript code.
 * The code here runs in a sandboxed environment using the TypeScript compiler.
 * 
 * Available Features:
 * 1. TypeScript Type Checking
 * 2. Code Execution
 * 3. Error Reporting
 * 4. AI Assistant Support
 * 
 * Choose an example from the dropdown menu above to get started!
 */

// Example: Basic TypeScript Interface
interface User {
  id: number;
  name: string;
  email: string;
  isActive: boolean;
}

// Example: Function with Type Safety
function greetUser(user: User): string {
  return \`Hello, \${user.name}! Your email is \${user.email}\`;
}

// Example: Using the Interface
const user: User = {
  id: 1,
  name: "John Doe",
  email: "john@example.com",
  isActive: true
};

console.log(greetUser(user));`,
  },
  {
    value: 'async_await',
    label: 'Async/Await Example',
    code: `/**
 * Example: Using async/await with TypeScript
 */

// Define types for our API response
interface Todo {
  id: number;
  title: string;
  completed: boolean;
}

// Async function to fetch todos
async function fetchTodos(): Promise<Todo[]> {
  try {
    const response = await fetch('https://jsonplaceholder.typicode.com/todos');
    const data = await response.json();
    return data as Todo[];
  } catch (error) {
    console.error('Error fetching todos:', error);
    return [];
  }
}

// Main function to demonstrate async/await
async function main() {
  console.log('Fetching todos...');
  const todos = await fetchTodos();
  console.log('First 3 todos:', todos.slice(0, 3));
}

// Run the example
main();`,
  },
  {
    value: 'generics',
    label: 'Generics Example',
    code: `/**
 * Example: Using TypeScript Generics
 */

// Generic function to create an array of a specific type
function createArray<T>(length: number, value: T): T[] {
  return Array(length).fill(value);
}

// Generic class for a simple stack
class Stack<T> {
  private items: T[] = [];

  push(item: T): void {
    this.items.push(item);
  }

  pop(): T | undefined {
    return this.items.pop();
  }

  peek(): T | undefined {
    return this.items[this.items.length - 1];
  }

  isEmpty(): boolean {
    return this.items.length === 0;
  }
}

// Example usage
const numberStack = new Stack<number>();
numberStack.push(1);
numberStack.push(2);
numberStack.push(3);

console.log('Stack operations:');
console.log('Pop:', numberStack.pop());
console.log('Peek:', numberStack.peek());
console.log('Is empty:', numberStack.isEmpty());

// Using the generic array creator
const stringArray = createArray<string>(3, 'hello');
console.log('String array:', stringArray);`,
  },
  {
    value: 'decorators',
    label: 'Decorators Example',
    code: `/**
 * Example: Using TypeScript Decorators
 * Note: Decorators are an experimental feature
 */

// Simple method decorator
function log(target: any, propertyKey: string, descriptor: PropertyDescriptor) {
  const originalMethod = descriptor.value;

  descriptor.value = function (...args: any[]) {
    console.log(\`Calling \${propertyKey} with args:\`, args);
    const result = originalMethod.apply(this, args);
    console.log(\`\${propertyKey} returned:\`, result);
    return result;
  };

  return descriptor;
}

// Class using the decorator
class Calculator {
  @log
  add(a: number, b: number): number {
    return a + b;
  }

  @log
  multiply(a: number, b: number): number {
    return a * b;
  }
}

// Example usage
const calc = new Calculator();
console.log('Calculator operations:');
calc.add(5, 3);
calc.multiply(4, 2);`,
  },
];

export function TypeScriptPlayground({ templates }: { templates?: Template[] }) {
  const codeTemplates = templates && Array.isArray(templates) ? templates : EXAMPLE_TEMPLATES;
  const [code, setCode] = useState<string>(codeTemplates[0].code);
  const [output, setOutput] = useState<string[]>(['Initializing TypeScript environment...']);
  const [isLoading, setIsLoading] = useState(true);
  const [isRunning, setIsRunning] = useState(false);
  const [showChat, setShowChat] = useState(false);
  const [messages, setMessages] = useState<Message[]>([]);
  const [input, setInput] = useState('');
  const [isAiLoading, setIsAiLoading] = useState(false);
  const [abortController, setAbortController] = useState<AbortController | null>(null);
  const [isExpanded, setIsExpanded] = useState(false);
  const expandTriggerRef = useRef<HTMLButtonElement | null>(null);
  const lastFocusedRef = useRef<HTMLElement | null>(null);

  const openExpanded = (from?: HTMLElement | null) => {
    lastFocusedRef.current = from ?? (document.activeElement as HTMLElement | null);
    setIsExpanded(true);
    // focus textarea inside modal after open
    setTimeout(() => {
      const ta = document.querySelector('.cm-theme') as HTMLElement | null;
      ta?.focus?.();
    }, 50);
  };

  const closeExpanded = () => {
    setIsExpanded(false);
    setTimeout(() => {
      try { lastFocusedRef.current?.focus?.(); } catch (err) {}
    }, 0);
  };

  useEffect(() => {
    if (!isExpanded) return;
    const onKey = (e: KeyboardEvent) => {
      if (e.key === 'Escape') {
        e.preventDefault();
        closeExpanded();
      }
    };
    document.addEventListener('keydown', onKey);
    return () => document.removeEventListener('keydown', onKey);
  }, [isExpanded]);

  useEffect(() => {
    // Initialize TypeScript environment
    const initTypeScript = async () => {
      try {
        // Load TypeScript compiler
        const script = document.createElement('script');
        script.src = 'https://unpkg.com/typescript@latest/lib/typescript.js';
        document.head.appendChild(script);

        await new Promise((resolve) => {
          script.onload = resolve;
        });

        setOutput(['✅ TypeScript environment ready']);
        setIsLoading(false);
      } catch (error) {
        setOutput((prev) => [
          ...prev,
          `❌ Error: ${error instanceof Error ? error.message : 'Unknown error'}`,
        ]);
        setIsLoading(false);
      }
    };

    initTypeScript();
  }, []);

  const runCode = async () => {
    if (isLoading || isRunning) {
      return;
    }
    setIsRunning(true);
    setOutput(['▶ Running code...']);

    try {
      // Compile TypeScript to JavaScript
      const jsCode = (window as any).ts.transpile(code, {
        target: 'ES2020',
        module: 'ES2020',
        strict: true,
        esModuleInterop: true,
        skipLibCheck: true,
        forceConsistentCasingInFileNames: true,
      });

      // Create a sandboxed environment with better console handling
      const sandbox = `
        const originalConsole = console;
        const console = {
          log: (...args) => {
            window.parent.postMessage({ type: 'log', args: args.map(arg => 
              typeof arg === 'object' ? JSON.stringify(arg, null, 2) : String(arg)
            )}, '*');
            originalConsole.log(...args);
          },
          error: (...args) => {
            window.parent.postMessage({ type: 'error', args: args.map(arg => 
              typeof arg === 'object' ? JSON.stringify(arg, null, 2) : String(arg)
            )}, '*');
            originalConsole.error(...args);
          },
          warn: (...args) => {
            window.parent.postMessage({ type: 'log', args: args.map(arg => 
              typeof arg === 'object' ? JSON.stringify(arg, null, 2) : String(arg)
            )}, '*');
            originalConsole.warn(...args);
          },
          info: (...args) => {
            window.parent.postMessage({ type: 'log', args: args.map(arg => 
              typeof arg === 'object' ? JSON.stringify(arg, null, 2) : String(arg)
            )}, '*');
            originalConsole.info(...args);
          }
        };

        // Add error handling for uncaught errors
        window.onerror = (message, source, lineno, colno, error) => {
          console.error(\`Error: \${message}\nLine: \${lineno}, Column: \${colno}\`);
          return true;
        };

        // Add promise rejection handling
        window.onunhandledrejection = (event) => {
          console.error(\`Unhandled Promise Rejection: \${event.reason}\`);
        };

        try {
          ${jsCode}
        } catch (error) {
          console.error(error.message);
        }
      `;

      // Create an iframe for sandboxed execution
      const iframe = document.createElement('iframe');
      iframe.style.display = 'none';
      document.body.appendChild(iframe);

      // Handle messages from the sandbox
      const messageHandler = (event: MessageEvent) => {
        if (event.data.type === 'log') {
          setOutput(prev => [...prev, ...event.data.args]);
        } else if (event.data.type === 'error') {
          setOutput(prev => [...prev, `❌ Error: ${event.data.args.join(' ')}`]);
        }
      };

      window.addEventListener('message', messageHandler);

      // Write the sandboxed code to the iframe
      if (iframe.contentWindow) {
        iframe.contentWindow.document.open();
        iframe.contentWindow.document.write(`
          <!DOCTYPE html>
          <html>
            <head>
              <script>
                ${sandbox}
              </script>
            </head>
            <body></body>
          </html>
        `);
        iframe.contentWindow.document.close();
      }

      // Wait for any pending console output before cleanup
      await new Promise(resolve => setTimeout(resolve, 100));

      // Cleanup
      window.removeEventListener('message', messageHandler);
      document.body.removeChild(iframe);
      setOutput(prev => [...prev, '✅ Code execution completed']);
      setIsRunning(false);
    } catch (error: any) {
      setOutput((prev) => [...prev, `❌ Error: ${error.message}`]);
      setIsRunning(false);
    }
  };

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    if (!input.trim()) return;

    const userMessage: Message = { role: 'user', content: input };
    setMessages(prev => [...prev, userMessage]);
    setInput('');
    setIsAiLoading(true);

    const controller = new AbortController();
    setAbortController(controller);

    try {
      const response = await fetch('/api/chat', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ messages: [...messages, userMessage], code }),
        signal: controller.signal
      });

      if (!response.ok) throw new Error('Failed to get response');
      
      const reader = response.body?.getReader();
      if (!reader) throw new Error('No reader available');

      let assistantMessage = '';
      while (true) {
        const { done, value } = await reader.read();
        if (done) break;
        
        const text = new TextDecoder().decode(value);
        assistantMessage += text;
        setMessages(prev => {
          const newMessages = [...prev];
          const lastMessage = newMessages[newMessages.length - 1];
          if (lastMessage.role === 'assistant') {
            lastMessage.content = assistantMessage;
          } else {
            newMessages.push({ role: 'assistant', content: assistantMessage });
          }
          return newMessages;
        });
      }
    } catch (error) {
      if (error instanceof Error && error.name === 'AbortError') {
        setMessages(prev => [...prev, { role: 'assistant', content: 'Response generation stopped.' }]);
      } else {
        console.error('Chat error:', error);
        setMessages(prev => [...prev, { role: 'assistant', content: 'Sorry, there was an error processing your request.' }]);
      }
    } finally {
      setIsAiLoading(false);
      setAbortController(null);
    }
  };

  const handleKeyDown = (e: React.KeyboardEvent<HTMLTextAreaElement>) => {
    if (e.key === 'Enter' && !e.shiftKey) {
      e.preventDefault();
      if (input.trim() && !isAiLoading) {
        handleSubmit(e as unknown as React.FormEvent);
      }
    }
  };

  const stopGeneration = () => {
    if (abortController) {
      abortController.abort();
      setAbortController(null);
    }
  };

  const clearOutput = () => setOutput([]);

  const clearConversation = () => {
    setMessages([]);
  };

  return (
    <Paper withBorder p="md" radius="md">
      <Stack>
        <Group justify="space-between" align="center">
          <Group>
            <Tooltip label="Load example">
              <Select
                size="xs"
                placeholder="Load example"
                data={codeTemplates}
                onChange={(value) =>
                  value && setCode(codeTemplates.find((t) => t.value === value)?.code || '')
                }
                leftSection={<IconTemplate size={14} />}
              />
            </Tooltip>
            <Tooltip label="Ask AI Assistant">
              <ActionIcon 
                variant="light" 
                onClick={() => setShowChat(!showChat)}
                color={showChat ? "blue" : "gray"}
              >
                <IconMessage size={14} />
              </ActionIcon>
            </Tooltip>
          </Group>
        </Group>

        <Box>
          <CodeMirror
            value={code}
            height="400px"
            theme={vscodeDark}
            extensions={[javascript({ typescript: true })]}
            onChange={setCode}
            editable={!isLoading}
            basicSetup={{
              lineNumbers: true,
              highlightActiveLineGutter: true,
              highlightSpecialChars: true,
              history: true,
              foldGutter: true,
              drawSelection: true,
              dropCursor: true,
              allowMultipleSelections: true,
              indentOnInput: true,
              syntaxHighlighting: true,
              bracketMatching: true,
              closeBrackets: true,
              autocompletion: true,
              rectangularSelection: true,
              crosshairCursor: true,
              highlightActiveLine: true,
              highlightSelectionMatches: true,
              closeBracketsKeymap: true,
              defaultKeymap: true,
              searchKeymap: true,
              historyKeymap: true,
              foldKeymap: true,
              completionKeymap: true,
              lintKeymap: true,
            }}
          />
        </Box>

        {showChat && (
          <>
            <Paper withBorder p="md" radius="md">
              <Stack>
                <Group justify="space-between" align="center">
                  <Text size="sm" fw={500}>AI Assistant</Text>
                  <Group>
                    <Tooltip label={isExpanded ? "Minimize" : "Expand"}>
                      <ActionIcon 
                        ref={expandTriggerRef}
                        variant="light" 
                        onClick={(e) => openExpanded(e.currentTarget)}
                        onKeyDown={(e) => {
                          if (e.key === 'Enter' || e.key === ' ') {
                            e.preventDefault();
                            openExpanded(e.currentTarget as HTMLElement);
                          }
                        }}
                        disabled={isExpanded}
                      >
                        <IconMaximize size={14} />
                      </ActionIcon>
                    </Tooltip>
                    <Tooltip label="Clear conversation">
                      <ActionIcon 
                        variant="light" 
                        color="red" 
                        onClick={clearConversation}
                        disabled={messages.length === 0}
                      >
                        <IconTrash size={14} />
                      </ActionIcon>
                    </Tooltip>
                  </Group>
                </Group>
                <Box style={{ maxHeight: '200px', overflowY: 'auto' }}>
                  {[...messages].reverse().map((message, i) => (
                    <Paper key={i} p="xs" mb="xs" bg={message.role === 'user' ? 'dark.6' : 'dark.7'}>
                      <Text size="sm" c={message.role === 'user' ? 'blue' : 'white'}>
                        {message.role === 'user' ? (
                          message.content
                        ) : (
                          <ReactMarkdown
                            components={{
                              code({ node, inline, className, children, ...props }: CodeProps) {
                                const match = /language-(\w+)/.exec(className || '');
                                return !inline && match ? (
                                  <SyntaxHighlighter
                                    style={vscDarkPlus}
                                    language={match[1]}
                                    PreTag="div"
                                    {...props}
                                  >
                                    {String(children).replace(/\n$/, '')}
                                  </SyntaxHighlighter>
                                ) : (
                                  <code className={className} {...props}>
                                    {children}
                                  </code>
                                );
                              },
                            }}
                          >
                            {message.content}
                          </ReactMarkdown>
                        )}
                      </Text>
                    </Paper>
                  ))}
                </Box>
                <form onSubmit={handleSubmit}>
                  <Group>
                    <Textarea
                      placeholder="Ask about your code... (Press Enter to send, Shift+Enter for new line)"
                      value={input}
                      onChange={(e) => setInput(e.target.value)}
                      onKeyDown={handleKeyDown}
                      style={{ flex: 1 }}
                      disabled={isAiLoading}
                      autosize
                      minRows={1}
                      maxRows={4}
                    />
                    {isAiLoading ? (
                      <Button 
                        color="red" 
                        onClick={stopGeneration}
                        leftSection={<IconPlayerStop size={14} />}
                      >
                        Stop
                      </Button>
                    ) : (
                      <Button type="submit">
                        Send
                      </Button>
                    )}
                  </Group>
                </form>
              </Stack>
            </Paper>

            <Modal
              opened={isExpanded}
              onClose={() => closeExpanded()}
              size="xl"
              title={
                <Group justify="space-between" w="100%">
                  <Text size="sm" fw={500}>AI Assistant</Text>
                  <Group>
                    <Tooltip label="Minimize">
                      <ActionIcon 
                          variant="light" 
                          onClick={() => closeExpanded()}
                        >
                        <IconMinimize size={14} />
                      </ActionIcon>
                    </Tooltip>
                    <Tooltip label="Clear conversation">
                      <ActionIcon 
                        variant="light" 
                        color="red" 
                        onClick={clearConversation}
                        disabled={messages.length === 0}
                      >
                        <IconTrash size={14} />
                      </ActionIcon>
                    </Tooltip>
                  </Group>
                </Group>
              }
              styles={{
                title: {
                  width: '100%',
                  marginRight: 0,
                },
                body: {
                  height: 'calc(100vh - 200px)',
                  display: 'flex',
                  flexDirection: 'column',
                },
              }}
            >
              <Stack style={{ flex: 1, height: '100%' }}>
                <Box style={{ flex: 1, overflowY: 'auto' }}>
                  {[...messages].reverse().map((message, i) => (
                    <Paper key={i} p="xs" mb="xs" bg={message.role === 'user' ? 'dark.6' : 'dark.7'}>
                      <Text size="sm" c={message.role === 'user' ? 'blue' : 'white'}>
                        {message.role === 'user' ? (
                          message.content
                        ) : (
                          <ReactMarkdown
                            components={{
                              code({ node, inline, className, children, ...props }: CodeProps) {
                                const match = /language-(\w+)/.exec(className || '');
                                return !inline && match ? (
                                  <SyntaxHighlighter
                                    style={vscDarkPlus}
                                    language={match[1]}
                                    PreTag="div"
                                    {...props}
                                  >
                                    {String(children).replace(/\n$/, '')}
                                  </SyntaxHighlighter>
                                ) : (
                                  <code className={className} {...props}>
                                    {children}
                                  </code>
                                );
                              },
                            }}
                          >
                            {message.content}
                          </ReactMarkdown>
                        )}
                      </Text>
                    </Paper>
                  ))}
                </Box>
                <form onSubmit={handleSubmit}>
                  <Group>
                    <Textarea
                      placeholder="Ask about your code... (Press Enter to send, Shift+Enter for new line)"
                      value={input}
                      onChange={(e) => setInput(e.target.value)}
                      onKeyDown={handleKeyDown}
                      style={{ flex: 1 }}
                      disabled={isAiLoading}
                      autosize
                      minRows={1}
                      maxRows={4}
                    />
                    {isAiLoading ? (
                      <Button 
                        color="red" 
                        onClick={stopGeneration}
                        leftSection={<IconPlayerStop size={14} />}
                      >
                        Stop
                      </Button>
                    ) : (
                      <Button type="submit">
                        Send
                      </Button>
                    )}
                  </Group>
                </form>
              </Stack>
            </Modal>
          </>
        )}

        <Group justify="space-between">
          <Button
            onClick={runCode}
            loading={isRunning}
            disabled={isLoading}
            color="blue"
            leftSection={<IconPlayerPlay size={14} />}
          >
            {isRunning ? 'Running...' : 'Run'}
          </Button>
          <Button
            onClick={clearOutput}
            variant="light"
            disabled={isLoading || output.length === 0}
            leftSection={<IconTrash size={14} />}
          >
            Clear Output
          </Button>
        </Group>

        <Paper
          withBorder
          p="xs"
          style={{
            backgroundColor: '#1e1e1e',
            color: '#33ff00',
            fontFamily: 'monospace',
            height: '200px',
            overflow: 'auto',
          }}
        >
          {output.map((line, i) => (
            <Text key={i} style={{ whiteSpace: 'pre-wrap', color: 'inherit' }}>
              {line}
            </Text>
          ))}
        </Paper>
      </Stack>
    </Paper>
  );
} 