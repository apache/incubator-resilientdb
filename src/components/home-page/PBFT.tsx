import React from "react";
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from "@/components/ui/card";
import PBFTVisualization from "@/components/ui/PBFT graph/PBFTVisualization"; // Adjust path if needed

const PBFTPage = () => {
  return (
    <div className="min-h-screen bg-gray-900 p-4 text-white">
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
            <PBFTVisualization />
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
            </div>
          </CardContent>
        </Card>
      </div>
    </div>
  );
};

export default PBFTPage;
