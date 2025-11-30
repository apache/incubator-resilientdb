"use client";

import { useState } from "react";
import { ResizablePanelGroup, ResizablePanel, ResizableHandle } from "@/components/ui/resizable";
import { Card, CardHeader, CardTitle, CardDescription, CardContent } from "@/components/ui/card";
import { Button } from "@/components/ui/button";
import { TutorPanel } from "./components/tutor-panel";
import { Loader2 } from "lucide-react";

export default function GraphQLTutorPage() {
  const [query, setQuery] = useState("");
  const [analysis, setAnalysis] = useState<any>(null);
  const [loading, setLoading] = useState(false);

  const handleAnalyze = async () => {
    if (!query.trim()) return;

    setLoading(true);
    try {
      const response = await fetch("/api/graphql-tutor/analyze", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ query }),
      });

      const data = await response.json();
      setAnalysis(data);
    } catch (error) {
      console.error("Error analyzing query:", error);
    } finally {
      setLoading(false);
    }
  };

  return (
    <div className="container mx-auto p-6 h-screen flex flex-col">
      <div className="mb-4">
        <h1 className="text-3xl font-bold">GraphQL Query Tutor</h1>
        <p className="text-muted-foreground">
          Get AI-powered explanations, optimizations, and efficiency analysis for your GraphQL queries
        </p>
      </div>

      <ResizablePanelGroup direction="horizontal" className="flex-1">
        <ResizablePanel defaultSize={50} minSize={30}>
          <Card className="h-full flex flex-col">
            <CardHeader>
              <CardTitle>GraphQL Query Editor</CardTitle>
              <CardDescription>
                Write and analyze your ResilientDB GraphQL queries
              </CardDescription>
            </CardHeader>
            <CardContent className="flex-1 flex flex-col">
              <textarea
                value={query}
                onChange={(e) => setQuery(e.target.value)}
                placeholder='Enter your GraphQL query, e.g., { getTransaction(id: "123") { asset } }'
                className="flex-1 w-full p-4 border rounded-md font-mono text-sm resize-none"
              />
              <Button
                onClick={handleAnalyze}
                disabled={loading || !query.trim()}
                className="mt-4"
              >
                {loading ? (
                  <>
                    <Loader2 className="mr-2 h-4 w-4 animate-spin" />
                    Analyzing...
                  </>
                ) : (
                  "Analyze Query"
                )}
              </Button>
            </CardContent>
          </Card>
        </ResizablePanel>

        <ResizableHandle />

        <ResizablePanel defaultSize={50} minSize={30}>
          <TutorPanel analysis={analysis} loading={loading} />
        </ResizablePanel>
      </ResizablePanelGroup>
    </div>
  );
}

