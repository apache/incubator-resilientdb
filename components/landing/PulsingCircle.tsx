'use client';

import { PulsingBorder } from '@paper-design/shaders-react';
import { motion } from 'framer-motion';

export default function PulsingCircle() {
  return (
    <div style={{ position: 'absolute', bottom: 32, right: 32, zIndex: 30 }}>
      <div style={{ position: 'relative', width: 80, height: 80, display: 'flex', alignItems: 'center', justifyContent: 'center' }}>
        <PulsingBorder
          colors={[ '#BEECFF', '#E77EDC', '#FF4C3E', '#00FF88', '#FFD700', '#FF6B35', '#8A2BE2' ]}
          colorBack="#00000000"
          speed={1.5}
          roundness={1}
          thickness={0.1}
          softness={0.2}
          intensity={5}
          spotSize={0.1}
          pulse={0.1}
          smoke={0.5}
          smokeSize={4}
          scale={0.65}
          rotation={0}
          frame={9161408.251009725}
          style={{ width: 60, height: 60, borderRadius: '50%' }}
        />

        <motion.svg
          style={{ position: 'absolute', inset: 0, width: '100%', height: '100%', transform: 'scale(1.6)' }}
          viewBox="0 0 100 100"
          animate={{ rotate: 360 }}
          transition={{ duration: 20, repeat: Number.POSITIVE_INFINITY, ease: 'linear' }}
        >
          <defs>
            <path id="circle" d="M 50, 50 m -38, 0 a 38,38 0 1,1 76,0 a 38,38 0 1,1 -76,0" />
          </defs>
          <text style={{ fontSize: 10, fill: 'rgba(255,255,255,0.8)' }}>
            <textPath href="#circle" startOffset="0%">
              Beacon • Resilient • Modern • Elegant •
            </textPath>
          </text>
        </motion.svg>
      </div>
    </div>
  );
}







