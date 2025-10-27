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
*
*/

import { useEffect, useRef, useState } from 'react'
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from "@/components/ui/card"
import PBFTVisualization from "@/components/ui/PBFT graph/PBFTVisualization"
import Navbar from "@/components/ui/navbar"

export default function PBFTFullView() {
  const [isVisible, setIsVisible] = useState(false)
  const containerRef = useRef<HTMLDivElement>(null)

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

  return (
    <div className="min-h-screen bg-gray-900 p-4 text-white">
      <Navbar />
      <div className="mt-16">
        <h1 className="text-3xl font-bold text-center mb-8">
          PBFT Consensus Visualization
        </h1>
        <div className="max-w-6xl mx-auto">
          <Card className="bg-gray-800 text-white mb-8">
            <CardHeader>
              <CardTitle>Practical Byzantine Fault Tolerance (PBFT) Protocol</CardTitle>
              <CardDescription className="text-gray-300">
                A visualization of the consensus process in a distributed system
              </CardDescription>
            </CardHeader>
            <CardContent>
              <div ref={containerRef} className={`pbft-visualization ${isVisible ? 'visible' : ''}`}>
                <PBFTVisualization />
              </div>
            </CardContent>
          </Card>
          <Card className="bg-gray-800 text-white">
            <CardHeader>
              <CardTitle>Legend and Explanation</CardTitle>
            </CardHeader>
            <CardContent>
              <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
                <div>
                  <h3 className="text-xl font-semibold mb-2">Color Legend</h3>
                  <div className="space-y-2">
                    <div className="flex items-center">
                      <div className="w-4 h-4 bg-yellow-500 mr-2"></div>
                      <span>Client</span>
                    </div>
                    <div className="flex items-center">
                      <div className="w-4 h-4 bg-green-500 mr-2"></div>
                      <span>Primary Node</span>
                    </div>
                    <div className="flex items-center">
                      <div className="w-4 h-4 bg-blue-500 mr-2"></div>
                      <span>Secondary Nodes</span>
                    </div>
                    <div className="flex items-center">
                      <div className="w-4 h-4 bg-green-500 mr-2"></div>
                      <span>Request</span>
                    </div>
                    <div className="flex items-center">
                      <div className="w-4 h-4 bg-orange-400 mr-2"></div>
                      <span>Pre-prepare</span>
                    </div>
                    <div className="flex items-center">
                      <div className="w-4 h-4 bg-blue-400 mr-2"></div>
                      <span>Prepare</span>
                    </div>
                    <div className="flex items-center">
                      <div className="w-4 h-4 bg-red-400 mr-2"></div>
                      <span>Commit</span>
                    </div>
                    <div className="flex items-center">
                      <div className="w-4 h-4 bg-purple-400 mr-2"></div>
                      <span>Reply</span>
                    </div>
                  </div>
                </div>
                <div>
                  <h3 className="text-xl font-semibold mb-2">PBFT Process</h3>
                  <ol className="list-decimal list-inside space-y-2">
                    <li>Client sends a request to the primary node</li>
                    <li>Primary broadcasts pre-prepare messages</li>
                    <li>Nodes exchange prepare messages</li>
                    <li>Nodes exchange commit messages</li>
                    <li>Nodes send reply messages to the client</li>
                  </ol>
                </div>
                <div>
                  <h3 className="text-xl font-semibold mb-2">Replay Animation</h3>
                  <p>Click the replay button at the bottom of the visualization to restart the animation.</p>
                </div>
              </div>
            </CardContent>
          </Card>
        </div>
      </div>
    </div>
  )
}
