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

import { useEffect, useState } from 'react';
import { python } from '@codemirror/lang-python';
import { IconPlayerPlay, IconTemplate, IconTrash, IconMessage, IconPlayerStop, IconMaximize, IconMinimize } from '@tabler/icons-react';
import { vscodeDark } from '@uiw/codemirror-theme-vscode';
import CodeMirror from '@uiw/react-codemirror';
import { Box, Button, Group, Paper, Select, Stack, Text, Tooltip, Textarea, ActionIcon, Modal } from '@mantine/core';
import ReactMarkdown from 'react-markdown';
import { Prism as SyntaxHighlighter } from 'react-syntax-highlighter';
import { vscDarkPlus } from 'react-syntax-highlighter/dist/cjs/styles/prism';

declare global {
  interface Window {
    loadPyodide: (options: { indexURL: string }) => Promise<any>;
    pyodide: any;
  }
}

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
    label: 'Instructions',
    code: `
      """
        Instructions
      """
    `,
  },
  {
    value: 'solution',
    label: 'Solution',
    code: `
      """
        Solution
      """
`,
  },
];

export function PythonPlayground({ templates }: { templates?: Template[] }) {
  const codeTemplates = templates && Array.isArray(templates) ? templates : EXAMPLE_TEMPLATES;
  const [code, setCode] = useState<string>(codeTemplates[0].code);
  const [output, setOutput] = useState<string[]>(['Initializing Python environment...']);
  const [isLoading, setIsLoading] = useState(true);
  const [isRunning, setIsRunning] = useState(false);
  const [showChat, setShowChat] = useState(false);
  const [messages, setMessages] = useState<Message[]>([]);
  const [input, setInput] = useState('');
  const [isAiLoading, setIsAiLoading] = useState(false);
  const [abortController, setAbortController] = useState<AbortController | null>(null);
  const [isExpanded, setIsExpanded] = useState(false);

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    if (!input.trim()) return;

    const userMessage: Message = { role: 'user', content: input };
    setMessages(prev => [...prev, userMessage]);
    setInput('');
    setIsAiLoading(true);

    // Create new AbortController for this request
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

  useEffect(() => {
    const initPython = async () => {
      try {
        if (window.pyodide) {
          setOutput(['✅ Python environment ready']);
          setIsLoading(false);
          return;
        }

        const script = document.createElement('script');
        script.src = 'https://cdn.jsdelivr.net/pyodide/v0.24.1/full/pyodide.js';
        document.head.appendChild(script);

        await new Promise((resolve) => {
          script.onload = resolve;
        });

        const pyodide = await window.loadPyodide({
          indexURL: 'https://cdn.jsdelivr.net/pyodide/v0.24.1/full/',
        });


        // Load and register our SDK
        const sdk = await fetch('/resdb_sdk.py').then((res) => res.text());
        await pyodide.runPythonAsync(sdk);

        // Register the SDK as a module
        await pyodide.runPythonAsync(`
import sys
import types
resdb_sdk = types.ModuleType('resdb_sdk')
resdb_sdk.__dict__.update(globals())
sys.modules['resdb_sdk'] = resdb_sdk
        `);

        await pyodide.runPythonAsync(`
          import sys
          from io import StringIO
          sys.stdout = StringIO()
          sys.stderr = StringIO()
        `);

        window.pyodide = pyodide;
        setOutput(['✅ Python environment ready']);
        setIsLoading(false);
      } catch (error) {
        setOutput((prev) => [
          ...prev,
          `❌ Error: ${error instanceof Error ? error.message : 'Unknown error'}`,
        ]);
        setIsLoading(false);
      }
    };

    initPython();
  }, []);

  const runCode = async () => {
    if (isLoading || isRunning) {
      return;
    }
    setIsRunning(true);
    setOutput(['▶ Running code...']);

    try {
      // Wrap code in async function if it uses await
      const codeToRun = /\bawait\b/.test(code)
        ? `
async def __run():
${code
  .split('\n')
  .map((line: string) => `    ${line}`)
  .join('\n')}

import asyncio
loop = asyncio.get_event_loop()
loop.run_until_complete(__run())
`
        : code;

      // Run the code and capture output
      await window.pyodide.runPythonAsync(codeToRun);

      // Get output
      const stdout = await window.pyodide.runPythonAsync('sys.stdout.getvalue()');
      if (stdout) {
        setOutput((prev) => [...prev, stdout]);
      }

      setOutput((prev) => [...prev, '✅ Code execution completed']);
    } catch (error: any) {
      setOutput((prev) => [...prev, `❌ Error: ${error.message}`]);
    } finally {
      // Clear buffers
      await window.pyodide.runPythonAsync('sys.stdout.truncate(0)\nsys.stdout.seek(0)');
      setIsRunning(false);
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
            extensions={[python()]}
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
                        variant="light" 
                        onClick={() => setIsExpanded(true)}
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
              onClose={() => setIsExpanded(false)}
              size="xl"
              title={
                <Group justify="space-between" w="100%">
                  <Text size="sm" fw={500}>AI Assistant</Text>
                  <Group>
                    <Tooltip label="Minimize">
                      <ActionIcon 
                        variant="light" 
                        onClick={() => setIsExpanded(false)}
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
