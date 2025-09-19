'use client';

import { ActionIcon, Tooltip, Paper, Text, Group, Box, Modal, Textarea, Button, Stack, Divider, Avatar, Switch, Badge } from '@mantine/core';
import { IconBrain, IconSend, IconRobot, IconUser, IconHighlight, IconSettings } from '@tabler/icons-react';
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
  const [highlightMode, setHighlightMode] = useState(true);
  const [showSettings, setShowSettings] = useState(false);

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
    // Only handle text selection if highlight mode is enabled
    if (!highlightMode) return;

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
  }, [debounceTimer, lastSelection, highlightMode]);

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
      <div
          style={{
            position: 'fixed',
            bottom: '20px',
            right: '20px',
            zIndex: 1000,
          display: 'flex',
          flexDirection: 'column',
          gap: 12,
          alignItems: 'flex-end',
        }}
      >
        {/* Settings Toggle */}
        <Tooltip 
          label="AI Assistant Settings"
          styles={{
            tooltip: {
              background: 'rgba(0,0,0,0.8)',
              backdropFilter: 'blur(8px)',
              border: '1px solid rgba(255,255,255,0.1)',
              color: 'rgba(255,255,255,0.9)',
              fontSize: '12px',
              fontWeight: 400,
            }
          }}
        >
          <ActionIcon
            variant="light"
            size="md"
            radius="xl"
            style={{
              background: 'rgba(255,255,255,0.05)',
              backdropFilter: 'blur(8px)',
              border: '1px solid rgba(255,255,255,0.1)',
              color: 'rgba(255,255,255,0.7)',
              transition: 'all 200ms ease',
            }}
            onClick={() => setShowSettings(!showSettings)}
            onMouseEnter={(e) => {
              e.currentTarget.style.background = 'rgba(0, 191, 255, 0.1)';
              e.currentTarget.style.borderColor = 'rgba(0, 191, 255, 0.3)';
              e.currentTarget.style.color = '#00bfff';
            }}
            onMouseLeave={(e) => {
              e.currentTarget.style.background = 'rgba(255,255,255,0.05)';
              e.currentTarget.style.borderColor = 'rgba(255,255,255,0.1)';
              e.currentTarget.style.color = 'rgba(255,255,255,0.7)';
            }}
          >
            <IconSettings size={16} />
        </ActionIcon>
      </Tooltip>

        {/* Settings Panel */}
        {showSettings && (
          <div
            style={{
              background: 'rgba(255,255,255,0.05)',
              backdropFilter: 'blur(12px)',
              border: '1px solid rgba(255,255,255,0.1)',
              borderRadius: 12,
              padding: 16,
              minWidth: 200,
              position: 'relative',
            }}
          >
            <div
              style={{
                position: 'absolute',
                top: 0,
                left: 0,
                right: 0,
                height: 1,
                background: 'linear-gradient(90deg, transparent, rgba(255,255,255,0.2), transparent)',
                borderRadius: '12px 12px 0 0',
              }}
            />
            <Stack gap={12}>
              <Group justify="space-between" align="center">
                <Group gap={8}>
                  <IconHighlight size={16} style={{ color: '#00bfff' }} />
                  <Text size="sm" c="rgba(255,255,255,0.9)" fw={500}>
                    Highlight Mode
                  </Text>
                </Group>
                <Switch
                  checked={highlightMode}
                  onChange={(event) => setHighlightMode(event.currentTarget.checked)}
                  size="sm"
                  styles={{
                    track: {
                      backgroundColor: highlightMode ? 'rgba(0, 191, 255, 0.3)' : 'rgba(255,255,255,0.1)',
                      borderColor: highlightMode ? 'rgba(0, 191, 255, 0.5)' : 'rgba(255,255,255,0.2)',
                    },
                    thumb: {
                      backgroundColor: highlightMode ? '#00bfff' : 'rgba(255,255,255,0.6)',
                    },
                  }}
                />
              </Group>
              <Text size="xs" c="rgba(255,255,255,0.6)" style={{ lineHeight: 1.4 }}>
                {highlightMode 
                  ? "Select text to automatically get explanations" 
                  : "Click the AI button to ask questions manually"}
              </Text>
              {highlightMode && (
                <Badge
                  size="xs"
                  variant="light"
                  color="blue"
                  style={{
                    background: 'rgba(0, 191, 255, 0.1)',
                    border: '1px solid rgba(0, 191, 255, 0.2)',
                    color: '#00bfff',
                  }}
                >
                  Active
                </Badge>
              )}
            </Stack>
          </div>
        )}

        {/* Main AI Assistant Button */}
        <Tooltip 
          label={highlightMode ? "AI Assistant - Select text to get explanations" : "AI Assistant - Click to ask questions"}
          styles={{
            tooltip: {
              background: 'rgba(0,0,0,0.8)',
              backdropFilter: 'blur(8px)',
              border: '1px solid rgba(255,255,255,0.1)',
              color: 'rgba(255,255,255,0.9)',
              fontSize: '12px',
              fontWeight: 400,
            }
          }}
        >
          <ActionIcon
            variant="light"
            size="xl"
            radius="xl"
            style={{
              background: 'rgba(0, 191, 255, 0.1)',
              backdropFilter: 'blur(12px)',
              border: '1px solid rgba(0, 191, 255, 0.2)',
              color: '#00bfff',
              transition: 'all 200ms ease',
              position: 'relative',
              overflow: 'hidden',
            }}
            onClick={() => setIsOpen(true)}
            onMouseEnter={(e) => {
              e.currentTarget.style.background = 'rgba(0, 191, 255, 0.2)';
              e.currentTarget.style.borderColor = 'rgba(0, 191, 255, 0.4)';
              e.currentTarget.style.transform = 'translateY(-2px)';
              e.currentTarget.style.boxShadow = '0 8px 20px rgba(0, 191, 255, 0.3)';
            }}
            onMouseLeave={(e) => {
              e.currentTarget.style.background = 'rgba(0, 191, 255, 0.1)';
              e.currentTarget.style.borderColor = 'rgba(0, 191, 255, 0.2)';
              e.currentTarget.style.transform = 'translateY(0)';
              e.currentTarget.style.boxShadow = 'none';
            }}
          >
            <div
              style={{
                position: 'absolute',
                top: 0,
                left: 0,
                right: 0,
                height: 1,
                background: 'linear-gradient(90deg, transparent, rgba(255,255,255,0.3), transparent)',
                borderRadius: '50% 50% 0 0',
              }}
            />
            <IconBrain size={20} style={{ position: 'relative', zIndex: 1 }} />
            {highlightMode && (
              <div
                style={{
                  position: 'absolute',
                  top: -2,
                  right: -2,
                  width: 8,
                  height: 8,
                  background: '#00ff88',
                  borderRadius: '50%',
                  border: '2px solid rgba(0,0,0,0.8)',
                }}
              />
            )}
          </ActionIcon>
        </Tooltip>
      </div>

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
            background: 'rgba(0,0,0,0.8)',
            backdropFilter: 'blur(20px)',
            border: '1px solid rgba(255,255,255,0.1)',
            borderRadius: 16,
          },
          body: {
            flex: 1,
            padding: 0,
            display: 'flex',
            flexDirection: 'column',
          },
          header: {
            padding: '20px',
            borderBottom: '1px solid rgba(255,255,255,0.1)',
            background: 'rgba(255,255,255,0.02)',
            backdropFilter: 'blur(8px)',
            borderRadius: '16px 16px 0 0',
          },
        }}
        title={
          <Group justify="space-between" w="100%">
            <Group gap={12}>
              <div
                style={{
                  width: 40,
                  height: 40,
                  borderRadius: '50%',
                  background: 'rgba(0, 191, 255, 0.1)',
                  border: '1px solid rgba(0, 191, 255, 0.2)',
                  display: 'flex',
                  alignItems: 'center',
                  justifyContent: 'center',
                }}
              >
                <IconRobot size={20} style={{ color: '#00bfff' }} />
              </div>
              <div>
                <Text size="sm" fw={500} c="rgba(255,255,255,0.9)">
                  AI Assistant
                </Text>
                <Text size="xs" c="rgba(255,255,255,0.6)">
                  {highlightMode ? "Highlight mode enabled" : "Manual mode"}
                </Text>
              </div>
            </Group>
            {selectedText && (
              <div
                style={{
                  background: 'rgba(0, 191, 255, 0.1)',
                  border: '1px solid rgba(0, 191, 255, 0.2)',
                  borderRadius: 8,
                  padding: '8px 12px',
                  maxWidth: '60%',
                }}
              >
                <Text size="xs" c="#00bfff" fw={500}>
                Selected: "{selectedText.length > 50 ? selectedText.substring(0, 50) + '...' : selectedText}"
              </Text>
              </div>
            )}
          </Group>
        }
      >
        <Stack style={{ flex: 1, height: '100%' }} p={0}>
          {selectedText && (
            <div
              style={{
                padding: '16px 20px',
                background: 'rgba(255,255,255,0.03)',
                borderBottom: '1px solid rgba(255,255,255,0.1)',
                backdropFilter: 'blur(8px)',
              }}
            >
              <Group gap={12}>
                <div
                  style={{
                    width: 32,
                    height: 32,
                    borderRadius: '50%',
                    background: 'rgba(255,255,255,0.05)',
                    border: '1px solid rgba(255,255,255,0.1)',
                    display: 'flex',
                    alignItems: 'center',
                    justifyContent: 'center',
                  }}
                >
                  <IconUser size={14} style={{ color: 'rgba(255,255,255,0.7)' }} />
                </div>
                <Box style={{ flex: 1 }}>
                  <Text size="sm" fw={500} mb={4} c="rgba(255,255,255,0.9)">
                    Selected Text
                  </Text>
                  <Text size="sm" c="rgba(255,255,255,0.7)" style={{ lineHeight: 1.5 }}>
                    {selectedText}
                  </Text>
                </Box>
              </Group>
            </div>
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
              <Group align="flex-start" gap={12}>
                <div
                  style={{
                    width: 32,
                    height: 32,
                    borderRadius: '50%',
                    background: 'rgba(0, 191, 255, 0.1)',
                    border: '1px solid rgba(0, 191, 255, 0.2)',
                    display: 'flex',
                    alignItems: 'center',
                    justifyContent: 'center',
                  }}
                >
                  <IconRobot size={16} style={{ color: '#00bfff' }} />
                </div>
                <div
                  style={{
                    background: 'rgba(255,255,255,0.05)',
                    border: '1px solid rgba(255,255,255,0.1)',
                    borderRadius: 12,
                    padding: 16,
                    maxWidth: '80%',
                    backdropFilter: 'blur(8px)',
                  }}
                >
                  <Text size="sm" c="rgba(255,255,255,0.7)">
                    Generating response...
                  </Text>
                </div>
              </Group>
            ) : explanation ? (
              <Group align="flex-start" gap={12}>
                <div
                  style={{
                    width: 32,
                    height: 32,
                    borderRadius: '50%',
                    background: 'rgba(0, 191, 255, 0.1)',
                    border: '1px solid rgba(0, 191, 255, 0.2)',
                    display: 'flex',
                    alignItems: 'center',
                    justifyContent: 'center',
                  }}
                >
                  <IconRobot size={16} style={{ color: '#00bfff' }} />
                </div>
                <div
                  style={{
                    background: 'rgba(255,255,255,0.05)',
                    border: '1px solid rgba(255,255,255,0.1)',
                    borderRadius: 12,
                    padding: 16,
                    maxWidth: '80%',
                    backdropFilter: 'blur(8px)',
                  }}
                >
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
                        <Text size="sm" mb="md" c="rgba(255,255,255,0.9)" style={{ whiteSpace: 'pre-wrap', lineHeight: 1.6 }}>
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
                        <Text size="sm" component="li" mb="xs" c="rgba(255,255,255,0.9)">
                          {children}
                        </Text>
                      ),
                      h1: ({ children }: MarkdownComponentProps) => (
                        <Text size="xl" fw={700} mb="md" c="rgba(255,255,255,0.9)">
                          {children}
                        </Text>
                      ),
                      h2: ({ children }: MarkdownComponentProps) => (
                        <Text size="lg" fw={600} mb="md" c="rgba(255,255,255,0.9)">
                          {children}
                        </Text>
                      ),
                      h3: ({ children }: MarkdownComponentProps) => (
                        <Text size="md" fw={600} mb="md" c="rgba(255,255,255,0.9)">
                          {children}
                        </Text>
                      ),
                      blockquote: ({ children }: MarkdownComponentProps) => (
                        <div
                          style={{
                            padding: 12,
                            marginBottom: 16,
                            background: 'rgba(0, 191, 255, 0.05)',
                            border: '1px solid rgba(0, 191, 255, 0.1)',
                            borderLeft: '4px solid #00bfff',
                            borderRadius: 8,
                          }}
                        >
                          {children}
                        </div>
                      ),
                    }}
                  >
                    {explanation}
                  </ReactMarkdown>
                </div>
              </Group>
            ) : (
              <Group align="flex-start" gap={12}>
                <div
                  style={{
                    width: 32,
                    height: 32,
                    borderRadius: '50%',
                    background: 'rgba(0, 191, 255, 0.1)',
                    border: '1px solid rgba(0, 191, 255, 0.2)',
                    display: 'flex',
                    alignItems: 'center',
                    justifyContent: 'center',
                  }}
                >
                  <IconRobot size={16} style={{ color: '#00bfff' }} />
                </div>
                <div
                  style={{
                    background: 'rgba(255,255,255,0.05)',
                    border: '1px solid rgba(255,255,255,0.1)',
                    borderRadius: 12,
                    padding: 16,
                    maxWidth: '80%',
                    backdropFilter: 'blur(8px)',
                  }}
                >
                  <Text size="sm" c="rgba(255,255,255,0.7)">
                    {selectedText 
                      ? "Select text to get an explanation" 
                      : "Ask any question or select text to get an explanation"}
                  </Text>
                </div>
              </Group>
            )}
          </Box>

          <div
            style={{ 
              padding: '20px',
              borderTop: '1px solid rgba(255,255,255,0.1)',
              background: 'rgba(255,255,255,0.02)',
              backdropFilter: 'blur(8px)',
            }}
          >
            <form onSubmit={handleAskQuestion}>
              <Group gap={12}>
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
                      background: 'rgba(255,255,255,0.05)',
                      border: '1px solid rgba(255,255,255,0.1)',
                      color: 'rgba(255,255,255,0.9)',
                      borderRadius: 12,
                      '&:focus': {
                        borderColor: 'rgba(0, 191, 255, 0.5)',
                        background: 'rgba(255,255,255,0.08)',
                      },
                      '&::placeholder': {
                        color: 'rgba(255,255,255,0.5)',
                      }
                    }
                  }}
                />
                <Button 
                  type="submit" 
                  loading={isAskingQuestion}
                  disabled={!question.trim()}
                  leftSection={<IconSend size={14} />}
                  style={{
                    background: 'rgba(0, 191, 255, 0.1)',
                    border: '1px solid rgba(0, 191, 255, 0.3)',
                    color: '#00bfff',
                    borderRadius: 12,
                    transition: 'all 200ms ease',
                  }}
                  onMouseEnter={(e) => {
                    e.currentTarget.style.background = 'rgba(0, 191, 255, 0.2)';
                    e.currentTarget.style.borderColor = 'rgba(0, 191, 255, 0.5)';
                  }}
                  onMouseLeave={(e) => {
                    e.currentTarget.style.background = 'rgba(0, 191, 255, 0.1)';
                    e.currentTarget.style.borderColor = 'rgba(0, 191, 255, 0.3)';
                  }}
                >
                  Send
                </Button>
              </Group>
            </form>
          </div>
        </Stack>
      </Modal>
    </>
  );
} 