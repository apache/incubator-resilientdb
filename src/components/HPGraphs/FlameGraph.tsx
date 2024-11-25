import React from 'react';
import { ResponsiveContainer, Treemap } from 'recharts';
import { ChartContainer } from "@/components/ui/chart";

const data = [
  {
    name: 'Database Processes',
    children: [
      { 
        name: 'Query Execution',
        children: [
          { name: 'Parse Query', size: 200, color: '#8884d8' },
          { name: 'Optimize Query', size: 300, color: '#83a6ed' },
          { name: 'Execute Query', size: 500, color: '#8dd1e1' },
        ],
      },
      { 
        name: 'Data Management',
        children: [
          { name: 'Buffer Pool', size: 400, color: '#82ca9d' },
          { name: 'Cache Management', size: 300, color: '#a4de6c' },
        ],
      },
      { 
        name: 'Transaction Management',
        children: [
          { name: 'Lock Management', size: 200, color: '#d0ed57' },
          { name: 'ACID Compliance', size: 300, color: '#ffc658' },
        ],
      },
      { name: 'I/O Operations', size: 400, color: '#ff8a65' },
    ],
  },
];

const CustomizedContent: React.FC<any> = ({ root, depth, x, y, width, height, index, colors, name }) => {
  return (
    <g>
      <rect
        x={x}
        y={y}
        width={width}
        height={height}
        style={{
          fill: depth < 2 ? colors[Math.floor((index / root.children.length) * 6)] : 'none',
          stroke: '#fff',
          strokeWidth: 2 / (depth + 1e-10),
          strokeOpacity: 1 / (depth + 1e-10),
        }}
      />
      {depth === 1 && (
        <text x={x + width / 2} y={y + height / 2 + 7} textAnchor="middle" fill="#fff" fontSize={14}>
          {name}
        </text>
      )}
    </g>
  );
};

export const FlameGraph: React.FC = () => {
  return (
    <ChartContainer config={{}} className="h-[400px]">
      <ResponsiveContainer width="100%" height="100%">
        <Treemap
          data={data}
          dataKey="size"
          stroke="#fff"
          fill="#8884d8"
          content={<CustomizedContent colors={['#8884d8', '#83a6ed', '#8dd1e1', '#82ca9d', '#a4de6c', '#d0ed57', '#ffc658', '#ff8a65']} />}
        />
      </ResponsiveContainer>
    </ChartContainer>
  );
};

