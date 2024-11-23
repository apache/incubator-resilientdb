"use client"

import { useEffect, useState, useRef } from 'react'
import './PBFTVisualization.css'

interface Message {
  id: string
  from: string
  to: string
  phase: string
  color: string
  delay: number
  duration: number
}

export default function PBFTVisualization() {
  const [animationStarted, setAnimationStarted] = useState(false)
  const [isVisible, setIsVisible] = useState(false)
  const [animationKey, setAnimationKey] = useState(0)
  const containerRef = useRef<HTMLDivElement>(null)
  const nodes = ['Client', 'Node 0', 'Node 1', 'Node 2', 'Node 3']
  const phases = ['Request', 'Pre-prepare', 'Prepare', 'Commit', 'Reply']
  
  const messages: Message[] = [
    // Request phase
    { id: 'req1', from: 'Client', to: 'Node 0', phase: 'Request', color: '#4CAF50', delay: 0, duration: 2 },
    
    // Pre-prepare phase
    { id: 'pp1', from: 'Node 0', to: 'Node 1', phase: 'Pre-prepare', color: '#FFA726', delay: 2, duration: 2 },
    { id: 'pp2', from: 'Node 0', to: 'Node 2', phase: 'Pre-prepare', color: '#FFA726', delay: 2, duration: 2 },
    { id: 'pp3', from: 'Node 0', to: 'Node 3', phase: 'Pre-prepare', color: '#FFA726', delay: 2, duration: 2 },
    
    // Prepare phase
    { id: 'p1', from: 'Node 1', to: 'Node 0', phase: 'Prepare', color: '#29B6F6', delay: 4, duration: 2 },
    { id: 'p2', from: 'Node 1', to: 'Node 2', phase: 'Prepare', color: '#29B6F6', delay: 4, duration: 2 },
    { id: 'p3', from: 'Node 1', to: 'Node 3', phase: 'Prepare', color: '#29B6F6', delay: 4, duration: 2 },
    { id: 'p4', from: 'Node 2', to: 'Node 0', phase: 'Prepare', color: '#29B6F6', delay: 4, duration: 2 },
    { id: 'p5', from: 'Node 2', to: 'Node 1', phase: 'Prepare', color: '#29B6F6', delay: 4, duration: 2 },
    { id: 'p6', from: 'Node 2', to: 'Node 3', phase: 'Prepare', color: '#29B6F6', delay: 4, duration: 2 },
    
    // Commit phase
    { id: 'c1', from: 'Node 0', to: 'Node 1', phase: 'Commit', color: '#EF5350', delay: 6, duration: 2 },
    { id: 'c2', from: 'Node 0', to: 'Node 2', phase: 'Commit', color: '#EF5350', delay: 6, duration: 2 },
    { id: 'c3', from: 'Node 0', to: 'Node 3', phase: 'Commit', color: '#EF5350', delay: 6, duration: 2 },
    { id: 'c4', from: 'Node 1', to: 'Node 0', phase: 'Commit', color: '#EF5350', delay: 6, duration: 2 },
    { id: 'c5', from: 'Node 1', to: 'Node 2', phase: 'Commit', color: '#EF5350', delay: 6, duration: 2 },
    { id: 'c6', from: 'Node 1', to: 'Node 3', phase: 'Commit', color: '#EF5350', delay: 6, duration: 2 },
    { id: 'c7', from: 'Node 2', to: 'Node 0', phase: 'Commit', color: '#EF5350', delay: 6, duration: 2 },
    { id: 'c8', from: 'Node 2', to: 'Node 1', phase: 'Commit', color: '#EF5350', delay: 6, duration: 2 },
    { id: 'c9', from: 'Node 2', to: 'Node 3', phase: 'Commit', color: '#EF5350', delay: 6, duration: 2 },
    
  ]

  useEffect(() => {
    const observer = new IntersectionObserver(
      ([entry]) => {
        if (entry.isIntersecting) {
          setIsVisible(true)
        }
      },
      { threshold: 0.1 } // Trigger when 10% of the component is visible
    )

    if (containerRef.current) {
      observer.observe(containerRef.current)
    }

    return () => {
      if (containerRef.current) {
        observer.unobserve(containerRef.current)
      }
    }
  }, [])

  useEffect(() => {
    if (isVisible) {
      const timer = setTimeout(() => setAnimationStarted(true), 500)
      return () => clearTimeout(timer)
    }
  }, [isVisible, animationKey])

  const getNodeY = (node: string) => {
    const index = nodes.indexOf(node)
    return 100 + index * 100
  }

  const getMessagePath = (from: string, to: string, phase: string) => {
    const fromY = getNodeY(from)
    const toY = getNodeY(to)
    const phaseIndex = phases.indexOf(phase)
    const startX = 150 + phaseIndex * 200
    const endX = startX + 200
    const midX = (startX + endX) / 2
    
    return `M ${startX} ${fromY} C ${midX} ${fromY}, ${midX} ${toY}, ${endX} ${toY}`
  }

  return (
    <div ref={containerRef} className="pbft-visualization">
      <svg viewBox="0 0 1200 600" className="pbft-svg" key={animationKey}>
        {/* Background grid */}
        <defs>
          <pattern id="grid" width="50" height="50" patternUnits="userSpaceOnUse">
            <path d="M 50 0 L 0 0 0 50" fill="none" stroke="rgba(255,255,255,0.1)" strokeWidth="0.5"/>
          </pattern>
        </defs>
        <rect width="100%" height="100%" fill="url(#grid)" />

        {/* Phase columns */}
        {phases.map((phase, index) => (
          <g key={phase}>
            <line
              x1={150 + index * 200}
              y1="50"
              x2={150 + index * 200}
              y2="550"
              className="phase-line"
            />
            <text
              x={250 + index * 200}
              y="30"
              className="phase-label"
              textAnchor="middle"
            >
              {phase}
            </text>
          </g>
        ))}

        {/* Nodes */}
        {nodes.map((node, index) => (
          <g key={node}>
            <line 
              x1="150" 
              y1={getNodeY(node)} 
              x2="1050" 
              y2={getNodeY(node)} 
              className="timeline"
              strokeDasharray={node === 'Node 3' ? '5,5' : 'none'}
            />
            <circle 
              cx="100" 
              cy={getNodeY(node)} 
              r="20" 
              className={`node ${node === 'Node 0' ? 'primary' : 'secondary'} ${node === 'Node 3' ? 'faulty' : ''}`}
            />
            <text 
              x="50" 
              y={getNodeY(node)} 
              className="node-label"
            >
              {node}
            </text>
          </g>
        ))}

        {/* Messages */}
        {animationStarted && messages.map((msg) => (
          <g key={msg.id} className="message">
            <path 
              d={getMessagePath(msg.from, msg.to, msg.phase)} 
              stroke={msg.color}
              className="message-path"
            >
              <animate
                attributeName="stroke-dashoffset"
                from="1000"
                to="0"
                dur={`${msg.duration}s`}
                begin={`${msg.delay}s`}
                fill="freeze"
              />
            </path>
            <circle r="4" fill={msg.color} className="message-dot">
              <animateMotion
                path={getMessagePath(msg.from, msg.to, msg.phase)}
                dur={`${msg.duration}s`}
                begin={`${msg.delay}s`}
                fill="freeze"
              />
            </circle>
          </g>
        ))}

        {/* Reply phase */}
        {animationStarted && (
          <g className="reply-combination">
            {['Node 0', 'Node 1', 'Node 2'].map((node, index) => (
              <g key={`reply-${node}`}>
                <path
                  d={`M 950 ${getNodeY(node)} C 1000 ${getNodeY(node)}, 1000 ${getNodeY('Client')}, 1050 ${getNodeY('Client')}`}
                  className="reply-path"
                  stroke="#AB47BC"
                >
                  <animate
                    attributeName="stroke-dashoffset"
                    from="1000"
                    to="0"
                    dur="2s"
                    begin={`${8 + index * 0.2}s`}
                    fill="freeze"
                  />
                </path>
                <circle r="4" fill="#AB47BC">
                  <animateMotion
                    path={`M 950 ${getNodeY(node)} C 1000 ${getNodeY(node)}, 1000 ${getNodeY('Client')}, 1050 ${getNodeY('Client')}`}
                    dur="2s"
                    begin={`${8 + index * 0.2}s`}
                    fill="freeze"
                  />
                </circle>
              </g>
            ))}
          </g>
        )}

        {/* Replay button */}
        <g
          className="replay-button"
          onClick={() => {
            setAnimationStarted(false)
            setAnimationKey(prev => prev + 1)
            setTimeout(() => setAnimationStarted(true), 100)
          }}
        >
          <circle cx="600" cy="580" r="20" fill="#4CAF50" />
          <path
            d="M 595 570 L 595 590 L 610 580 Z"
            fill="white"
          />
        </g>
      </svg>
    </div>
  )
}

