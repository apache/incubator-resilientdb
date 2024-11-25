import React from 'react';
import { ResponsiveContainer, BarChart, Bar, XAxis, YAxis, CartesianGrid, Tooltip, Legend } from 'recharts';

const data = [
  { operation: 'SELECT', count: 1200, color: '#8884d8' },
  { operation: 'INSERT', count: 800, color: '#82ca9d' },
  { operation: 'UPDATE', count: 600, color: '#ffc658' },
  { operation: 'DELETE', count: 300, color: '#ff8042' },
  { operation: 'JOIN', count: 450, color: '#a4de6c' },
  { operation: 'INDEX SCAN', count: 700, color: '#8dd1e1' },
];

export const DatabaseOperationsHistogram: React.FC = () => {
  return (
    <div className="h-[300px]">
      <ResponsiveContainer width="100%" height="100%">
        <BarChart data={data}>
          <CartesianGrid strokeDasharray="3 3" />
          <XAxis dataKey="operation" stroke="#fff" />
          <YAxis stroke="#fff" />
          <Tooltip
            contentStyle={{ backgroundColor: '#333', border: 'none' }}
            labelStyle={{ color: '#fff' }}
          />
          <Legend />
          <Bar dataKey="count" fill="#8884d8" />
        </BarChart>
      </ResponsiveContainer>
    </div>
  );
};

