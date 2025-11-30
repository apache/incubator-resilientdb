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

import { useState, useEffect } from "react";
import { Card, CardHeader, CardTitle, CardContent } from "@/components/ui/card";
import { Badge } from "@/components/ui/badge";
import { Progress } from "@/components/ui/progress";
import { useMode } from "@/contexts/ModeContext";
import { middlewareApi } from "@/lib/api";
import { Loader2, RefreshCw, TrendingUp, Clock, Zap, AlertCircle } from "lucide-react";
import { Button } from "@/components/ui/button";

interface QueryStat {
  id: string;
  query: string;
  efficiency: {
    score: number;
    estimatedTime?: string;
    resourceUsage?: string;
    complexity?: string;
    recommendations?: string[];
  };
  explanation?: {
    explanation?: string;
    complexity?: string;
  };
  optimizations?: Array<{
    query?: string;
    explanation?: string;
    confidence?: number;
  }>;
  timestamp: string;
  createdAt: string;
}

interface AggregatedStats {
  totalQueries: number;
  avgEfficiency: number;
  complexityDistribution: Record<string, number>;
  recentQueries: QueryStat[];
}

export function QueryStats() {
  const { api, refreshTrigger } = useMode();
  const [queryStats, setQueryStats] = useState<QueryStat[]>([]);
  const [aggregatedStats, setAggregatedStats] = useState<AggregatedStats | null>(null);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);

  const fetchQueryStats = async () => {
    try {
      setLoading(true);
      setError(null);

      // Get base URL from environment
      const baseUrl = import.meta.env.VITE_MIDDLEWARE_BASE_URL || "http://localhost:3003/api/v1";
      console.log("Fetching query stats from:", baseUrl);

      // Use fetch directly to have better error handling
      const statsUrl = `${baseUrl}/queryStats?limit=20`;
      console.log("Full URL:", statsUrl);
      
      const statsResponse = await fetch(statsUrl, {
        method: "GET",
        headers: {
          "Content-Type": "application/json",
        },
      });

      if (!statsResponse.ok) {
        throw new Error(`HTTP error! status: ${statsResponse.status}`);
      }

      const statsData = await statsResponse.json();
      console.log("Query stats response:", statsData);
      
      if (statsData?.success) {
        setQueryStats(statsData.data || []);
      }

      // Fetch aggregated statistics
      const aggregatedUrl = `${baseUrl}/queryStats/aggregated`;
      const aggregatedResponse = await fetch(aggregatedUrl, {
        method: "GET",
        headers: {
          "Content-Type": "application/json",
        },
      });

      if (!aggregatedResponse.ok) {
        throw new Error(`HTTP error! status: ${aggregatedResponse.status}`);
      }

      const aggregatedData = await aggregatedResponse.json();
      console.log("Aggregated stats response:", aggregatedData);
      
      if (aggregatedData?.success) {
        setAggregatedStats(aggregatedData.data);
      }
    } catch (err: any) {
      console.error("Error fetching query statistics:", err);
      console.error("Error details:", {
        message: err.message,
        name: err.name,
        stack: err.stack,
      });
      
      // More user-friendly error message
      let errorMessage = "Failed to fetch query statistics.";
      if (err.message?.includes("Failed to fetch") || err.message?.includes("NetworkError")) {
        errorMessage = "Network error: Cannot connect to middleware. Please ensure ResLens Middleware is running on port 3003.";
      } else if (err.message?.includes("HTTP error")) {
        errorMessage = `Server error: ${err.message}`;
      } else {
        errorMessage = err.message || errorMessage;
      }
      
      setError(errorMessage);
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    fetchQueryStats();
  }, [api, refreshTrigger]);

  if (loading) {
    return (
      <div className="flex items-center justify-center h-64">
        <Loader2 className="h-8 w-8 animate-spin text-blue-500" />
      </div>
    );
  }

  if (error) {
    return (
      <Card>
        <CardContent className="pt-6">
          <div className="flex items-center gap-2 text-red-500">
            <AlertCircle className="h-5 w-5" />
            <p>Error: {error}</p>
          </div>
          <Button onClick={fetchQueryStats} className="mt-4" variant="outline">
            <RefreshCw className="h-4 w-4 mr-2" />
            Retry
          </Button>
        </CardContent>
      </Card>
    );
  }

  return (
    <div className="space-y-6">
      {/* Aggregated Statistics */}
      {aggregatedStats && (
        <div className="grid grid-cols-1 md:grid-cols-3 gap-4">
          <Card>
            <CardHeader>
              <CardTitle className="text-sm font-medium flex items-center gap-2">
                <TrendingUp className="h-4 w-4" />
                Total Queries
              </CardTitle>
            </CardHeader>
            <CardContent>
              <p className="text-3xl font-bold">{aggregatedStats.totalQueries}</p>
              <p className="text-sm text-muted-foreground mt-1">
                Queries analyzed
              </p>
            </CardContent>
          </Card>

          <Card>
            <CardHeader>
              <CardTitle className="text-sm font-medium flex items-center gap-2">
                <Zap className="h-4 w-4" />
                Average Efficiency
              </CardTitle>
            </CardHeader>
            <CardContent>
              <p className="text-3xl font-bold">{aggregatedStats.avgEfficiency.toFixed(1)}</p>
              <p className="text-sm text-muted-foreground mt-1">Out of 100</p>
              <Progress value={aggregatedStats.avgEfficiency} className="mt-2" />
            </CardContent>
          </Card>

          <Card>
            <CardHeader>
              <CardTitle className="text-sm font-medium flex items-center gap-2">
                <Clock className="h-4 w-4" />
                Complexity Distribution
              </CardTitle>
            </CardHeader>
            <CardContent>
              <div className="space-y-2">
                {Object.entries(aggregatedStats.complexityDistribution).map(([complexity, count]) => (
                  <div key={complexity} className="flex items-center justify-between">
                    <Badge variant={
                      complexity === "low" ? "default" :
                      complexity === "medium" ? "secondary" : "destructive"
                    }>
                      {complexity}
                    </Badge>
                    <span className="text-sm">{count}</span>
                  </div>
                ))}
              </div>
            </CardContent>
          </Card>
        </div>
      )}

      {/* Query Statistics List */}
      <Card>
        <CardHeader>
          <div className="flex items-center justify-between">
            <CardTitle>Query Statistics</CardTitle>
            <Button onClick={fetchQueryStats} variant="outline" size="sm">
              <RefreshCw className="h-4 w-4 mr-2" />
              Refresh
            </Button>
          </div>
        </CardHeader>
        <CardContent>
          {queryStats.length === 0 ? (
            <div className="text-center py-8 text-muted-foreground">
              <p>No query statistics available yet.</p>
              <p className="text-sm mt-2">
                Analyze queries in Nexus GraphQL Tutor to see statistics here.
              </p>
            </div>
          ) : (
            <div className="space-y-4">
              {queryStats.map((stat) => (
                <Card key={stat.id} className="border-l-4 border-l-blue-500">
                  <CardContent className="pt-4">
                    <div className="space-y-4">
                      {/* Query */}
                      <div>
                        <p className="text-sm font-medium mb-1">Query:</p>
                        <code className="text-xs bg-muted p-2 rounded block overflow-x-auto">
                          {stat.query}
                        </code>
                      </div>

                      {/* Efficiency Metrics */}
                      <div className="grid grid-cols-2 md:grid-cols-4 gap-4">
                        <div>
                          <p className="text-xs text-muted-foreground">Efficiency Score</p>
                          <div className="flex items-center gap-2 mt-1">
                            <span className={`text-lg font-bold ${
                              stat.efficiency.score >= 80 ? "text-green-500" :
                              stat.efficiency.score >= 60 ? "text-yellow-500" : "text-red-500"
                            }`}>
                              {stat.efficiency.score}
                            </span>
                            <span className="text-xs text-muted-foreground">/100</span>
                          </div>
                          <Progress value={stat.efficiency.score} className="mt-1 h-1" />
                        </div>

                        <div>
                          <p className="text-xs text-muted-foreground">Estimated Time</p>
                          <p className="text-sm font-medium mt-1">
                            {stat.efficiency.estimatedTime || "N/A"}
                          </p>
                        </div>

                        <div>
                          <p className="text-xs text-muted-foreground">Complexity</p>
                          <Badge variant={
                            stat.efficiency.complexity === "low" ? "default" :
                            stat.efficiency.complexity === "medium" ? "secondary" : "destructive"
                          } className="mt-1">
                            {stat.efficiency.complexity || "unknown"}
                          </Badge>
                        </div>

                        <div>
                          <p className="text-xs text-muted-foreground">Resource Usage</p>
                          <p className="text-xs mt-1">
                            {stat.efficiency.resourceUsage || "N/A"}
                          </p>
                        </div>
                      </div>

                      {/* Explanation */}
                      {stat.explanation?.explanation && (
                        <div>
                          <p className="text-sm font-medium mb-1">Explanation:</p>
                          <p className="text-sm text-muted-foreground">
                            {stat.explanation.explanation.substring(0, 200)}
                            {stat.explanation.explanation.length > 200 ? "..." : ""}
                          </p>
                        </div>
                      )}

                      {/* Optimizations */}
                      {stat.optimizations && stat.optimizations.length > 0 && (
                        <div>
                          <p className="text-sm font-medium mb-1">Optimizations:</p>
                          <ul className="list-disc list-inside text-sm text-muted-foreground space-y-1">
                            {stat.optimizations.slice(0, 3).map((opt, idx) => (
                              <li key={idx}>
                                {opt.explanation || "Optimization suggestion"}
                              </li>
                            ))}
                          </ul>
                        </div>
                      )}

                      {/* Timestamp */}
                      <div className="text-xs text-muted-foreground">
                        Analyzed: {new Date(stat.timestamp || stat.createdAt).toLocaleString()}
                      </div>
                    </div>
                  </CardContent>
                </Card>
              ))}
            </div>
          )}
        </CardContent>
      </Card>
    </div>
  );
}

