import { Card, CardContent, CardHeader, CardTitle } from "@/components/ui/card"
import { LineChart, Line, XAxis, YAxis, Tooltip as RechartsTooltip, ResponsiveContainer } from 'recharts'
import { useState, useCallback } from "react"
import { Button } from "@/components/ui/button"
import { RefreshCw, Info } from 'lucide-react'
import {
  Tooltip,
  TooltipContent,
  TooltipProvider,
  TooltipTrigger,
} from "@/components/ui/tooltip"

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
  const [metricsData, setMetricsData] = useState({
    ioTime: generateRandomData(20),
    diskRW: generateRandomData(20),
    diskIOPS: generateRandomData(20),
    diskWaitTime: generateRandomData(20)
  })

  const refreshData = useCallback(() => {
    setMetricsData({
      ioTime: generateRandomData(20),
      diskRW: generateRandomData(20),
      diskIOPS: generateRandomData(20),
      diskWaitTime: generateRandomData(20)
    })
  }, [])

  const metrics = [
    { title: "Time Spent Doing I/O", yAxisLabel: "%", data: metricsData.ioTime },
    { title: "Disk R/W Data", yAxisLabel: "B", data: metricsData.diskRW },
    { title: "Disk IOPS", yAxisLabel: "", data: metricsData.diskIOPS },
    { title: "Disk Average Wait Time", yAxisLabel: "ms", data: metricsData.diskWaitTime }
  ]

  return (
    <Card className="bg-slate-900 p-6">
      <CardHeader className="flex flex-row items-center justify-between pb-2">
        <CardTitle className="text-xl font-bold text-white">Memory Metrics</CardTitle>
        <div className="flex items-center space-x-2">
          <Button onClick={refreshData} variant="outline" size="icon">
            <RefreshCw className="h-4 w-4" />
          </Button>
          <TooltipProvider>
            <Tooltip>
              <TooltipTrigger asChild>
                <button
                  className="p-2 bg-slate-700 text-slate-400 hover:text-white hover:bg-slate-600 transition-colors duration-200 ease-in-out rounded"
                  onClick={() => window.open("https://gmail.com", "_blank")}
                >
                  <Info size={24} />
                </button>
              </TooltipTrigger>
              <TooltipContent>
                <p>Click for more information about these metrics</p>
              </TooltipContent>
            </Tooltip>
          </TooltipProvider>
        </div>
      </CardHeader>
      <CardContent>
        <div className="grid grid-cols-2 gap-6">
          {metrics.map((metric) => (
            <MetricChart key={metric.title} title={metric.title} data={metric.data} yAxisLabel={metric.yAxisLabel} />
          ))}
        </div>
      </CardContent>
    </Card>
  )
}

