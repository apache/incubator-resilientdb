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

