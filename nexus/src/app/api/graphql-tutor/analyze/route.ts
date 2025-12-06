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
      // Extract complexity from efficiencyData or fallback to explanationData
      const complexity = efficiencyData?.complexity || 
                        explanationData?.complexity || 
                        (efficiencyData?.queryAnalysis ? 
                          (efficiencyData.queryAnalysis.fieldCount > 20 ? "high" : 
                           efficiencyData.queryAnalysis.fieldCount > 10 ? "medium" : "low") : 
                          "medium");

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
            complexity: complexity, // Use extracted complexity
            similarQueries: efficiencyData.similarQueries,
          }
        : {
            score: 0,
            estimatedTime: "Unknown",
            resourceUsage: "Unknown",
            recommendations: [],
            complexity: complexity, // Use extracted complexity even in fallback
          };

      // Send query statistics to ResLens Middleware
      const middlewareUrl = process.env.RESLENS_MIDDLEWARE_URL || "http://localhost:3003";
      const storeUrl = `${middlewareUrl}/api/v1/queryStats/store`;
      
      try {
        console.log(`[Nexus] Sending query stats to middleware: ${storeUrl}`);
        console.log(`[Nexus] Efficiency complexity: ${efficiency.complexity}`);
        const middlewareResponse = await fetch(storeUrl, {
          method: "POST",
          headers: { "Content-Type": "application/json" },
          body: JSON.stringify({
            query,
            efficiency: {
              ...efficiency,
              complexity: efficiency.complexity || "medium", // Ensure complexity is always set
            },
            explanation,
            optimizations,
            timestamp: new Date().toISOString(),
          }),
        });

        if (!middlewareResponse.ok) {
          const errorText = await middlewareResponse.text().catch(() => 'Unknown error');
          console.error(`[Nexus] Middleware returned ${middlewareResponse.status}: ${errorText}`);
        } else {
          const result = await middlewareResponse.json().catch(() => ({}));
          console.log(`[Nexus] Query stats stored successfully:`, result);
        }
      } catch (middlewareError) {
        // Log but don't fail if middleware is unavailable
        console.error("[Nexus] Failed to store query statistics in middleware:", middlewareError);
        if (middlewareError instanceof Error) {
          console.error("[Nexus] Error details:", middlewareError.message);
          console.error("[Nexus] Stack:", middlewareError.stack);
        }
      }

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

