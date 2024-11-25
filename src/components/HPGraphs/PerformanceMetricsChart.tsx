import React from 'react';
import { ResponsiveContainer, LineChart, Line, XAxis, YAxis, CartesianGrid, Tooltip, Legend } from 'recharts';

const data = [
  { process: 'Query Executor', cpu: 45, memory: 30 },
  { process: 'Buffer Manager', cpu: 30, memory: 40 },
  { process: 'Transaction Manager', cpu: 25, memory: 20 },
  { process: 'Lock Manager', cpu: 15, memory: 10 },
  { process: 'I/O Subsystem', cpu: 35, memory: 25 },
];

export const PerformanceMetricsChart: React.FC = () => {
  return (
    <div className="h-[300px]">
      <ResponsiveContainer width="100%" height="100%">
        <LineChart data={data}>
          <CartesianGrid strokeDasharray="3 3" />
          <XAxis dataKey="process" stroke="#fff" />
          <YAxis stroke="#fff" />
          <Tooltip
            contentStyle={{ backgroundColor: '#333', border: 'none' }}
            labelStyle={{ color: '#fff' }}
          />
          <Legend />
          <Line type="monotone" dataKey="cpu" stroke="#8884d8" activeDot={{ r: 8 }} />
          <Line type="monotone" dataKey="memory" stroke="#82ca9d" activeDot={{ r: 8 }} />
        </LineChart>
      </ResponsiveContainer>
    </div>
  );
};

