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

'use client'

import { useState, useEffect, useRef } from 'react';
import { useRouter } from 'next/navigation';

interface SearchResult {
  id: string;
  data: () => Promise<{
    url: string;
    meta: {
      title: string;
      description?: string;
    };
    excerpt: string;
  }>;
}

interface PagefindSearchProps {
  onClose?: () => void;
}

export function PagefindSearch({ onClose }: PagefindSearchProps) {
  const [query, setQuery] = useState('');
  const [results, setResults] = useState<SearchResult[]>([]);
  const [isLoading, setIsLoading] = useState(false);
  const [isOpen, setIsOpen] = useState(false);
  const [pagefind, setPagefind] = useState<any>(null);
  const [mode, setMode] = useState<'search' | 'agent'>('search');
  const [agentResults, setAgentResults] = useState<any[]>([]);
  const [isAgentLoading, setIsAgentLoading] = useState(false);
  const inputRef = useRef<HTMLInputElement>(null);
  const resultsRef = useRef<HTMLDivElement>(null);
  const triggerRef = useRef<HTMLButtonElement | null>(null);
  const lastFocusedRef = useRef<HTMLElement | null>(null);
  const router = useRouter();

  // Load Pagefind
  useEffect(() => {
    async function loadPagefind() {
      if (typeof window !== 'undefined' && typeof (window as any).pagefind === 'undefined') {
        try {
          (window as any).pagefind = await import(
            // @ts-expect-error pagefind.js generated after build
            /* webpackIgnore: true */ '/_pagefind/pagefind.js'
          );
          setPagefind((window as any).pagefind);
        } catch (e) {
          console.log('Pagefind not available in development');
          // Mock pagefind for development
          (window as any).pagefind = { 
            search: () => ({ results: [] }),
            debouncedSearch: () => ({ results: [] })
          };
          setPagefind((window as any).pagefind);
        }
      } else if (typeof window !== 'undefined') {
        setPagefind((window as any).pagefind);
      }
    }
    loadPagefind();
  }, []);

  // Highlight search terms in text
  const highlightSearchTerms = (text: string, query: string): string => {
    if (!query.trim()) return text;
    
    const terms = query.toLowerCase().split(/\s+/).filter(term => term.length > 2);
    let highlightedText = text;
    
    terms.forEach(term => {
      const regex = new RegExp(`(${term})`, 'gi');
      highlightedText = highlightedText.replace(regex, '<mark style="background: rgba(0, 191, 255, 0.3); color: #00bfff; padding: 1px 2px; border-radius: 2px;">$1</mark>');
    });
    
    return highlightedText;
  };

  // Handle search
  const handleSearch = async (searchQuery: string) => {
    if (!pagefind || !searchQuery.trim()) {
      setResults([]);
      return;
    }

    setIsLoading(true);
    try {
      const search = await pagefind.debouncedSearch(searchQuery);
      const results = search.results || [];
      
      // Add highlighting to results
      const highlightedResults = results.map((result: any) => ({
        ...result,
        highlightedExcerpt: highlightSearchTerms(result.excerpt || '', searchQuery)
      }));
      
      setResults(highlightedResults);
    } catch (error) {
      console.error('Search error:', error);
      setResults([]);
    } finally {
      setIsLoading(false);
    }
  };

  // Handle AI Agent search
  const handleAgentSearch = async (searchQuery: string) => {
    if (!searchQuery.trim()) return;

    setIsAgentLoading(true);
    setAgentResults([]);
    
    try {
      const response = await fetch('/api/ai-search', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          query: searchQuery,
          limit: 10
        }),
      });

      if (!response.ok) throw new Error('Failed to perform AI agent search');
      
      const data = await response.json();
      const results = data.results || [];
      
      // Add highlighting to agent results
      const highlightedResults = results.map((result: any) => ({
        ...result,
        highlightedExcerpt: highlightSearchTerms(result.excerpt || '', searchQuery)
      }));
      
      setAgentResults(highlightedResults);
    } catch (error) {
      console.error('AI Agent search error:', error);
      setAgentResults([]);
    } finally {
      setIsAgentLoading(false);
    }
  };

  // Handle Enter key and search button for agent mode
  const handleSubmit = (e?: React.FormEvent) => {
    if (e) e.preventDefault();
    if (query.trim() && mode === 'agent') {
      handleAgentSearch(query);
    }
  };

  // Handle input change with debouncing (only for search mode)
  useEffect(() => {
    const timer = setTimeout(() => {
      if (query && mode === 'search') {
        handleSearch(query);
      } else if (mode === 'search') {
        setResults([]);
      }
    }, 300);

    return () => clearTimeout(timer);
  }, [query, pagefind, mode]);

  // Handle keyboard shortcuts
  // Global keyboard shortcuts (open/search mode toggle)
  useEffect(() => {
    const handleGlobalKeyDown = (e: KeyboardEvent) => {
      if ((e.metaKey || e.ctrlKey) && e.key === 'k') {
        e.preventDefault();
        // remember where focus was and open
        lastFocusedRef.current = document.activeElement as HTMLElement | null;
        setIsOpen(true);
        // focus the input shortly after opening
        setTimeout(() => inputRef.current?.focus(), 50);
      }
      if ((e.metaKey || e.ctrlKey) && e.key === 'j') {
        e.preventDefault();
        setMode((m) => (m === 'search' ? 'agent' : 'search'));
      }
    };

    document.addEventListener('keydown', handleGlobalKeyDown);
    return () => document.removeEventListener('keydown', handleGlobalKeyDown);
  }, []);

  // Add an Escape handler only while the modal is open so it can be removed when closed
  useEffect(() => {
    if (!isOpen) return;

    const handleEscape = (e: KeyboardEvent) => {
      if (e.key === 'Escape') {
        e.preventDefault();
        closeModal();
      }
    };

    document.addEventListener('keydown', handleEscape);
    return () => document.removeEventListener('keydown', handleEscape);
  }, [isOpen]);

  // Open/close helpers that manage focus
  const openModal = (fromTrigger?: HTMLElement | null) => {
    lastFocusedRef.current = fromTrigger || (document.activeElement as HTMLElement | null);
    setIsOpen(true);
    setTimeout(() => inputRef.current?.focus(), 50);
  };

  const closeModal = () => {
    setIsOpen(false);
    setQuery('');
    setResults([]);
    setAgentResults([]);
    onClose?.();
    // return focus to trigger if possible
    setTimeout(() => {
      try {
        (lastFocusedRef.current ?? triggerRef.current)?.focus?.();
      } catch (err) {
        // ignore
      }
    }, 0);
  };

  // Clean URL by removing .html extension and fixing paths
  const cleanUrl = (url: string): string => {
    // Remove .html extension
    let cleanUrl = url.replace(/\.html$/, '');
    
    // Handle index pages - convert /docs/index to /docs
    if (cleanUrl.endsWith('/index')) {
      cleanUrl = cleanUrl.replace(/\/index$/, '');
    }
    
    // Ensure URL starts with / if it doesn't already
    if (!cleanUrl.startsWith('/')) {
      cleanUrl = '/' + cleanUrl;
    }
    
    return cleanUrl;
  };

  // Handle result click
  const handleResultClick = async (result: SearchResult) => {
    try {
      const data = await result.data();
      const cleanPath = cleanUrl(data.url);
      router.push(cleanPath);
      setIsOpen(false);
      setQuery('');
      setResults([]);
      onClose?.();
    } catch (error) {
      console.error('Error navigating to result:', error);
    }
  };

  if (!isOpen) {
    return (
      <button
        ref={triggerRef}
        onClick={(e) => openModal(e.currentTarget)}
        onKeyDown={(e) => {
          if (e.key === 'Enter' || e.key === ' ') {
            e.preventDefault();
            openModal(e.currentTarget as HTMLElement);
          }
        }}
        style={{
          display: 'flex',
          alignItems: 'center',
          gap: 8,
          padding: '8px 16px',
          background: 'rgba(255, 255, 255, 0.08)',
          border: '1px solid rgba(255, 255, 255, 0.1)',
          borderRadius: 24,
          color: 'rgba(255, 255, 255, 0.7)',
          fontSize: 14,
          fontWeight: 400,
          cursor: 'pointer',
          transition: 'all 200ms ease',
          backdropFilter: 'blur(10px)',
          minWidth: 200,
          justifyContent: 'flex-start',
        }}
        onMouseEnter={(e) => {
          e.currentTarget.style.background = 'rgba(255, 255, 255, 0.12)';
          e.currentTarget.style.borderColor = 'rgba(255, 255, 255, 0.2)';
        }}
        onMouseLeave={(e) => {
          e.currentTarget.style.background = 'rgba(255, 255, 255, 0.08)';
          e.currentTarget.style.borderColor = 'rgba(255, 255, 255, 0.1)';
        }}
      >
        <svg
          width="16"
          height="16"
          fill="currentColor"
          viewBox="0 0 20 20"
          style={{ opacity: 0.6, flexShrink: 0 }}
  >
          <path
            fillRule="evenodd"
            d="M8 4a4 4 0 100 8 4 4 0 000-8zM2 8a6 6 0 1110.89 3.476l4.817 4.817a1 1 0 01-1.414 1.414l-4.816-4.816A6 6 0 012 8z"
            clipRule="evenodd"
          />
        </svg>
        <span style={{ flex: 1, textAlign: 'left' }}>Search...</span>
        <div style={{ display: 'flex', alignItems: 'center', gap: 2, opacity: 0.5 }}>
          <kbd style={{
            background: 'rgba(255, 255, 255, 0.1)',
            border: '1px solid rgba(255, 255, 255, 0.2)',
            borderRadius: 4,
            padding: '2px 6px',
            fontSize: 11,
            fontFamily: 'monospace',
            color: 'rgba(255, 255, 255, 0.7)',
          }}>
            ⌘
          </kbd>
          <kbd style={{
            background: 'rgba(255, 255, 255, 0.1)',
            border: '1px solid rgba(255, 255, 255, 0.2)',
            borderRadius: 4,
            padding: '2px 6px',
            fontSize: 11,
            fontFamily: 'monospace',
            color: 'rgba(255, 255, 255, 0.7)',
          }}>
            K
          </kbd>
        </div>
      </button>
    );
  }

  return (
    <div
      style={{
        position: 'fixed',
        top: 0,
        left: 0,
        right: 0,
        bottom: 0,
        background: 'rgba(0, 0, 0, 0.8)',
        backdropFilter: 'blur(8px)',
        zIndex: 10000,
        display: 'flex',
        alignItems: 'flex-start',
        justifyContent: 'center',
        paddingTop: '10vh',
      }}
      onClick={(e) => {
        if (e.target === e.currentTarget) {
          setIsOpen(false);
          setQuery('');
          setResults([]);
          onClose?.();
        }
      }}
    >
      <div
        style={{
          background: 'rgba(255, 255, 255, 0.1)',
          border: '1px solid rgba(255, 255, 255, 0.2)',
          borderRadius: 12,
          backdropFilter: 'blur(10px)',
          minWidth: 600,
          maxWidth: '90vw',
          height: '500px',
          overflow: 'hidden',
          display: 'flex',
          flexDirection: 'column',
        }}
      >
        {/* Search Input */}
        <div style={{ padding: '20px', borderBottom: '1px solid rgba(255, 255, 255, 0.1)' }}>
          {/* Mode Toggle */}
          <div style={{ display: 'flex', gap: 8, marginBottom: 12 }}>
            <button
              onClick={() => setMode('search')}
              style={{
                padding: '6px 12px',
                borderRadius: 6,
                border: 'none',
                background: mode === 'search' ? 'rgba(0, 191, 255, 0.2)' : 'rgba(255, 255, 255, 0.1)',
                color: mode === 'search' ? '#00bfff' : 'rgba(255, 255, 255, 0.7)',
                fontSize: 12,
                fontWeight: 500,
                cursor: 'pointer',
                transition: 'all 200ms ease',
              }}
            >
              🔍 Search
            </button>
            <button
              onClick={() => setMode('agent')}
              style={{
                padding: '6px 12px',
                borderRadius: 6,
                border: 'none',
                background: mode === 'agent' ? 'rgba(0, 191, 255, 0.2)' : 'rgba(255, 255, 255, 0.1)',
                color: mode === 'agent' ? '#00bfff' : 'rgba(255, 255, 255, 0.7)',
                fontSize: 12,
                fontWeight: 500,
                cursor: 'pointer',
                transition: 'all 200ms ease',
                display: 'flex',
                alignItems: 'center',
                gap: 6,
              }}
            >
              <svg width="14" height="14" viewBox="0 0 24 24" fill="currentColor">
                <path d="M12 2L2 7l10 5 10-5-10-5zM2 17l10 5 10-5M2 12l10 5 10-5"/>
              </svg>
              Ask Our Agent
              <span style={{
                background: 'rgba(255, 193, 7, 0.2)',
                color: '#ffc107',
                fontSize: 10,
                padding: '2px 4px',
                borderRadius: 3,
                fontWeight: 600,
                textTransform: 'uppercase',
                letterSpacing: '0.5px'
              }}>
                Beta
              </span>
            </button>
          </div>
          
          <form onSubmit={handleSubmit} style={{ display: 'flex', alignItems: 'center', gap: 12, width: '100%' }}>
            {mode === 'search' ? (
              <svg
                width="20"
                height="20"
                fill="currentColor"
                viewBox="0 0 20 20"
                style={{ opacity: 0.6 }}
              >
                <path
                  fillRule="evenodd"
                  d="M8 4a4 4 0 100 8 4 4 0 000-8zM2 8a6 6 0 1110.89 3.476l4.817 4.817a1 1 0 01-1.414 1.414l-4.816-4.816A6 6 0 012 8z"
                  clipRule="evenodd"
                />
              </svg>
            ) : (
              <svg
                width="20"
                height="20"
                fill="currentColor"
                viewBox="0 0 24 24"
                style={{ opacity: 0.6 }}
              >
                <path d="M12 2L2 7l10 5 10-5-10-5zM2 17l10 5 10-5M2 12l10 5 10-5"/>
              </svg>
            )}
            <input
              ref={inputRef}
              type="text"
              placeholder={
                mode === 'search' 
                  ? "Search documentation..." 
                  : "Ask our agent to find relevant content (e.g., 'storage optimization', 'transaction handling')..."
              }
              value={query}
              onChange={(e) => setQuery(e.target.value)}
              onKeyDown={(e) => {
                if (e.key === 'Enter' && mode === 'agent') {
                  e.preventDefault();
                  handleSubmit();
                }
              }}
              style={{
                background: 'transparent',
                border: 'none',
                outline: 'none',
                color: 'rgba(255, 255, 255, 0.9)',
                fontSize: 16,
                flex: 1,
                minWidth: 0,
              }}
            />
            {mode === 'agent' && query.trim() && !isAgentLoading && (
              <button
                type="submit"
                style={{
                  padding: '8px 16px',
                  background: 'rgba(0, 191, 255, 0.2)',
                  border: '1px solid rgba(0, 191, 255, 0.4)',
                  borderRadius: 8,
                  color: '#00bfff',
                  fontSize: 14,
                  fontWeight: 500,
                  cursor: 'pointer',
                  transition: 'all 200ms ease',
                }}
                onMouseEnter={(e) => {
                  e.currentTarget.style.background = 'rgba(0, 191, 255, 0.3)';
                  e.currentTarget.style.borderColor = 'rgba(0, 191, 255, 0.6)';
                }}
                onMouseLeave={(e) => {
                  e.currentTarget.style.background = 'rgba(0, 191, 255, 0.2)';
                  e.currentTarget.style.borderColor = 'rgba(0, 191, 255, 0.4)';
                }}
              >
                Ask Our Agent
              </button>
            )}
            {(isLoading || isAgentLoading) && (
              <div style={{ color: 'rgba(255, 255, 255, 0.6)', fontSize: 14 }}>
                {mode === 'search' ? 'Searching...' : 'Our agent is searching...'}
              </div>
            )}
          </form>
          
          {/* Keyboard shortcuts hint */}
          <div style={{ 
            display: 'flex', 
            gap: 8, 
            marginTop: 8, 
            fontSize: 11, 
            color: 'rgba(255, 255, 255, 0.5)' 
          }}>
            <span>⌘K to open</span>
            <span>⌘J to switch mode</span>
            {mode === 'agent' && <span>Enter to ask our agent</span>}
            <span>Esc to close</span>
          </div>
        </div>

        {/* Results */}
        <div 
          ref={resultsRef}
          style={{ 
            flex: 1,
            overflowY: 'auto',
            overflowX: 'hidden',
            minHeight: 0
          }}
        >
          {mode === 'search' ? (
            // Search Results
            results.length > 0 ? (
              results.map((result, index) => (
                <SearchResultItem
                  key={result.id}
                  result={result}
                  onClick={() => handleResultClick(result)}
                />
              ))
            ) : query && !isLoading ? (
              <div style={{ padding: '20px', textAlign: 'center', color: 'rgba(255, 255, 255, 0.6)' }}>
                No results found for "{query}"
              </div>
            ) : !query ? (
              <div style={{ padding: '20px', textAlign: 'center', color: 'rgba(255, 255, 255, 0.6)' }}>
                Start typing to search...
              </div>
            ) : null
          ) : (
            // Agent Results
            agentResults.length > 0 ? (
              agentResults.map((result, index) => (
                <div
                  key={result.id}
                  onClick={() => {
                    router.push(result.url);
                    setIsOpen(false); // Auto-close search bar
                    setQuery('');
                    setAgentResults([]);
                  }}
                  style={{
                    padding: '16px 20px',
                    borderBottom: '1px solid rgba(255, 255, 255, 0.05)',
                    cursor: 'pointer',
                    transition: 'background-color 0.2s ease',
                  }}
                  onMouseEnter={(e) => {
                    e.currentTarget.style.background = 'rgba(255, 255, 255, 0.1)';
                  }}
                  onMouseLeave={(e) => {
                    e.currentTarget.style.background = 'transparent';
                  }}
                >
                  <div style={{ 
                    color: 'rgba(255, 255, 255, 0.9)', 
                    fontSize: 16, 
                    fontWeight: 500, 
                    marginBottom: 8 
                  }}>
                    {result.title}
                    {result.matchedSection && (
                      <span style={{ 
                        color: 'rgba(0, 191, 255, 0.8)', 
                        fontSize: 14, 
                        fontWeight: 400,
                        marginLeft: 8
                      }}>
                        → {result.matchedSection.title}
                      </span>
                    )}
                  </div>
                  <div 
                    style={{ 
                      color: 'rgba(255, 255, 255, 0.7)', 
                      fontSize: 14, 
                      lineHeight: 1.4,
                      marginBottom: 8
                    }}
                    dangerouslySetInnerHTML={{ 
                      __html: result.highlightedExcerpt || result.excerpt 
                    }}
                  />
                  <div style={{ 
                    color: 'rgba(0, 191, 255, 0.8)', 
                    fontSize: 12,
                    display: 'flex',
                    alignItems: 'center',
                    gap: 8
                  }}>
                    <div style={{ display: 'flex', alignItems: 'center', gap: 4 }}>
                      <svg width="12" height="12" viewBox="0 0 24 24" fill="currentColor">
                        <path d="M12 2L2 7l10 5 10-5-10-5zM2 17l10 5 10-5M2 12l10 5 10-5"/>
                      </svg>
                      <span>Ask Our Agent</span>
                      <span style={{
                        background: 'rgba(255, 193, 7, 0.2)',
                        color: '#ffc107',
                        fontSize: 8,
                        padding: '1px 3px',
                        borderRadius: 2,
                        fontWeight: 600,
                        textTransform: 'uppercase',
                        letterSpacing: '0.3px'
                      }}>
                        Beta
                      </span>
                    </div>
                    <span>•</span>
                    <span>{result.url}</span>
                    <span>•</span>
                    <span>Score: {result.relevanceScore.toFixed(1)}</span>
                    {result.matchedSection && (
                      <>
                        <span>•</span>
                        <span>📍 Section match</span>
                      </>
                    )}
                  </div>
                </div>
              ))
            ) : query && !isAgentLoading ? (
              <div style={{ padding: '20px', textAlign: 'center', color: 'rgba(255, 255, 255, 0.6)' }}>
                No results found for "{query}"
              </div>
            ) : !query ? (
              <div style={{ padding: '20px', textAlign: 'center', color: 'rgba(255, 255, 255, 0.6)' }}>
                Start typing to ask our agent...
              </div>
            ) : null
          )}
        </div>
      </div>
    </div>
  );
}

function SearchResultItem({ result, onClick }: { result: SearchResult; onClick: () => void }) {
  const [data, setData] = useState<any>(null);

  useEffect(() => {
    async function fetchData() {
      try {
        const resultData = await result.data();
        setData(resultData);
      } catch (error) {
        console.error('Error fetching result data:', error);
      }
    }
    fetchData();
  }, [result]);

  if (!data) {
    return (
      <div style={{ padding: '16px 20px', color: 'rgba(255, 255, 255, 0.6)' }}>
        Loading...
      </div>
    );
  }

  return (
    <div
      onClick={onClick}
      style={{
        padding: '16px 20px',
        borderBottom: '1px solid rgba(255, 255, 255, 0.05)',
        cursor: 'pointer',
        transition: 'background-color 0.2s ease',
      }}
      onMouseEnter={(e) => {
        e.currentTarget.style.background = 'rgba(255, 255, 255, 0.1)';
      }}
      onMouseLeave={(e) => {
        e.currentTarget.style.background = 'transparent';
      }}
    >
      <div style={{ color: 'rgba(255, 255, 255, 0.9)', fontWeight: 600, marginBottom: 4 }}>
        {data.meta.title}
      </div>
      {data.meta.description && (
        <div style={{ color: 'rgba(255, 255, 255, 0.6)', fontSize: 14, marginBottom: 8 }}>
          {data.meta.description}
        </div>
      )}
      <div 
        style={{ color: 'rgba(255, 255, 255, 0.7)', fontSize: 14, lineHeight: 1.4 }}
        dangerouslySetInnerHTML={{ 
          __html: data.highlightedExcerpt || data.excerpt 
        }}
      />
      <div style={{ color: 'rgba(255, 255, 255, 0.5)', fontSize: 12, marginTop: 8 }}>
        {data.url.replace(/\.html$/, '').replace(/\/index$/, '')}
      </div>
    </div>
  );
}
