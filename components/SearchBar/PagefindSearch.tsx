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
  const inputRef = useRef<HTMLInputElement>(null);
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

  // Handle search
  const handleSearch = async (searchQuery: string) => {
    if (!pagefind || !searchQuery.trim()) {
      setResults([]);
      return;
    }

    setIsLoading(true);
    try {
      const search = await pagefind.debouncedSearch(searchQuery);
      setResults(search.results || []);
    } catch (error) {
      console.error('Search error:', error);
      setResults([]);
    } finally {
      setIsLoading(false);
    }
  };

  // Handle input change with debouncing
  useEffect(() => {
    const timer = setTimeout(() => {
      if (query) {
        handleSearch(query);
      } else {
        setResults([]);
      }
    }, 300);

    return () => clearTimeout(timer);
  }, [query, pagefind]);

  // Handle keyboard shortcuts
  useEffect(() => {
    const handleKeyDown = (e: KeyboardEvent) => {
      if ((e.metaKey || e.ctrlKey) && e.key === 'k') {
        e.preventDefault();
        setIsOpen(true);
        setTimeout(() => inputRef.current?.focus(), 100);
      }
      if (e.key === 'Escape') {
        setIsOpen(false);
        setQuery('');
        setResults([]);
        onClose?.();
      }
    };

    document.addEventListener('keydown', handleKeyDown);
    return () => document.removeEventListener('keydown', handleKeyDown);
  }, [onClose]);

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
        onClick={() => setIsOpen(true)}
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
          maxHeight: '80vh',
          overflow: 'hidden',
        }}
      >
        {/* Search Input */}
        <div style={{ padding: '20px', borderBottom: '1px solid rgba(255, 255, 255, 0.1)' }}>
          <div style={{ display: 'flex', alignItems: 'center', gap: 12 }}>
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
            <input
              ref={inputRef}
              type="text"
              placeholder="Search documentation..."
              value={query}
              onChange={(e) => setQuery(e.target.value)}
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
            {isLoading && (
              <div style={{ color: 'rgba(255, 255, 255, 0.6)', fontSize: 14 }}>
                Searching...
              </div>
            )}
          </div>
        </div>

        {/* Search Results */}
        <div style={{ maxHeight: '60vh', overflowY: 'auto' }}>
          {results.length > 0 ? (
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
          ) : null}
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
      <div style={{ color: 'rgba(255, 255, 255, 0.7)', fontSize: 14, lineHeight: 1.4 }}>
        {data.excerpt}
      </div>
      <div style={{ color: 'rgba(255, 255, 255, 0.5)', fontSize: 12, marginTop: 8 }}>
        {data.url.replace(/\.html$/, '').replace(/\/index$/, '')}
      </div>
    </div>
  );
}
