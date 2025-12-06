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

