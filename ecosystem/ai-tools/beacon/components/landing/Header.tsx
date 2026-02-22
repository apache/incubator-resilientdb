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


import { SearchBar } from '../SearchBar/SearchBar';
import { useEffect, useState } from 'react';


export default function Header() {
  // Simple client-side breakpoint so we can adjust inline styles without refactoring to CSS modules
  const [isMobile, setIsMobile] = useState(false);


  useEffect(() => {
    const mq = window.matchMedia('(max-width: 640px)');


    const apply = () => setIsMobile(mq.matches);
    apply();


    // Support older Safari
    if (typeof mq.addEventListener === 'function') {
      mq.addEventListener('change', apply);
      return () => mq.removeEventListener('change', apply);
    } else {
      // @ts-ignore
      mq.addListener(apply);
      // @ts-ignore
      return () => mq.removeListener(apply);
    }
  }, []);


  return (
    <header
      style={{
        position: 'relative',
        zIndex: 60,
        display: 'flex',
        alignItems: 'center',
        justifyContent: 'space-between',
        padding: isMobile ? 12 : 24,
        gap: isMobile ? 10 : 0,
        flexWrap: isMobile ? 'wrap' : 'nowrap',
      }}
    >
      {/* Logo - Left side */}
      <div style={{ display: 'flex', alignItems: 'center', flex: '0 0 auto' }}>
        <a href="/" style={{ textDecoration: 'none' }}>
          <button
            style={{
              background: 'transparent',
              border: 'none',
              cursor: 'pointer',
              padding: isMobile ? 6 : 8,
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
              style={{
                width: isMobile ? 104 : 120,
                height: isMobile ? 28 : 32,
                color: '#fff',
              }}
            >
              {/* Vercel-style lighthouse triangle - sized to match text */}
              <path d="M8 4 L2 18 L8 28 L14 18 Z" fill="currentColor" opacity="0.9" />
              <path d="M8 6 L4 18 L8 26 L12 18 Z" fill="currentColor" opacity="0.6" />
              <circle cx="8" cy="18" r="1.5" fill="currentColor" opacity="0.8" />


              {/* Text */}
              <text
                x="20"
                y="22"
                fontSize={isMobile ? 16 : 18}
                fontWeight="600"
                fill="currentColor"
                fontFamily="system-ui, -apple-system, sans-serif"
              >
                Beacon
              </text>
            </svg>
          </button>
        </a>
      </div>


      {/* Navigation - Center (desktop absolute; mobile flows and wraps) */}
      <nav
        style={{
          display: 'flex',
          alignItems: 'center',
          columnGap: 8,


          // Desktop: centered overlay
          position: isMobile ? 'static' : 'absolute',
          left: isMobile ? undefined : '50%',
          transform: isMobile ? undefined : 'translateX(-50%)',


          // Mobile: take full row and center items
          width: isMobile ? '100%' : undefined,
          justifyContent: isMobile ? 'center' : undefined,
          order: isMobile ? 3 : undefined,


          // Small extra breathing room so it doesn't collide vertically
          marginTop: isMobile ? 4 : undefined,
        }}
      >
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
            whiteSpace: 'nowrap',
          }}
          onMouseEnter={(e) => (e.currentTarget.style.background = 'rgba(255,255,255,0.1)')}
          onMouseLeave={(e) => (e.currentTarget.style.background = 'transparent')}
        >
          Docs
        </a>


        <div style={{ position: 'relative' }}>
          {/* Community dropdown trigger */}
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
              whiteSpace: 'nowrap',
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
            onKeyDown={(e) => {
              if (e.key === 'Enter' || e.key === ' ') {
                e.preventDefault();
                const dropdown = (e.currentTarget as HTMLElement).nextElementSibling as HTMLElement | null;
                if (dropdown) {
                  const isShown = dropdown.style.display === 'block';
                  if (!isShown) {
                    dropdown.style.display = 'block';
                    const first = dropdown.querySelector('a, button') as HTMLElement | null;
                    first?.focus();
                  } else {
                    dropdown.style.display = 'none';
                  }
                }
              }
              if (e.key === 'ArrowDown') {
                e.preventDefault();
                const dropdown = (e.currentTarget as HTMLElement).nextElementSibling as HTMLElement | null;
                const first = dropdown?.querySelector('a, button') as HTMLElement | null;
                first?.focus();
              }
            }}
            tabIndex={0}
            aria-haspopup="true"
            aria-expanded={false}
          >
            Community
            <svg width="12" height="12" fill="currentColor" viewBox="0 0 20 20">
              <path
                fillRule="evenodd"
                d="M5.293 7.293a1 1 0 011.414 0L10 10.586l3.293-3.293a1 1 0 111.414 1.414l-4 4a1 1 0 01-1.414 0l-4-4a1 1 0 010-1.414z"
                clipRule="evenodd"
              />
            </svg>
          </button>


          <div
            role="menu"
            aria-label="Community"
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
            onKeyDown={(e) => {
              if (e.key === 'Escape') {
                e.preventDefault();
                const trigger = e.currentTarget.previousElementSibling as HTMLElement | null;
                (e.currentTarget as HTMLElement).style.display = 'none';
                trigger?.focus?.();
              }
            }}
          >
            <a
              href="https://blog.resilientdb.com"
              target="_blank"
              rel="noopener noreferrer"
              role="menuitem"
              tabIndex={-1}
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
              onKeyDown={(e) => {
                if (e.key === 'Escape') {
                  e.preventDefault();
                  const menu = e.currentTarget.parentElement as HTMLElement | null;
                  menu && (menu.style.display = 'none');
                  const trigger = menu?.previousElementSibling as HTMLElement | null;
                  trigger?.focus?.();
                }
              }}
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
            whiteSpace: 'nowrap',
          }}
          onMouseEnter={(e) => (e.currentTarget.style.background = 'rgba(255,255,255,0.1)')}
          onMouseLeave={(e) => (e.currentTarget.style.background = 'transparent')}
        >
          About
        </a>
      </nav>


      {/* Search Bar - Right side */}
      <div
        style={{
          display: 'flex',
          alignItems: 'center',
          flex: '0 0 auto',


          // Mobile: allow search to be on its own line if needed
          order: isMobile ? 2 : undefined,
          width: isMobile ? 'auto' : undefined,
          marginLeft: isMobile ? 'auto' : undefined,
        }}
      >
        <SearchBar />
      </div>
    </header>
  );
}

