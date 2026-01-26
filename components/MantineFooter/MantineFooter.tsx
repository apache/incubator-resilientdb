'use client';

/**
 * Sleek footer component matching the landing page design language
 * Features glassmorphism effects, subtle animations, and modern styling
 * Uses regular HTML/React components for full width stretching
 *
 * @since 1.0.0
 */
export const MantineFooter = () => (
  <footer 
    style={{ 
      position: 'relative', 
      marginTop: 48,
      width: '100vw',
      left: '50%',
      right: '50%',
      marginLeft: '-50vw',
      marginRight: '-50vw',
      padding: '32px 24px',
      background: 'rgba(255, 255, 255, 0.02)',
      backdropFilter: 'blur(10px)',
      borderTop: '1px solid rgba(255, 255, 255, 0.08)',
      borderBottom: '1px solid rgba(255, 255, 255, 0.08)',
    }}
  >
    <div style={{ maxWidth: '1200px', margin: '0 auto' }}>
      <div style={{ display: 'flex', flexDirection: 'column', gap: 24 }}>
        {/* Main footer content */}
        <div style={{ 
          display: 'flex', 
          justifyContent: 'space-between', 
          alignItems: 'flex-start', 
          width: '100%',
          gap: 32,
          flexWrap: 'wrap'
        }}>
          {/* Left section - Brand */}
          <div style={{ flex: 1, minWidth: 200 }}>
            <div style={{ display: 'flex', flexDirection: 'column', gap: 12 }}>
              <div style={{ display: 'flex', alignItems: 'center', gap: 8 }}>
                <svg
                  fill="currentColor"
                  viewBox="0 0 120 32"
                  xmlns="http://www.w3.org/2000/svg"
                  style={{ width: 100, height: 24, color: 'rgba(255,255,255,0.9)' }}
                >
                  <path d="M8 4 L2 18 L8 28 L14 18 Z" fill="currentColor" opacity="0.9"/>
                  <path d="M8 6 L4 18 L8 26 L12 18 Z" fill="currentColor" opacity="0.6"/>
                  <circle cx="8" cy="18" r="1.5" fill="currentColor" opacity="0.8"/>
                  <text x="20" y="22" fontSize="16" fontWeight="600" fill="currentColor" fontFamily="system-ui, -apple-system, sans-serif">
                    Beacon
                  </text>
                </svg>
              </div>
              <p style={{ 
                fontSize: 14, 
                color: 'rgba(255,255,255,0.6)', 
                lineHeight: 1.5, 
                maxWidth: 280,
                margin: 0
              }}>
                Next generation documentation for ResilientDB ecosystem. 
                Your gateway to high-performance blockchain infrastructure.
              </p>
            </div>
          </div>

          {/* Center section - Links */}
          <div style={{ 
            flex: 1, 
            display: 'flex', 
            justifyContent: 'center', 
            gap: 32,
            flexWrap: 'wrap'
          }}>
            <div style={{ display: 'flex', flexDirection: 'column', gap: 8 }}>
              <h4 style={{ 
                fontSize: 14, 
                fontWeight: 500, 
                color: 'white', 
                margin: '0 0 4px 0'
              }}>
                Documentation
              </h4>
              <div style={{ display: 'flex', flexDirection: 'column', gap: 4 }}>
                <a 
                  href="/docs" 
                  style={{ 
                    fontSize: 14,
                    color: 'rgba(255,255,255,0.6)',
                    transition: 'all 200ms ease',
                    textDecoration: 'none',
                    display: 'block',
                  }}
                  onMouseEnter={(e) => {
                    e.currentTarget.style.color = 'rgba(255,255,255,0.9)';
                    e.currentTarget.style.transform = 'translateX(4px)';
                  }}
                  onMouseLeave={(e) => {
                    e.currentTarget.style.color = 'rgba(255,255,255,0.6)';
                    e.currentTarget.style.transform = 'translateX(0)';
                  }}
                >
                  Getting Started
                </a>
                <a 
                  href="/docs/installation" 
                  style={{ 
                    fontSize: 14,
                    color: 'rgba(255,255,255,0.6)',
                    transition: 'all 200ms ease',
                    textDecoration: 'none',
                    display: 'block',
                  }}
                  onMouseEnter={(e) => {
                    e.currentTarget.style.color = 'rgba(255,255,255,0.9)';
                    e.currentTarget.style.transform = 'translateX(4px)';
                  }}
                  onMouseLeave={(e) => {
                    e.currentTarget.style.color = 'rgba(255,255,255,0.6)';
                    e.currentTarget.style.transform = 'translateX(0)';
                  }}
                >
                  Installation
                </a>
                <a 
                  href="/docs/resilientdb" 
                  style={{ 
                    fontSize: 14,
                    color: 'rgba(255,255,255,0.6)',
                    transition: 'all 200ms ease',
                    textDecoration: 'none',
                    display: 'block',
                  }}
                  onMouseEnter={(e) => {
                    e.currentTarget.style.color = 'rgba(255,255,255,0.9)';
                    e.currentTarget.style.transform = 'translateX(4px)';
                  }}
                  onMouseLeave={(e) => {
                    e.currentTarget.style.color = 'rgba(255,255,255,0.6)';
                    e.currentTarget.style.transform = 'translateX(0)';
                  }}
                >
                  ResilientDB
                </a>
              </div>
            </div>

            <div style={{ display: 'flex', flexDirection: 'column', gap: 8 }}>
              <h4 style={{ 
                fontSize: 14, 
                fontWeight: 500, 
                color: 'white', 
                margin: '0 0 4px 0'
              }}>
                Community
              </h4>
              <div style={{ display: 'flex', flexDirection: 'column', gap: 4 }}>
                <a 
                  href="https://resilientdb.incubator.apache.org/" 
                  target="_blank"
                  rel="noopener noreferrer"
                  style={{ 
                    fontSize: 14,
                    color: 'rgba(255,255,255,0.6)',
                    transition: 'all 200ms ease',
                    textDecoration: 'none',
                    display: 'block',
                  }}
                  onMouseEnter={(e) => {
                    e.currentTarget.style.color = 'rgba(255,255,255,0.9)';
                    e.currentTarget.style.transform = 'translateX(4px)';
                  }}
                  onMouseLeave={(e) => {
                    e.currentTarget.style.color = 'rgba(255,255,255,0.6)';
                    e.currentTarget.style.transform = 'translateX(0)';
                  }}
                >
                  Official Site
                </a>
                <a 
                  href="https://blog.resilientdb.com" 
                  target="_blank"
                  rel="noopener noreferrer"
                  style={{ 
                    fontSize: 14,
                    color: 'rgba(255,255,255,0.6)',
                    transition: 'all 200ms ease',
                    textDecoration: 'none',
                    display: 'block',
                  }}
                  onMouseEnter={(e) => {
                    e.currentTarget.style.color = 'rgba(255,255,255,0.9)';
                    e.currentTarget.style.transform = 'translateX(4px)';
                  }}
                  onMouseLeave={(e) => {
                    e.currentTarget.style.color = 'rgba(255,255,255,0.6)';
                    e.currentTarget.style.transform = 'translateX(0)';
                  }}
                >
                  Blog
                </a>
                <a 
                  href="/docs/about" 
                  style={{ 
                    fontSize: 14,
                    color: 'rgba(255,255,255,0.6)',
                    transition: 'all 200ms ease',
                    textDecoration: 'none',
                    display: 'block',
                  }}
                  onMouseEnter={(e) => {
                    e.currentTarget.style.color = 'rgba(255,255,255,0.9)';
                    e.currentTarget.style.transform = 'translateX(4px)';
                  }}
                  onMouseLeave={(e) => {
                    e.currentTarget.style.color = 'rgba(255,255,255,0.6)';
                    e.currentTarget.style.transform = 'translateX(0)';
                  }}
                >
                  About
                </a>
              </div>
            </div>
          </div>

          {/* Right section - Tech stack */}
          <div style={{ 
            flex: 1, 
            minWidth: 200, 
            display: 'flex', 
            flexDirection: 'column', 
            alignItems: 'flex-end',
            gap: 12
          }}>
            <h4 style={{ 
              fontSize: 14, 
              fontWeight: 500, 
              color: 'white', 
              margin: 0
            }}>
              Built with
            </h4>
            <div style={{ display: 'flex', gap: 8, flexWrap: 'wrap' }}>
              <div
                style={{
                  padding: '4px 8px',
                  borderRadius: 6,
                  background: 'rgba(255,255,255,0.05)',
                  border: '1px solid rgba(255,255,255,0.1)',
                  fontSize: 11,
                  color: 'rgba(255,255,255,0.7)',
                  transition: 'all 200ms ease',
                }}
                onMouseEnter={(e) => {
                  e.currentTarget.style.background = 'rgba(255,255,255,0.1)';
                  e.currentTarget.style.borderColor = 'rgba(255,255,255,0.2)';
                }}
                onMouseLeave={(e) => {
                  e.currentTarget.style.background = 'rgba(255,255,255,0.05)';
                  e.currentTarget.style.borderColor = 'rgba(255,255,255,0.1)';
                }}
              >
                Next.js
              </div>
              <div
                style={{
                  padding: '4px 8px',
                  borderRadius: 6,
                  background: 'rgba(255,255,255,0.05)',
                  border: '1px solid rgba(255,255,255,0.1)',
                  fontSize: 11,
                  color: 'rgba(255,255,255,0.7)',
                  transition: 'all 200ms ease',
                }}
                onMouseEnter={(e) => {
                  e.currentTarget.style.background = 'rgba(255,255,255,0.1)';
                  e.currentTarget.style.borderColor = 'rgba(255,255,255,0.2)';
                }}
                onMouseLeave={(e) => {
                  e.currentTarget.style.background = 'rgba(255,255,255,0.05)';
                  e.currentTarget.style.borderColor = 'rgba(255,255,255,0.1)';
                }}
              >
                Mantine
              </div>
              <div
                style={{
                  padding: '4px 8px',
                  borderRadius: 6,
                  background: 'rgba(255,255,255,0.05)',
                  border: '1px solid rgba(255,255,255,0.1)',
                  fontSize: 11,
                  color: 'rgba(255,255,255,0.7)',
                  transition: 'all 200ms ease',
                }}
                onMouseEnter={(e) => {
                  e.currentTarget.style.background = 'rgba(255,255,255,0.1)';
                  e.currentTarget.style.borderColor = 'rgba(255,255,255,0.2)';
                }}
                onMouseLeave={(e) => {
                  e.currentTarget.style.background = 'rgba(255,255,255,0.05)';
                  e.currentTarget.style.borderColor = 'rgba(255,255,255,0.1)';
                }}
              >
                Nextra
              </div>
            </div>
          </div>
        </div>

        {/* Divider */}
        <div 
          style={{ 
            height: 1,
            margin: '8px 0',
            background: 'linear-gradient(90deg, transparent, rgba(255,255,255,0.1), transparent)',
          }} 
        />

        {/* Bottom section - Copyright */}
        <div style={{ 
          display: 'flex', 
          justifyContent: 'space-between', 
          alignItems: 'center', 
          width: '100%',
          flexWrap: 'wrap',
          gap: 16
        }}>
          <p style={{ 
            fontSize: 14, 
            color: 'rgba(255,255,255,0.6)', 
            margin: 0
          }}>
            Apache {new Date().getFullYear()} ©{' '}
            <a 
              href="https://resilientdb.incubator.apache.org/" 
              style={{ 
                color: 'rgba(255,255,255,0.6)',
                transition: 'all 200ms ease',
                textDecoration: 'none',
              }}
              onMouseEnter={(e) => {
                e.currentTarget.style.color = 'rgba(255,255,255,0.9)';
              }}
              onMouseLeave={(e) => {
                e.currentTarget.style.color = 'rgba(255,255,255,0.6)';
              }}
            >
              Apache Software Foundation
            </a>
          </p>
          
          <div style={{ display: 'flex', gap: 16, alignItems: 'center' }}>
            <span style={{ 
              fontSize: 12, 
              color: 'rgba(255,255,255,0.6)', 
              opacity: 0.7
            }}>
              Licensed under Apache 2.0
            </span>
            <div
              style={{
                width: 4,
                height: 4,
                borderRadius: '50%',
                background: 'rgba(255,255,255,0.3)',
                opacity: 0.5,
              }}
            />
            <span style={{ 
              fontSize: 12, 
              color: 'rgba(255,255,255,0.6)', 
              opacity: 0.7
            }}>
              Incubating Project
            </span>
          </div>
        </div>
      </div>
    </div>
  </footer>
);
