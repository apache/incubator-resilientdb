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

import { ActionIcon, Tooltip, Paper, Text, Group, Box, Modal, Textarea, Button, Stack, Divider, Avatar } from '@mantine/core';
import { IconBrain, IconSend, IconRobot, IconUser } from '@tabler/icons-react';
import { useState, useEffect, ReactNode, useCallback } from 'react';
import ReactMarkdown from 'react-markdown';
import { Prism as SyntaxHighlighter } from 'react-syntax-highlighter';
import { vscDarkPlus } from 'react-syntax-highlighter/dist/cjs/styles/prism';

interface CodeProps {
  node?: any;
  inline?: boolean;
  className?: string;
  children?: ReactNode;
  [key: string]: any;
}

interface MarkdownComponentProps {
  children: ReactNode;
}

export function FloatingAssistant() {
  const [isOpen, setIsOpen] = useState(false);
  const [selectedText, setSelectedText] = useState('');
  const [explanation, setExplanation] = useState('');
  const [isLoading, setIsLoading] = useState(false);
  const [question, setQuestion] = useState('');
  const [isAskingQuestion, setIsAskingQuestion] = useState(false);
  const [debounceTimer, setDebounceTimer] = useState<NodeJS.Timeout | null>(null);
  const [lastSelection, setLastSelection] = useState('');

  // Function to check if element is within a code editor
  const isWithinCodeEditor = (element: Element | null): boolean => {
    if (!element) return false;
    
    // Check if element is within a code editor
    if (element.closest('.cm-editor') || 
        element.closest('.CodeMirror') || 
        element.closest('pre') || 
        element.closest('code')) {
      return true;
    }
    
    // Check parent elements
    return isWithinCodeEditor(element.parentElement);
  };

  // Debounced text selection handler
  const handleTextSelection = useCallback(() => {
    // Clear any existing timer
    if (debounceTimer) {
      clearTimeout(debounceTimer);
    }

    // Set new timer
    const timer = setTimeout(() => {
      const selection = window.getSelection();
      const selectedText = selection?.toString().trim() || '';
      
      // Don't trigger if:
      // 1. No text is selected
      // 2. Selection is within a code editor
      // 3. Selection is the same as last time
      if (!selectedText || 
          isWithinCodeEditor(selection?.anchorNode?.parentElement as Element | null) ||
          selectedText === lastSelection) {
        return;
      }

      setLastSelection(selectedText);
      setSelectedText(selectedText);
      setIsOpen(true);
      getExplanation(selectedText);
    }, 300); // 300ms debounce delay

    setDebounceTimer(timer);
  }, [debounceTimer, lastSelection]);

  // Cleanup timer on unmount
  useEffect(() => {
    return () => {
      if (debounceTimer) {
        clearTimeout(debounceTimer);
      }
    };
  }, [debounceTimer]);

  // Add event listener for text selection
  useEffect(() => {
    document.addEventListener('mouseup', handleTextSelection);
    return () => {
      document.removeEventListener('mouseup', handleTextSelection);
    };
  }, [handleTextSelection]);

  // Reset lastSelection when modal is closed
  const handleModalClose = () => {
    setIsOpen(false);
    setSelectedText('');
    setExplanation('');
    setQuestion('');
    setLastSelection('');
  };

  const getExplanation = async (text: string, customQuestion?: string) => {
    setIsLoading(true);
    try {
      const response = await fetch('/api/chat', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          messages: [
            {
              role: 'user',
              content: customQuestion 
                ? (selectedText 
                    ? `Regarding this text: "${selectedText}"\n\n${customQuestion}`
                    : customQuestion)
                : `Explain the following text or concept in detail:
${text}

Please:
1. Break down any complex concepts
2. Provide examples if relevant
3. Explain any technical terms
4. Use markdown formatting for better readability`
            }
          ]
        }),
      });

      if (!response.ok) throw new Error('Failed to get explanation');
      
      const reader = response.body?.getReader();
      if (!reader) throw new Error('No reader available');

      let explanation = '';
      while (true) {
        const { done, value } = await reader.read();
        if (done) break;
        
        const text = new TextDecoder().decode(value);
        explanation += text;
        setExplanation(explanation);
      }
    } catch (error) {
      console.error('Error getting explanation:', error);
      setExplanation('Sorry, there was an error getting the explanation.');
    } finally {
      setIsLoading(false);
      setIsAskingQuestion(false);
    }
  };

  const handleAskQuestion = (e: React.FormEvent) => {
    e.preventDefault();
    if (!question.trim()) return;
    
    setIsAskingQuestion(true);
    getExplanation(selectedText || '', question);
    setQuestion('');
  };

  return (
    <>
      <Tooltip label="AI Assistant - Select text to get explanations or ask general questions">
        <ActionIcon
          variant="filled"
          color="blue"
          size="xl"
          radius="xl"
          style={{
            position: 'fixed',
            bottom: '20px',
            right: '20px',
            zIndex: 1000,
            boxShadow: '0 4px 12px rgba(0, 0, 0, 0.15)',
          }}
          onClick={() => setIsOpen(true)}
        >
          <IconBrain size={20} />
        </ActionIcon>
      </Tooltip>

      <Modal
        opened={isOpen}
        onClose={handleModalClose}
        size="60vw"
        styles={{
          content: {
            display: 'flex',
            flexDirection: 'column',
            height: '80vh',
            padding: 0,
          },
          body: {
            flex: 1,
            padding: 0,
            display: 'flex',
            flexDirection: 'column',
          },
          header: {
            padding: '1rem',
            borderBottom: '1px solid var(--mantine-color-dark-4)',
            backgroundColor: 'var(--mantine-color-dark-7)',
          },
        }}
        title={
          <Group justify="space-between" w="100%">
            <Group>
              <Avatar color="blue" radius="xl">
                <IconRobot size={20} />
              </Avatar>
              <Text size="sm" fw={500}>AI Assistant</Text>
            </Group>
            {selectedText && (
              <Text size="xs" c="dimmed" style={{ maxWidth: '70%' }}>
                Selected: "{selectedText.length > 50 ? selectedText.substring(0, 50) + '...' : selectedText}"
              </Text>
            )}
          </Group>
        }
      >
        <Stack style={{ flex: 1, height: '100%' }} p={0}>
          {selectedText && (
            <Paper p="md" bg="dark.6" style={{ borderBottom: '1px solid var(--mantine-color-dark-4)' }}>
              <Group>
                <Avatar color="gray" radius="xl">
                  <IconUser size={16} />
                </Avatar>
                <Box>
                  <Text size="sm" fw={500} mb={4}>Selected Text</Text>
                  <Text size="sm" c="dimmed">{selectedText}</Text>
                </Box>
              </Group>
            </Paper>
          )}
          
          <Box 
            style={{ 
              flex: 1,
              overflowY: 'auto',
              padding: '1rem',
              display: 'flex',
              flexDirection: 'column',
              gap: '1rem',
            }}
          >
            {isLoading ? (
              <Group>
                <Avatar color="blue" radius="xl">
                  <IconRobot size={20} />
                </Avatar>
                <Paper p="md" bg="dark.7" style={{ maxWidth: '80%' }}>
                  <Text size="sm" c="dimmed">Generating response...</Text>
                </Paper>
              </Group>
            ) : explanation ? (
              <Group align="flex-start">
                <Avatar color="blue" radius="xl">
                  <IconRobot size={20} />
                </Avatar>
                <Paper p="md" bg="dark.7" style={{ maxWidth: '80%' }}>
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
                      p: ({ children }: MarkdownComponentProps) => (
                        <Text size="sm" mb="md" style={{ whiteSpace: 'pre-wrap' }}>
                          {children}
                        </Text>
                      ),
                      ul: ({ children }: MarkdownComponentProps) => (
                        <Box component="ul" mb="md" style={{ paddingLeft: '1.5rem' }}>
                          {children}
                        </Box>
                      ),
                      ol: ({ children }: MarkdownComponentProps) => (
                        <Box component="ol" mb="md" style={{ paddingLeft: '1.5rem' }}>
                          {children}
                        </Box>
                      ),
                      li: ({ children }: MarkdownComponentProps) => (
                        <Text size="sm" component="li" mb="xs">
                          {children}
                        </Text>
                      ),
                      h1: ({ children }: MarkdownComponentProps) => (
                        <Text size="xl" fw={700} mb="md">
                          {children}
                        </Text>
                      ),
                      h2: ({ children }: MarkdownComponentProps) => (
                        <Text size="lg" fw={600} mb="md">
                          {children}
                        </Text>
                      ),
                      h3: ({ children }: MarkdownComponentProps) => (
                        <Text size="md" fw={600} mb="md">
                          {children}
                        </Text>
                      ),
                      blockquote: ({ children }: MarkdownComponentProps) => (
                        <Paper
                          withBorder
                          p="xs"
                          mb="md"
                          bg="dark.6"
                          style={{ borderLeft: '4px solid var(--mantine-color-blue-6)' }}
                        >
                          {children}
                        </Paper>
                      ),
                    }}
                  >
                    {explanation}
                  </ReactMarkdown>
                </Paper>
              </Group>
            ) : (
              <Group>
                <Avatar color="blue" radius="xl">
                  <IconRobot size={20} />
                </Avatar>
                <Paper p="md" bg="dark.7" style={{ maxWidth: '80%' }}>
                  <Text size="sm" c="dimmed">
                    {selectedText 
                      ? "Select text to get an explanation" 
                      : "Ask any question or select text to get an explanation"}
                  </Text>
                </Paper>
              </Group>
            )}
          </Box>

          <Paper 
            p="md" 
            style={{ 
              borderTop: '1px solid var(--mantine-color-dark-4)',
              backgroundColor: 'var(--mantine-color-dark-7)',
            }}
          >
            <form onSubmit={handleAskQuestion}>
              <Group>
                <Textarea
                  placeholder={selectedText 
                    ? "Ask a specific question about the selected text..." 
                    : "Ask any question..."}
                  value={question}
                  onChange={(e: React.ChangeEvent<HTMLTextAreaElement>) => setQuestion(e.target.value)}
                  style={{ flex: 1 }}
                  disabled={isLoading}
                  autosize
                  minRows={1}
                  maxRows={3}
                  styles={{
                    input: {
                      backgroundColor: 'var(--mantine-color-dark-6)',
                      borderColor: 'var(--mantine-color-dark-4)',
                    },
                  }}
                />
                <Button 
                  type="submit" 
                  loading={isAskingQuestion}
                  disabled={!question.trim()}
                  leftSection={<IconSend size={14} />}
                >
                  Send
                </Button>
              </Group>
            </form>
          </Paper>
        </Stack>
      </Modal>
    </>
  );
} 