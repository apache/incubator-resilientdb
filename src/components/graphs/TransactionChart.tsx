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

import { useState, useEffect, useRef } from "react"
import { LineChart, Line, XAxis, YAxis, Tooltip, ResponsiveContainer, ReferenceArea, CartesianGrid } from "recharts";

import { Button } from "../ui/button"
import { ChartContainer, ChartTooltipContent } from "../ui/LineGraphChart"
import { middlewareApi } from "@/lib/api"
import { RefreshCcw } from "lucide-react"
import { Card, CardHeader, CardTitle, CardContent } from "../ui/card"

const getAxisYDomain = (data, left, right, offset) => {
  const range = data.filter((d) => d.epoch >= left && d.epoch <= right)
  if (range.length === 0) return [0, 100]

  let bottom = range[0].volume
  let top = range[0].volume

  range.forEach((d) => {
    if (d.volume > top) top = d.volume
    if (d.volume < bottom) bottom = d.volume
  })

  return [Math.floor(bottom) - offset, Math.ceil(top) + offset]
}

export default function TransactionZoomChart() {
  const chartRef = useRef(null)
  const [chartData, setChartData] = useState([])
  const [refAreaLeft, setRefAreaLeft] = useState(null)
  const [refAreaRight, setRefAreaRight] = useState(null)
  const [left, setLeft] = useState(0)
  const [right, setRight] = useState(0)
  const [top, setTop] = useState(0)
  const [bottom, setBottom] = useState(0)
  const [fullDomain, setFullDomain] = useState({ left: 0, right: 0, top: 0, bottom: 0 })

  useEffect(() => {
    const fetchTransactionHistory = async () => {
      const { default: downsample } = await import("downsample-lttb")
      try {
        const response = await middlewareApi.get("/explorer/getAllEncodedBlocks")
        const decoded = decodeDeltaEncoding(response?.data)

        const points = decoded.map((d) => [d.epoch / 1000, d.volume])
        const sampledPoints = downsample.processData(points, 2000)

        const finalData = sampledPoints.map(([x, y]) => ({
          epoch: x * 1000,
          volume: y,
          createdAt: new Date(x * 1000).toISOString(),
        }))

        const [b, t] = getAxisYDomain(finalData, finalData[0].epoch, finalData[finalData.length - 1].epoch, 1)

        setChartData(finalData)
        setLeft(finalData[0].epoch)
        setRight(finalData[finalData.length - 1].epoch)
        setTop(t)
        setBottom(b)
        setFullDomain({ left: finalData[0].epoch, right: finalData[finalData.length - 1].epoch, top: t, bottom: b })
      } catch (error) {
        console.error("Failed to fetch transaction history:", error)
      }
    }

    fetchTransactionHistory()
  }, [])

  const zoom = () => {
    if (refAreaLeft === null || refAreaRight === null || refAreaLeft === refAreaRight) {
      setRefAreaLeft(null)
      setRefAreaRight(null)
      return
    }

    const [newLeft, newRight] = [Math.min(refAreaLeft, refAreaRight), Math.max(refAreaLeft, refAreaRight)]
    const [newBottom, newTop] = getAxisYDomain(chartData, newLeft, newRight, 1)

    setLeft(newLeft)
    setRight(newRight)
    setTop(newTop)
    setBottom(newBottom)
    setRefAreaLeft(null)
    setRefAreaRight(null)
  }

  const zoomOut = () => {
    setLeft(fullDomain.left)
    setRight(fullDomain.right)
    setTop(fullDomain.top)
    setBottom(fullDomain.bottom)
    setRefAreaLeft(null)
    setRefAreaRight(null)
  }

  const formatXAxis = (epoch) => {
    if (epoch > 1e15) epoch = epoch / 1000
    const date = new Date(epoch)
    if (isNaN(date.getTime())) return "Invalid"
    return date.toLocaleString("en-US", { month: "short", year: "numeric" })
  }

  return (
    <Card className="w-full bg-gradient-to-br from-slate-900 to-slate-950 text-white shadow-xl">
      <CardHeader className="pb-2">
        <div className="flex justify-between">
          <div className="flex items-center gap-2">
            <CardTitle className="text-2xl font-bold">Transaction Timeline</CardTitle>
          </div>
          <Button variant="outline" size="icon" onClick={zoomOut} title="Reset zoom">
            <RefreshCcw className="h-4 w-4" />
          </Button>
        </div>
      </CardHeader>
      <CardContent className="pt-2">
        <div className="w-full h-full" style={{ userSelect: "none" }}>
          <ChartContainer config={{ desktop: { label: "Volume", color: "hsl(var(--chart-1))" } }}>
            <ResponsiveContainer width="100%" height="100%">
              <LineChart
                data={chartData}
                margin={{ top: 5, right: 5, left: 5, bottom: 20 }}
                onMouseDown={(e) => {
                  if (e?.activePayload && e.activePayload[0]) {
                    setRefAreaLeft(e.activePayload[0].payload.epoch)
                  }
                }}
                onMouseMove={(e) => {
                  if (refAreaLeft !== null && e?.activePayload && e.activePayload[0]) {
                    setRefAreaRight(e.activePayload[0].payload.epoch)
                  }
                }}
                onMouseUp={zoom}
              >
                {/* <CartesianGrid strokeDasharray="3 3" /> */}
                <XAxis
                  dataKey="epoch"
                  type="number"
                  domain={[left, right]}
                  allowDataOverflow
                  tickFormatter={formatXAxis}
                  height={30}
                />
                <YAxis type="number" domain={[bottom, top]} allowDataOverflow width={40} />
                <Tooltip
                  content={<ChartTooltipContent indicator="dashed" />}
                  formatter={(value) => ["Volume: ", value]}
                  labelFormatter={(value) =>
                    new Date(value).toLocaleDateString("en-US", {
                      day: "numeric",
                      month: "short",
                      year: "numeric",
                    })
                  }
                />
                <Line
                  dataKey="volume"
                  type="monotone"
                  stroke="hsl(var(--chart-1))"
                  strokeWidth={2}
                  dot={false}
                  animationDuration={750}
                />
                {refAreaLeft && refAreaRight && (
                  <ReferenceArea x1={refAreaLeft} x2={refAreaRight} strokeOpacity={0.3} />
                )}
              </LineChart>
            </ResponsiveContainer>
          </ChartContainer>
        </div>
      </CardContent>
    </Card>
  )
}

function decodeDeltaEncoding(encodedData) {
  const { startEpoch, startVolume, timeMultiplier = 1000000, epochs, volumes } = encodedData

  const result = [
    {
      epoch: startEpoch * timeMultiplier,
      volume: startVolume,
      createdAt: new Date(startEpoch * timeMultiplier).toISOString(),
    },
  ]

  for (let i = 0; i < epochs.length; i++) {
    const prevPoint = result[result.length - 1]
    const currentEpoch = prevPoint.epoch / timeMultiplier + epochs[i]

    result.push({
      epoch: currentEpoch * timeMultiplier,
      volume: prevPoint.volume + volumes[i],
      createdAt: new Date(currentEpoch * timeMultiplier).toISOString(),
    })
  }

  return result
}
