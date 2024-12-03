import { Card, CardContent, CardHeader, CardTitle } from "@/components/ui/card"
import { LineChart, Line, XAxis, YAxis, Tooltip as RechartsTooltip, ResponsiveContainer } from 'recharts'
import { useState } from "react"

const generateRandomData = (length: number) => {
  return Array.from({ length }, (_, i) => ({
    name: i.toString(),
    value: Math.floor(Math.random() * 100)
  }))
}

const MetricChart = ({ title, data, yAxisLabel }) => (
  <Card className="bg-slate-800">
    <CardHeader>
      <CardTitle className="text-lg font-semibold text-white">{title}</CardTitle>
    </CardHeader>
    <CardContent>
      <ResponsiveContainer width="100%" height={200}>
        <LineChart data={data} margin={{ top: 5, right: 30, left: 20, bottom: 5 }}>
          <XAxis 
            dataKey="name" 
            stroke="#888888"
            fontSize={12}
            tickLine={false}
            axisLine={{ stroke: '#888888' }}
          />
          <YAxis
            stroke="#888888"
            fontSize={12}
            tickLine={false}
            axisLine={{ stroke: '#888888' }}
            tickFormatter={(value) => `${value}${yAxisLabel}`}
          />
          <RechartsTooltip
            contentStyle={{ backgroundColor: '#334155', border: 'none' }}
            labelStyle={{ color: '#94a3b8' }}
            itemStyle={{ color: '#e2e8f0' }}
          />
          <Line
            type="monotone"
            dataKey="value"
            stroke="#2563eb"
            strokeWidth={2}
            dot={false}
            activeDot={{ r: 8 }}
          />
        </LineChart>
      </ResponsiveContainer>
    </CardContent>
  </Card>
)

export function MemoryMetricsGrid() {
  const metrics = [
    { title: "Time Spent Doing I/O", yAxisLabel: "%" },
    { title: "Disk R/W Data", yAxisLabel: "B" },
    { title: "Disk IOPS", yAxisLabel: "" },
    { title: "Disk Average Wait Time", yAxisLabel: "ms" }
  ]

  return (
    <div className="grid grid-cols-2 gap-6 mt-8">
      {metrics.map((metric) => (
        <MetricChart key={metric.title} title={metric.title} data={generateRandomData(20)} yAxisLabel={metric.yAxisLabel} />
      ))}
    </div>
  )
}

