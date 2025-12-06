"use client";

import { Card, CardHeader, CardTitle, CardContent } from "@/components/ui/card";
import { Tabs, TabsContent, TabsList, TabsTrigger } from "@/components/ui/tabs";
import { ExplanationPanel } from "./explanation-panel";
import { OptimizationPanel } from "./optimization-panel";
import { EfficiencyDisplay } from "./efficiency-display";
import { Loader2, ExternalLink } from "lucide-react";
import { useState, useEffect } from "react";

interface TutorPanelProps {
  analysis: any;
  loading: boolean;
}

export function TutorPanel({ analysis, loading }: TutorPanelProps) {
  const [activeTab, setActiveTab] = useState("explanation");
  
  // ResLens is always on port 5173 (local dev server only)
  // Hardcode the URL to avoid any port detection issues
  const reslensUrl = "http://localhost:5173";

  const handleEfficiencyTabClick = (e: React.MouseEvent) => {
    e.preventDefault();
    // Open ResLens dashboard with Query Stats tab active
    const reslensDashboardUrl = `${reslensUrl}/dashboard?tab=query_stats`;
    console.log("Opening ResLens at:", reslensDashboardUrl);
    window.open(reslensDashboardUrl, "_blank", "noopener,noreferrer");
    // Also switch to efficiency tab to show the content
    setActiveTab("efficiency");
  };

  if (loading) {
    return (
      <Card className="h-full flex items-center justify-center">
        <Loader2 className="h-8 w-8 animate-spin" />
      </Card>
    );
  }

  if (!analysis) {
    return (
      <Card className="h-full flex items-center justify-center">
        <p className="text-muted-foreground">
          Enter a query and click "Analyze Query" to get started
        </p>
      </Card>
    );
  }

  return (
    <Card className="h-full flex flex-col">
      <CardHeader>
        <CardTitle>AI Tutor</CardTitle>
        <p className="text-sm text-muted-foreground">
          Get explanations, optimizations, and efficiency analysis
        </p>
      </CardHeader>
      <CardContent className="flex-1 overflow-auto">
        <Tabs value={activeTab} onValueChange={setActiveTab} className="w-full">
          <TabsList className="grid w-full grid-cols-3">
            <TabsTrigger value="explanation">Explanation</TabsTrigger>
            <TabsTrigger value="optimization">Optimization</TabsTrigger>
            <TabsTrigger 
              value="efficiency"
              onClick={handleEfficiencyTabClick}
              className="flex items-center gap-2"
            >
              Efficiency
              <ExternalLink className="h-3 w-3" />
            </TabsTrigger>
          </TabsList>
          <TabsContent value="explanation" className="mt-4">
            <ExplanationPanel explanation={analysis.explanation} />
          </TabsContent>
          <TabsContent value="optimization" className="mt-4">
            <OptimizationPanel optimizations={analysis.optimizations} />
          </TabsContent>
          <TabsContent value="efficiency" className="mt-4">
            <div className="space-y-4">
              <div className="flex items-center justify-between p-4 bg-muted rounded-lg">
                <div>
                  <p className="font-semibold">ResLens Dashboard Opened</p>
                  <p className="text-sm text-muted-foreground">
                    Query statistics and performance metrics are available in the ResLens Query Stats tab
                  </p>
                </div>
                <a
                  href={`${reslensUrl}/dashboard?tab=query_stats`}
                  target="_blank"
                  rel="noopener noreferrer"
                  className="text-sm text-primary hover:underline flex items-center gap-1"
                >
                  Open Query Stats <ExternalLink className="h-3 w-3" />
                </a>
              </div>
              <EfficiencyDisplay efficiency={analysis.efficiency} />
            </div>
          </TabsContent>
        </Tabs>
      </CardContent>
    </Card>
  );
}

