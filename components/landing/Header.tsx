'use client';

import { SearchBar } from '../SearchBar/SearchBar';

export default function Header() {
  return (
    <header style={{ position: 'relative', zIndex: 60, display: 'flex', alignItems: 'center', justifyContent: 'space-between', padding: 24 }}>
      {/* Logo - Left side */}
      <div style={{ display: 'flex', alignItems: 'center', flex: '0 0 auto' }}>
        <a href="/" style={{ textDecoration: 'none' }}>
          <button
            style={{
              background: 'transparent',
              border: 'none',
              cursor: 'pointer',
              padding: 8,
              borderRadius: 8,
              transition: 'all 200ms ease',
            }}
            onMouseEnter={(e) => (e.currentTarget.style.background = 'rgba(255,255,255,0.1)')}
            onMouseLeave={(e) => (e.currentTarget.style.background = 'transparent')}
            aria-label="Go to home page"
          >
        <svg
          fill="currentColor"
              viewBox="0 0 120 32"
          xmlns="http://www.w3.org/2000/svg"
          aria-hidden="true"
              style={{ width: 120, height: 32, color: '#fff' }}
            >
              {/* Vercel-style lighthouse triangle - sized to match text */}
              <path d="M8 4 L2 18 L8 28 L14 18 Z" fill="currentColor" opacity="0.9"/>
              <path d="M8 6 L4 18 L8 26 L12 18 Z" fill="currentColor" opacity="0.6"/>
              <circle cx="8" cy="18" r="1.5" fill="currentColor" opacity="0.8"/>
              
              {/* Text */}
              <text x="20" y="22" fontSize="18" fontWeight="600" fill="currentColor" fontFamily="system-ui, -apple-system, sans-serif">
                Beacon
              </text>
        </svg>
          </button>
        </a>
      </div>

      {/* Navigation - Center */}
      <nav style={{ display: 'flex', alignItems: 'center', columnGap: 8, position: 'absolute', left: '50%', transform: 'translateX(-50%)' }}>
          <a
          href="/docs"
            style={{
              color: 'rgba(255,255,255,0.8)',
              paddingInline: 12,
              paddingBlock: 8,
              borderRadius: 999,
              fontSize: 12,
              fontWeight: 300,
              textDecoration: 'none',
              transition: 'all 200ms ease',
              background: 'transparent',
              cursor: 'pointer',
            }}
            onMouseEnter={(e) => (e.currentTarget.style.background = 'rgba(255,255,255,0.1)')}
            onMouseLeave={(e) => (e.currentTarget.style.background = 'transparent')}
          >
          Docs
          </a>

        <div style={{ position: 'relative' }}>
        <button
          style={{
              color: 'rgba(255,255,255,0.8)',
              paddingInline: 12,
            paddingBlock: 8,
            borderRadius: 999,
            fontSize: 12,
              fontWeight: 300,
              background: 'transparent',
              border: 'none',
            cursor: 'pointer',
              transition: 'all 200ms ease',
            display: 'flex',
            alignItems: 'center',
              gap: 4,
            }}
            onMouseEnter={(e) => {
              e.currentTarget.style.background = 'rgba(255,255,255,0.1)';
              const dropdown = e.currentTarget.nextElementSibling as HTMLElement;
              if (dropdown) dropdown.style.display = 'block';
            }}
            onMouseLeave={(e) => {
              e.currentTarget.style.background = 'transparent';
              const dropdown = e.currentTarget.nextElementSibling as HTMLElement;
              if (dropdown) dropdown.style.display = 'none';
            }}
          >
            Community
            <svg width="12" height="12" fill="currentColor" viewBox="0 0 20 20">
              <path fillRule="evenodd" d="M5.293 7.293a1 1 0 011.414 0L10 10.586l3.293-3.293a1 1 0 111.414 1.414l-4 4a1 1 0 01-1.414 0l-4-4a1 1 0 010-1.414z" clipRule="evenodd" />
          </svg>
        </button>
          
          <div
            style={{
              position: 'absolute',
              top: '100%',
              left: 0,
              background: 'transparent',
              border: '1px solid rgba(255,255,255,0.1)',
              borderRadius: 8,
              padding: 8,
              minWidth: 120,
              display: 'none',
              zIndex: 1000,
            }}
            onMouseEnter={(e) => (e.currentTarget.style.display = 'block')}
            onMouseLeave={(e) => (e.currentTarget.style.display = 'none')}
          >
            <a
              href="https://blog.resilientdb.com"
              target="_blank"
              rel="noopener noreferrer"
              style={{
                display: 'block',
                color: 'rgba(255,255,255,0.8)',
                padding: '8px 12px',
                fontSize: 12,
                textDecoration: 'none',
                borderRadius: 4,
                transition: 'all 200ms ease',
              }}
              onMouseEnter={(e) => (e.currentTarget.style.background = 'rgba(255,255,255,0.1)')}
              onMouseLeave={(e) => (e.currentTarget.style.background = 'transparent')}
            >
              Blog
            </a>
          </div>
        </div>
        
        <a
          href="/docs/about"
          style={{
            color: 'rgba(255,255,255,0.8)',
            paddingInline: 12,
            paddingBlock: 8,
            borderRadius: 999,
            fontSize: 12,
            fontWeight: 300,
            textDecoration: 'none',
            transition: 'all 200ms ease',
            background: 'transparent',
            cursor: 'pointer',
          }}
          onMouseEnter={(e) => (e.currentTarget.style.background = 'rgba(255,255,255,0.1)')}
          onMouseLeave={(e) => (e.currentTarget.style.background = 'transparent')}
        >
          About
        </a>
      </nav>

      {/* Search Bar - Right side */}
      <div style={{ display: 'flex', alignItems: 'center', flex: '0 0 auto' }}>
        <SearchBar/>
      </div>
    </header>
  );
}


