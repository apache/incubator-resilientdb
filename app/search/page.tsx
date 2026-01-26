'use client'

import { PagefindSearch } from '../../components/SearchBar/PagefindSearch';

export default function SearchPage() {
  return (
    <div style={{ 
      minHeight: '100vh', 
      background: 'transparent',
      padding: '20px'
    }}>
      <div style={{ maxWidth: 800, margin: '0 auto' }}>
        <h1 style={{ 
          color: 'rgba(255, 255, 255, 0.9)', 
          marginBottom: '20px',
          fontSize: '2rem',
          fontWeight: 600
        }}>
          Search Documentation
        </h1>
        <PagefindSearch />
      </div>
    </div>
  );
}
