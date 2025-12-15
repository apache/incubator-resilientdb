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

import React, { useEffect, useRef } from 'react';

interface ShaderBackgroundProps {
  children: React.ReactNode;
}

export default function ShaderBackground({ children }: ShaderBackgroundProps) {
  const containerRef = useRef<HTMLDivElement>(null);
  const blobRef = useRef<HTMLDivElement>(null);

  // Lightweight pointer-based parallax, transform only (GPU-accelerated)
  useEffect(() => {
    const container = containerRef.current;
    const blob = blobRef.current;
    if (!container || !blob) return;

    let rafId = 0;
    const target = { x: 0, y: 0 };
    const current = { x: 0, y: 0 };

    const onMove = (e: PointerEvent) => {
      const rect = container.getBoundingClientRect();
      target.x = e.clientX - rect.left;
      target.y = e.clientY - rect.top;
      if (!rafId) rafId = requestAnimationFrame(tick);
    };

    const tick = () => {
      const lerp = 0.18;
      current.x += (target.x - current.x) * lerp;
      current.y += (target.y - current.y) * lerp;
      blob.style.left = `${current.x}px`;
      blob.style.top = `${current.y}px`;
      blob.style.opacity = '1';
      if (Math.abs(current.x - target.x) > 0.001 || Math.abs(current.y - target.y) > 0.001) {
        rafId = requestAnimationFrame(tick);
      } else {
        rafId = 0;
      }
    };

    const onLeave = () => {
      blob.style.opacity = '0';
    };

    container.addEventListener('pointermove', onMove, { passive: true });
    container.addEventListener('pointerleave', onLeave, { passive: true });
    return () => {
      container.removeEventListener('pointermove', onMove as any);
      container.removeEventListener('pointerleave', onLeave as any);
      if (rafId) cancelAnimationFrame(rafId);
    };
  }, []);

  return (
    <div ref={containerRef} style={{ minHeight: '100vh', background: '#000', position: 'relative', overflow: 'hidden' }}>
      {/* SVG Filters (used by header gooey/pill effects) */}
      <svg style={{ position: 'absolute', inset: 0, width: 0, height: 0 }}>
        <defs>
          <filter id="glass-effect" x="-50%" y="-50%" width="200%" height="200%">
            <feTurbulence baseFrequency="0.005" numOctaves="1" result="noise" />
            <feDisplacementMap in="SourceGraphic" in2="noise" scale="0.3" />
            <feColorMatrix type="matrix" values="1 0 0 0 0.02 0 1 0 0 0.02 0 0 1 0 0.05 0 0 0 0.9 0" result="tint" />
          </filter>
          <filter id="gooey-filter" x="-50%" y="-50%" width="200%" height="200%">
            <feGaussianBlur in="SourceGraphic" stdDeviation="4" result="blur" />
            <feColorMatrix in="blur" mode="matrix" values="1 0 0 0 0  0 1 0 0 0  0 0 1 0 0  0 0 0 19 -9" result="gooey" />
            <feComposite in="SourceGraphic" in2="gooey" operator="atop" />
          </filter>
        </defs>
      </svg>

      {/* Subtle radial overlays (dark, snappy) */}
      <div
        aria-hidden
        style={{
          position: 'absolute',
          inset: 0,
          pointerEvents: 'none',
          background:
            'radial-gradient(1200px 600px at 80% -10%, rgba(0, 102, 255, 0.06), transparent 60%),' +
            // Enhanced red hues for visual interest (still subtle)
            'radial-gradient(900px 450px at 15% 75%, rgba(255, 64, 64, 0.10), transparent 65%),' +
            'radial-gradient(700px 350px at 70% 30%, rgba(255, 64, 64, 0.06), transparent 70%),' +
            'radial-gradient(1000px 500px at -10% 10%, rgba(255, 0, 128, 0.06), transparent 60%),' +
            'radial-gradient(900px 400px at 50% 20%, rgba(255,255,255,0.03), transparent 70%),' +
            'linear-gradient(180deg, rgba(255, 255, 255, 0.01), rgba(255, 255, 255, 0.0))',
        }}
      />

      {/* Subtle grey hue overlay */}
      <div
        aria-hidden
        style={{
          position: 'absolute',
          inset: 0,
          pointerEvents: 'none',
          background:
            'radial-gradient(800px 400px at 20% 80%, rgba(148,163,184,0.06), transparent 70%),' +
            'radial-gradient(700px 350px at 80% 20%, rgba(148,163,184,0.04), transparent 70%)',
        }}
      />

      {/* Low-cost parallax blob */}
      <div ref={blobRef} className="parallax-blob" />

      {/* Beacon of light elements */}
      <div className="beacon-cone" style={{ pointerEvents: 'none' }} />
      <svg className="beacon-outline" viewBox="0 0 100 100" style={{ opacity: 0.6, pointerEvents: 'none' }}>
        <defs>
          <filter id="glow" x="-50%" y="-50%" width="200%" height="200%">
            <feGaussianBlur stdDeviation="2" result="coloredBlur" />
            <feMerge>
              <feMergeNode in="coloredBlur" />
              <feMergeNode in="SourceGraphic" />
            </feMerge>
          </filter>
        </defs>
        <path
          d="M50 12 L63 40 L50 68 L37 40 Z"
          fill="none"
          stroke="rgba(0,146,255,0.55)"
          strokeWidth="1"
          filter="url(#glow)"
        />
      </svg>
      <div className="beacon-source" />

      {children}
    </div>
  );
}


