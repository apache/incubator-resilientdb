'use client';

export default function HeroContent() {
  return (
    <main
      style={{
        position: 'absolute',
        inset: 0,
        zIndex: 20,
        display: 'grid',
        placeItems: 'center',
        padding: 24,
      }}
    >
      <div style={{ textAlign: 'center', maxWidth: 900 }}>
        <div
          style={{
            display: 'inline-flex',
            alignItems: 'center',
            paddingInline: 12,
            paddingBlock: 6,
            borderRadius: 999,
            background: 'rgba(255,255,255,0.05)',
            backdropFilter: 'blur(6px)',
            marginBottom: 16,
            filter: 'url(#glass-effect)',
            position: 'relative',
          }}
        >
          <div style={{ position: 'absolute', top: 0, left: 4, right: 4, height: 1, background: 'linear-gradient(90deg, transparent, rgba(255,255,255,0.2), transparent)', borderRadius: 999 }} />
          <span style={{ color: 'rgba(255,255,255,0.9)', fontSize: 12, fontWeight: 300, position: 'relative', zIndex: 10 }}>
            ✨ New Beacon Experience
          </span>
        </div>

        <h1 style={{ fontSize: '56px', lineHeight: 1.15, letterSpacing: '-0.02em', fontWeight: 600, color: '#fff', marginBottom: 12 }}>
          Next Gen Docs for
          <br />
          <span style={{ fontWeight: 300, color: '#fff' }}>ResilientDB</span>
        </h1>

        <p style={{ fontSize: 14, fontWeight: 300, color: 'rgba(255,255,255,0.7)', marginBottom: 16, lineHeight: 1.7 }}>
        Find all documentation related to ResilientDB, its applications, and ecosystem tools supported by the ResilientDB team. This site is your gateway to high-performance blockchain infrastructure, developer guides, and integration resources. To learn more about ResilientDB, visit the 
        <span style={{ fontSize: 14, fontWeight: 300, color: 'deepskyblue'}}><a href="https://resilientdb.incubator.apache.org/" target="_blank"> official website.</a></span>
        </p>

        <div style={{ display: 'flex', alignItems: 'center', justifyContent: 'center', gap: 16, flexWrap: 'wrap' }}>
          <button 
            onClick={() => window.open('https://github.com/apache/incubator-resilientdb/releases/tag/v1.10.0-rc03', '_blank')}
            style={{ 
              paddingInline: 24, 
              paddingBlock: 12, 
              borderRadius: 999, 
              background: 'transparent', 
              border: '1px solid rgba(255,255,255,0.3)', 
              color: '#fff', 
              fontWeight: 400, 
              fontSize: 12, 
              transition: 'all 200ms ease',
              cursor: 'pointer',
              fontFamily: 'inherit'
            }}
            onMouseEnter={(e) => {
              e.currentTarget.style.background = 'rgba(255,255,255,0.1)';
              e.currentTarget.style.borderColor = 'rgba(255,255,255,0.5)';
            }}
            onMouseLeave={(e) => {
              e.currentTarget.style.background = 'transparent';
              e.currentTarget.style.borderColor = 'rgba(255,255,255,0.3)';
            }}
          >
            Latest Release
          </button>
          <button 
            onClick={() => window.location.href = '/docs'}
            style={{ 
              paddingInline: 24, 
              paddingBlock: 12, 
              borderRadius: 999, 
              background: '#fff', 
              border: '1px solid #fff', 
              color: '#000', 
              fontWeight: 400, 
              fontSize: 12, 
              transition: 'all 200ms ease',
              cursor: 'pointer',
              fontFamily: 'inherit'
            }}
            onMouseEnter={(e) => {
              e.currentTarget.style.background = 'rgba(255,255,255,0.9)';
              e.currentTarget.style.borderColor = 'rgba(255,255,255,0.9)';
            }}
            onMouseLeave={(e) => {
              e.currentTarget.style.background = '#fff';
              e.currentTarget.style.borderColor = '#fff';
            }}
          >
            Get Started
          </button>
        </div>
      </div>
    </main>
  );
}


