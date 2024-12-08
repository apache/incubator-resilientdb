"use client"

import React, { useEffect, useState, useRef } from 'react'
import './PBFTVisualization.css'
import FullScreenButton from './FullScreenButton'

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
  const [isVisible, setIsVisible] = useState(false)
  const [animationKey, setAnimationKey] = useState(0)
  const containerRef = useRef<HTMLDivElement>(null)
  const nodes = ['Client', 'Primary', 'Node 1', 'Node 2', 'Node 3']
  const phases = ['Request', 'Pre-prepare', 'Prepare', 'Commit', 'Reply']
  
  const messages: Message[] = [
    // Request phase
    { id: 'req1', from: 'Client', to: 'Primary', phase: 'Request', color: '#FFA726', delay: 0, duration: 2 },
    
    // Pre-prepare phase
    { id: 'pp1', from: 'Primary', to: 'Node 1', phase: 'Pre-prepare', color: '#4CAF50', delay: 2, duration: 2 },
    { id: 'pp2', from: 'Primary', to: 'Node 2', phase: 'Pre-prepare', color: '#4CAF50', delay: 2, duration: 2 },
    { id: 'pp3', from: 'Primary', to: 'Node 3', phase: 'Pre-prepare', color: '#4CAF50', delay: 2, duration: 2 },
    
    // Prepare phase
    { id: 'p1', from: 'Node 1', to: 'Primary', phase: 'Prepare', color: '#29B6F6', delay: 4, duration: 2 },
    { id: 'p2', from: 'Node 1', to: 'Node 2', phase: 'Prepare', color: '#29B6F6', delay: 4, duration: 2 },
    { id: 'p3', from: 'Node 1', to: 'Node 3', phase: 'Prepare', color: '#29B6F6', delay: 4, duration: 2 },
    { id: 'p4', from: 'Node 2', to: 'Primary', phase: 'Prepare', color: '#29B6F6', delay: 4, duration: 2 },
    { id: 'p5', from: 'Node 2', to: 'Node 1', phase: 'Prepare', color: '#29B6F6', delay: 4, duration: 2 },
    { id: 'p6', from: 'Node 2', to: 'Node 3', phase: 'Prepare', color: '#29B6F6', delay: 4, duration: 2 },
    { id: 'p7', from: 'Node 3', to: 'Primary', phase: 'Prepare', color: '#29B6F6', delay: 4, duration: 2 },
    { id: 'p8', from: 'Node 3', to: 'Node 1', phase: 'Prepare', color: '#29B6F6', delay: 4, duration: 2 },
    { id: 'p9', from: 'Node 3', to: 'Node 2', phase: 'Prepare', color: '#29B6F6', delay: 4, duration: 2 },
    
    // Commit phase
    { id: 'c1', from: 'Primary', to: 'Node 1', phase: 'Commit', color: '#EF5350', delay: 6, duration: 2 },
    { id: 'c2', from: 'Primary', to: 'Node 2', phase: 'Commit', color: '#EF5350', delay: 6, duration: 2 },
    { id: 'c3', from: 'Primary', to: 'Node 3', phase: 'Commit', color: '#EF5350', delay: 6, duration: 2 },
    { id: 'c4', from: 'Node 1', to: 'Primary', phase: 'Commit', color: '#EF5350', delay: 6, duration: 2 },
    { id: 'c5', from: 'Node 1', to: 'Node 2', phase: 'Commit', color: '#EF5350', delay: 6, duration: 2 },
    { id: 'c6', from: 'Node 1', to: 'Node 3', phase: 'Commit', color: '#EF5350', delay: 6, duration: 2 },
    { id: 'c7', from: 'Node 2', to: 'Primary', phase: 'Commit', color: '#EF5350', delay: 6, duration: 2 },
    { id: 'c8', from: 'Node 2', to: 'Node 1', phase: 'Commit', color: '#EF5350', delay: 6, duration: 2 },
    { id: 'c9', from: 'Node 2', to: 'Node 3', phase: 'Commit', color: '#EF5350', delay: 6, duration: 2 },
    { id: 'c10', from: 'Node 3', to: 'Primary', phase: 'Commit', color: '#EF5350', delay: 6, duration: 2 },
    { id: 'c11', from: 'Node 3', to: 'Node 1', phase: 'Commit', color: '#EF5350', delay: 6, duration: 2 },
    { id: 'c12', from: 'Node 3', to: 'Node 2', phase: 'Commit', color: '#EF5350', delay: 6, duration: 2 },
  ]

  useEffect(() => {
    const observer = new IntersectionObserver(
      ([entry]) => {
        if (entry.isIntersecting) {
          setIsVisible(true)
          setAnimationKey(prev => prev + 1) // Reset animation when becoming visible
        } else {
          setIsVisible(false)
        }
      },
      { threshold: 0.7 } // Trigger when 70% of the component is visible
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

  const handleReplay = () => {
    setAnimationKey(prev => prev + 1)
  }

  return (
    <div className="pbft-container" ref={containerRef}>
      <div className="pbft-visualization">
        <svg viewBox="0 0 1200 620" className="pbft-svg" key={animationKey}>
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
              />
              <circle 
                cx="100" 
                cy={getNodeY(node)} 
                r="20" 
                className={`node ${node === 'Client' ? 'client' : node === 'Primary' ? 'primary' : 'secondary'}`}
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
          {isVisible && messages.map((msg) => (
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
                <animate
                  attributeName="opacity"
                  from="0"
                  to="0.8"
                  dur="0.1s"
                  begin={`${msg.delay}s`}
                  fill="freeze"
                />
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
          {isVisible && (
            <g className="reply-combination">
              {['Primary', 'Node 1', 'Node 2', 'Node 3'].map((node, index) => (
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
                    <animate
                      attributeName="opacity"
                      from="0"
                      to="0.8"
                      dur="0.1s"
                      begin={`${8 + index * 0.2}s`}
                      fill="freeze"
                    />
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
            onClick={handleReplay}
          >
            <circle cx="600" cy="600" r="24" fill="#2a2a2a" />
            <path
              d="M 600 588 A 12 12 0 1 1 588 600 L 588 596"
              fill="none"
              stroke="#ffffff"
              strokeWidth="2"
              strokeLinecap="round"
              strokeLinejoin="round"
            />
            <path
              d="M 584 600 L 588 596 L 592 600"
              fill="none"
              stroke="#ffffff"
              strokeWidth="2"
              strokeLinecap="round"
              strokeLinejoin="round"
            />
          </g>
        </svg>
      </div>
      <FullScreenButton/>
    </div>
  )
}

