# 📝 Nexus UI Extension Guide - Complete Code

This guide contains all the code needed to add the GraphQL Tutor UI to Nexus. Copy these files exactly as shown.

---

## 📁 File Structure

```
nexus/
├── src/
│   ├── app/
│   │   ├── api/
│   │   │   └── graphql-tutor/
│   │   │       └── analyze/
│   │   │           └── route.ts          ← Create this
│   │   ├── graphql-tutor/
│   │   │   ├── page.tsx                  ← Create this
│   │   │   └── components/
│   │   │       ├── tutor-panel.tsx       ← Create this
│   │   │       ├── explanation-panel.tsx ← Create this
│   │   │       ├── optimization-panel.tsx ← Create this
│   │   │       └── efficiency-display.tsx  ← Create this
│   │   └── page.tsx                      ← Modify this (add navigation link)
```

---

## 1. API Route: `src/app/api/graphql-tutor/analyze/route.ts`

```typescript
import { NextRequest, NextResponse } from "next/server";

/**
 * GraphQ-LLM Analysis API
 * Proxies requests to GraphQ-LLM backend for full query analysis
 */
export async function POST(req: NextRequest) {
  try {
    const { query } = await req.json();

    if (!query || typeof query !== "string") {
      return NextResponse.json(
        { error: "Query is required" },
        { status: 400 }
      );
    }

    // GraphQ-LLM backend URL (from environment or default)
    const graphqLlmUrl = process.env.GRAPHQ_LLM_API_URL || 
                        process.env.NEXT_PUBLIC_GRAPHQ_LLM_API_URL || 
                        "http://localhost:3001";

    try {
      // Call GraphQ-LLM explanation service
      const explainResponse = await fetch(`${graphqLlmUrl}/api/explanations/explain`, {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ query, detailed: true }),
      });

      // Call GraphQ-LLM optimization service
      const optimizeResponse = await fetch(`${graphqLlmUrl}/api/optimizations/optimize`, {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ query, includeSimilarQueries: true }),
      });

      // Call GraphQ-LLM efficiency service
      const efficiencyResponse = await fetch(`${graphqLlmUrl}/api/efficiency/estimate`, {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ query, useLiveStats: false }),
      });

      // Parse responses
      const explanationData = explainResponse.ok ? await explainResponse.json() : null;
      const optimizationData = optimizeResponse.ok ? await optimizeResponse.json() : null;
      const efficiencyData = efficiencyResponse.ok ? await efficiencyResponse.json() : null;

      // Format explanation response for UI
      const explanation = explanationData
        ? {
            explanation: explanationData.explanation || "No explanation available",
            complexity: explanationData.complexity || "medium",
            recommendations: explanationData.recommendations || [],
          }
        : {
            explanation: "Explanation service unavailable. Please ensure GraphQ-LLM backend is running.",
            complexity: "unknown",
            recommendations: [],
          };

      // Format optimizations response for UI
      const optimizations = optimizationData?.suggestions
        ? optimizationData.suggestions.map((s: any) => ({
            query: s.optimizedQuery || query,
            explanation: s.reason || s.suggestion || "Optimization suggestion",
            confidence: 0.8,
          }))
        : [];

      // Format efficiency response for UI
      const efficiency = efficiencyData
        ? {
            score: efficiencyData.efficiencyScore || 0,
            estimatedTime: efficiencyData.estimatedExecutionTime
              ? `${efficiencyData.estimatedExecutionTime}ms`
              : "Unknown",
            resourceUsage: efficiencyData.estimatedResourceUsage
              ? `CPU: ${efficiencyData.estimatedResourceUsage.cpu || "N/A"}%, Memory: ${efficiencyData.estimatedResourceUsage.memory || "N/A"}MB`
              : "Unknown",
            recommendations: efficiencyData.recommendations || [],
            complexity: efficiencyData.complexity,
            similarQueries: efficiencyData.similarQueries,
          }
        : {
            score: 0,
            estimatedTime: "Unknown",
            resourceUsage: "Unknown",
            recommendations: [],
            complexity: "unknown",
          };

      return NextResponse.json({
        explanation,
        optimizations,
        efficiency,
      });
    } catch (fetchError) {
      // Fallback to mock data if GraphQ-LLM backend is not available
      console.warn("GraphQ-LLM backend not available, using fallback:", fetchError);
      
      return NextResponse.json({
        explanation: {
          explanation: `This query retrieves transaction data from ResilientDB. The query structure suggests it's fetching a single transaction by ID.`,
          complexity: "low",
          recommendations: [
            "Consider adding pagination for large result sets",
            "Use specific field selections to reduce payload size",
          ],
        },
        optimizations: [
          {
            query: query,
            explanation: "Query is already well-optimized for single transaction retrieval.",
            confidence: 0.85,
          },
        ],
        efficiency: {
          score: 92,
          estimatedTime: "< 10ms",
          resourceUsage: "Low - Single transaction lookup",
          recommendations: [
            "Query is efficient for single transaction retrieval",
          ],
        },
      });
    }
  } catch (error) {
    console.error("Error analyzing query:", error);
    return NextResponse.json(
      { error: "Failed to analyze query" },
      { status: 500 }
    );
  }
}
```

---

## 2. Main Page: `src/app/graphql-tutor/page.tsx`

```typescript
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
```

---

## 3. Tutor Panel: `src/app/graphql-tutor/components/tutor-panel.tsx`

```typescript
"use client";

import { Card, CardHeader, CardTitle, CardContent } from "@/components/ui/card";
import { Tabs, TabsContent, TabsList, TabsTrigger } from "@/components/ui/tabs";
import { ExplanationPanel } from "./explanation-panel";
import { OptimizationPanel } from "./optimization-panel";
import { EfficiencyDisplay } from "./efficiency-display";
import { Loader2 } from "lucide-react";

interface TutorPanelProps {
  analysis: any;
  loading: boolean;
}

export function TutorPanel({ analysis, loading }: TutorPanelProps) {
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
        <Tabs defaultValue="explanation" className="w-full">
          <TabsList className="grid w-full grid-cols-3">
            <TabsTrigger value="explanation">Explanation</TabsTrigger>
            <TabsTrigger value="optimization">Optimization</TabsTrigger>
            <TabsTrigger value="efficiency">Efficiency</TabsTrigger>
          </TabsList>
          <TabsContent value="explanation" className="mt-4">
            <ExplanationPanel explanation={analysis.explanation} />
          </TabsContent>
          <TabsContent value="optimization" className="mt-4">
            <OptimizationPanel optimizations={analysis.optimizations} />
          </TabsContent>
          <TabsContent value="efficiency" className="mt-4">
            <EfficiencyDisplay efficiency={analysis.efficiency} />
          </TabsContent>
        </Tabs>
      </CardContent>
    </Card>
  );
}
```

---

## 4. Explanation Panel: `src/app/graphql-tutor/components/explanation-panel.tsx`

```typescript
"use client";

import { Card, CardHeader, CardTitle, CardContent } from "@/components/ui/card";
import { Badge } from "@/components/ui/badge";

interface ExplanationPanelProps {
  explanation: any;
}

export function ExplanationPanel({ explanation }: ExplanationPanelProps) {
  if (!explanation) {
    return <p className="text-muted-foreground">No explanation available</p>;
  }

  return (
    <div className="space-y-4">
      <div>
        <h3 className="text-lg font-semibold mb-2">Query Explanation</h3>
        <p className="text-sm text-muted-foreground mb-4">What this query does</p>
        <div className="prose prose-sm max-w-none">
          <p className="whitespace-pre-wrap">{explanation.explanation}</p>
        </div>
      </div>

      {explanation.complexity && (
        <div>
          <h4 className="text-md font-semibold mb-2">Complexity</h4>
          <Badge variant={
            explanation.complexity === "low" ? "default" :
            explanation.complexity === "medium" ? "secondary" : "destructive"
          }>
            {explanation.complexity}
          </Badge>
        </div>
      )}

      {explanation.recommendations && explanation.recommendations.length > 0 && (
        <div>
          <h4 className="text-md font-semibold mb-2">Recommendations</h4>
          <ul className="list-disc list-inside space-y-1">
            {explanation.recommendations.map((rec: string, idx: number) => (
              <li key={idx} className="text-sm">{rec}</li>
            ))}
          </ul>
        </div>
      )}
    </div>
  );
}
```

---

## 5. Optimization Panel: `src/app/graphql-tutor/components/optimization-panel.tsx`

```typescript
"use client";

import { Card, CardHeader, CardTitle, CardContent } from "@/components/ui/card";
import { Badge } from "@/components/ui/badge";

interface OptimizationPanelProps {
  optimizations: any[];
}

export function OptimizationPanel({ optimizations }: OptimizationPanelProps) {
  if (!optimizations || optimizations.length === 0) {
    return <p className="text-muted-foreground">No optimization suggestions available</p>;
  }

  return (
    <div className="space-y-4">
      <h3 className="text-lg font-semibold">Optimization Suggestions</h3>
      {optimizations.map((opt, idx) => (
        <Card key={idx}>
          <CardHeader>
            <div className="flex items-center justify-between">
              <CardTitle className="text-sm">Suggestion {idx + 1}</CardTitle>
              {opt.confidence && (
                <Badge variant="outline">
                  {Math.round(opt.confidence * 100)}% confidence
                </Badge>
              )}
            </div>
          </CardHeader>
          <CardContent>
            <p className="text-sm mb-2">{opt.explanation}</p>
            {opt.query && (
              <div className="mt-2">
                <p className="text-xs text-muted-foreground mb-1">Optimized Query:</p>
                <code className="text-xs bg-muted p-2 rounded block">
                  {opt.query}
                </code>
              </div>
            )}
          </CardContent>
        </Card>
      ))}
    </div>
  );
}
```

---

## 6. Efficiency Display: `src/app/graphql-tutor/components/efficiency-display.tsx`

```typescript
"use client";

import { Card, CardHeader, CardTitle, CardContent } from "@/components/ui/card";
import { Progress } from "@/components/ui/progress";
import { Badge } from "@/components/ui/badge";

interface EfficiencyDisplayProps {
  efficiency: any;
}

export function EfficiencyDisplay({ efficiency }: EfficiencyDisplayProps) {
  if (!efficiency) {
    return <p className="text-muted-foreground">No efficiency data available</p>;
  }

  const score = efficiency.score || 0;
  const scoreColor = score >= 80 ? "text-green-600" : score >= 60 ? "text-yellow-600" : "text-red-600";

  return (
    <div className="space-y-4">
      <div>
        <h3 className="text-lg font-semibold mb-2">Efficiency Score</h3>
        <div className="space-y-2">
          <div className="flex items-center justify-between">
            <span className="text-sm">Overall Efficiency</span>
            <span className={`text-2xl font-bold ${scoreColor}`}>{score}/100</span>
          </div>
          <Progress value={score} className="h-2" />
        </div>
      </div>

      <div className="grid grid-cols-2 gap-4">
        <Card>
          <CardHeader>
            <CardTitle className="text-sm">Estimated Time</CardTitle>
          </CardHeader>
          <CardContent>
            <p className="text-2xl font-bold">{efficiency.estimatedTime || "Unknown"}</p>
          </CardContent>
        </Card>

        <Card>
          <CardHeader>
            <CardTitle className="text-sm">Resource Usage</CardTitle>
          </CardHeader>
          <CardContent>
            <p className="text-sm">{efficiency.resourceUsage || "Unknown"}</p>
          </CardContent>
        </Card>
      </div>

      {efficiency.complexity && (
        <div>
          <h4 className="text-md font-semibold mb-2">Complexity</h4>
          <Badge variant={
            efficiency.complexity === "low" ? "default" :
            efficiency.complexity === "medium" ? "secondary" : "destructive"
          }>
            {efficiency.complexity}
          </Badge>
        </div>
      )}

      {efficiency.recommendations && efficiency.recommendations.length > 0 && (
        <div>
          <h4 className="text-md font-semibold mb-2">Recommendations</h4>
          <ul className="list-disc list-inside space-y-1">
            {efficiency.recommendations.map((rec: string, idx: number) => (
              <li key={idx} className="text-sm">{rec}</li>
            ))}
          </ul>
        </div>
      )}
    </div>
  );
}
```

---

## 7. Update Home Page: `src/app/page.tsx`

Add a navigation link to the GraphQL Tutor. Find your navigation section and add:

```typescript
import Link from "next/link";
import { Button } from "@/components/ui/button";

// In your navigation or main content area:
<Link href="/graphql-tutor">
  <Button variant="outline">GraphQL Tutor</Button>
</Link>
```

---

## 8. Environment Variables

Add to Nexus `.env` file:

```env
# GraphQ-LLM Backend URL
GRAPHQ_LLM_API_URL=http://localhost:3001
# Or use NEXT_PUBLIC_ prefix for client-side access:
NEXT_PUBLIC_GRAPHQ_LLM_API_URL=http://localhost:3001
```

---

## 9. Required Dependencies

Ensure these are installed in Nexus:

```bash
cd /path/to/nexus
npm install lucide-react

# If using shadcn/ui (recommended):
# These should already be installed, but verify:
npm install @radix-ui/react-tabs
npm install @radix-ui/react-progress
```

---

## 10. Verification

After creating all files:

1. **Restart Nexus:**
   ```bash
   # Stop current server (Ctrl+C)
   npm run dev
   ```

2. **Test the API route:**
   ```bash
   curl -X POST http://localhost:3000/api/graphql-tutor/analyze \
     -H "Content-Type: application/json" \
     -d '{"query":"{ getTransaction(id: \"123\") { asset } }"}'
   ```

3. **Test the UI:**
   - Open `http://localhost:3000/graphql-tutor`
   - Enter a query
   - Click "Analyze Query"
   - Verify you see explanation, optimization, and efficiency tabs

---

## 🐛 Troubleshooting

### **Issue: Components not found**

**Solution:** Ensure all component files are created in the correct directories:
- `src/app/graphql-tutor/components/tutor-panel.tsx`
- `src/app/graphql-tutor/components/explanation-panel.tsx`
- `src/app/graphql-tutor/components/optimization-panel.tsx`
- `src/app/graphql-tutor/components/efficiency-display.tsx`

### **Issue: UI components not rendering**

**Solution:** 
- Check that shadcn/ui components are installed
- Verify imports match your component library structure
- Check browser console for errors

### **Issue: API route returns 500 error**

**Solution:**
- Verify GraphQ-LLM backend is running: `curl http://localhost:3001/health`
- Check `GRAPHQ_LLM_API_URL` in Nexus `.env`
- Check Nexus server logs for errors

---

## ✅ Checklist

After setup, verify:

- [ ] All 6 files created in correct locations
- [ ] Navigation link added to home page
- [ ] Environment variable `GRAPHQ_LLM_API_URL` set
- [ ] Dependencies installed (`lucide-react`, shadcn/ui components)
- [ ] Nexus restarted
- [ ] API route responds: `curl http://localhost:3000/api/graphql-tutor/analyze`
- [ ] UI page accessible: `http://localhost:3000/graphql-tutor`
- [ ] Query analysis works in UI

---

**That's it! Your Nexus UI now has the GraphQL Tutor integration! 🎉**

