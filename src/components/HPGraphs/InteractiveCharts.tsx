import React from 'react';
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from "@/components/ui/card";
import { FlameGraph } from './FlameGraph';
import { DatabaseOperationsHistogram } from './DatabaseOperationsHistogram';
import { PerformanceMetricsChart } from './PerformanceMetricsChart';
import { Carousel } from '../Carousel';

export const InteractiveCharts: React.FC = () => {
  return (
    <div className="space-y-8 p-8 pt-16 bg-[#1a1a1a]">
      <h2 className="text-3xl font-bold text-white mb-6">Interactive Profiling Visualizations</h2>
      <Carousel>
        <Card className="bg-[#2a2a2a] text-white rounded-xl">
          <CardHeader>
            <CardTitle>Database Process Flame Graph</CardTitle>
            <CardDescription>Visualize CPU time distribution across database processes and function calls</CardDescription>
          </CardHeader>
          <CardContent>
            <FlameGraph />
          </CardContent>
        </Card>
        <Card className="bg-[#2a2a2a] text-white rounded-xl">
          <CardHeader>
            <CardTitle>Process Performance Metrics</CardTitle>
            <CardDescription>Monitor CPU and memory usage for key database processes</CardDescription>
          </CardHeader>
          <CardContent>
            <PerformanceMetricsChart />
          </CardContent>
        </Card>
        <Card className="bg-[#2a2a2a] text-white rounded-xl">
          <CardHeader>
            <CardTitle>Database Operations Histogram</CardTitle>
            <CardDescription>View the distribution of read and write operations on the database</CardDescription>
          </CardHeader>
          <CardContent>
            <DatabaseOperationsHistogram />
          </CardContent>
        </Card>
      </Carousel>
    </div>
  );
};

