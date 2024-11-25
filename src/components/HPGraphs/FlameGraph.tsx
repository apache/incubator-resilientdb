import React from 'react';
import { ResponsiveContainer, Treemap, Tooltip } from 'recharts';

const data = {
  name: 'Database Processes',
  children: [
    {
      name: 'Query Execution',
      children: [
        { name: 'Parse Query', size: 200, time: '20ms' },
        { name: 'Optimize Query', size: 300, time: '30ms' },
        { name: 'Execute Query', size: 500, time: '50ms' },
      ],
    },
    {
      name: 'Data Management',
      children: [
        { name: 'Buffer Pool', size: 400, time: '40ms' },
        { name: 'Cache Management', size: 300, time: '30ms' },
      ],
    },
    {
      name: 'Transaction Management',
      children: [
        { name: 'Lock Management', size: 200, time: '20ms' },
        { name: 'ACID Compliance', size: 300, time: '30ms' },
      ],
    },
    { name: 'I/O Operations', size: 400, time: '40ms' },
  ],
};

const COLORS = ['#8884d8', '#83a6ed', '#8dd1e1', '#82ca9d', '#a4de6c', '#d0ed57', '#ffc658', '#ff8a65'];

const CustomizedContent: React.FC<any> = ({ root, depth, x, y, width, height, index, colors, name }) => {
  return (
    <g>
      <rect
        x={x}
        y={y}
        width={width}
        height={height}
        style={{
          fill: colors[index % colors.length],
          stroke: '#fff',
          strokeWidth: 2 / (depth + 1e-10),
          strokeOpacity: 1 / (depth + 1e-10),
        }}
      />
      {width > 50 && height > 20 && (
        <text x={x + width / 2} y={y + height / 2} textAnchor="middle" fill="#fff" fontSize={14} dy=".3em">
          {name}
        </text>
      )}
    </g>
  );
};

const CustomTooltip: React.FC<any> = ({ active, payload }) => {
  if (active && payload && payload.length) {
    const data = payload[0].payload;
    return (
      <div className="bg-gray-800 p-2 rounded shadow-lg">
        <p className="text-white">{`Process: ${data.name}`}</p>
        <p className="text-white">{`Time: ${data.time || 'N/A'}`}</p>
      </div>
    );
  }
  return null;
};

export const FlameGraph: React.FC = () => {
  return (
    <div className="h-[300px]">
      <ResponsiveContainer width="100%" height="100%">
        <Treemap
          data={[data]}
          dataKey="size"
          stroke="#fff"
          fill="#8884d8"
          content={<CustomizedContent colors={COLORS} />}
        >
          <Tooltip content={<CustomTooltip />} />
        </Treemap>
      </ResponsiveContainer>
    </div>
  );
};

