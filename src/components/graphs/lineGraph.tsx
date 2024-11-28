import React, { Component } from "react";
import {
  LineChart,
  Line,
  XAxis,
  YAxis,
  CartesianGrid,
  Tooltip,
  ReferenceArea,
  ResponsiveContainer,
} from "recharts";
import { Button } from "../ui/button";
import { cpuChartData } from "@/static/processCpuChartData";
import {
  ChartConfig,
  ChartContainer,
  ChartTooltip,
  ChartTooltipContent,
} from "../ui/chart";

interface DataPoint {
  index: number;
  timestamp: number;
  value: number;
}

interface ChartState {
  data: DataPoint[];
  left: number;
  right: number;
  refAreaLeft: number | null;
  refAreaRight: number | null;
  top: number;
  bottom: number;
  animation: boolean;
}

const getAxisYDomain = (
  data: DataPoint[],
  left: number,
  right: number,
  offset: number
): [number, number] => {
  const filteredData = data.filter(
    (d) => d.timestamp >= left && d.timestamp <= right
  );
  if (filteredData.length === 0) return [0, 100];

  let [bottom, top] = [filteredData[0].value, filteredData[0].value];

  filteredData.forEach((d) => {
    if (d.value > top) top = d.value;
    if (d.value < bottom) bottom = d.value;
  });

  return [Math.max(0, Math.floor(bottom) - offset), Math.min(100, Math.ceil(top) + offset)];
};

const chartConfig: ChartConfig = {
  kv_service: {
    label: "KV Service",
    color: "hsl(var(--chart-1))",
  },
};

export default class App extends Component<{}, ChartState> {
  constructor(props: {}) {
    super(props);
    const chartsData = cpuChartData?.data?.result[0]?.values.map((val, idx) => ({
      index: idx,
      timestamp: Number(val[0]) * 1000, // Convert to milliseconds
      value: parseFloat(String(val[1])),
    }));

    const [bottom, top] = getAxisYDomain(
      chartsData,
      chartsData[0].timestamp,
      chartsData[chartsData.length - 1].timestamp,
      1
    );

    this.state = {
      data: chartsData,
      left: chartsData[0].timestamp,
      right: chartsData[chartsData.length - 1].timestamp,
      refAreaLeft: null,
      refAreaRight: null,
      top,
      bottom,
      animation: true,
    };
  }

  formatXAxis = (tickItem: number) => {
    const date = new Date(tickItem);
    return date.toLocaleDateString("en-US", {
      month: "short",
      day: "numeric",
      hour: "numeric",
      minute: "numeric",
    });
  };

  zoom() {
    const { refAreaLeft, refAreaRight, data } = this.state;

    if (refAreaLeft === null || refAreaRight === null || refAreaLeft === refAreaRight) {
      this.setState({ refAreaLeft: null, refAreaRight: null });
      return;
    }

    const [left, right] = [Math.min(refAreaLeft, refAreaRight), Math.max(refAreaLeft, refAreaRight)];
    const [bottom, top] = getAxisYDomain(data, left, right, 1);

    this.setState({
      refAreaLeft: null,
      refAreaRight: null,
      left,
      right,
      bottom,
      top,
    });
  }

  zoomOut() {
    const { data } = this.state;
    const [bottom, top] = getAxisYDomain(
      data,
      data[0].timestamp,
      data[data.length - 1].timestamp,
      1
    );
    this.setState({
      refAreaLeft: null,
      refAreaRight: null,
      left: data[0].timestamp,
      right: data[data.length - 1].timestamp,
      top,
      bottom,
    });
  }

  render() {
    const { data, left, right, refAreaLeft, refAreaRight, top, bottom } = this.state;

    return (
      <ChartContainer config={chartConfig} className="aspect-auto w-full">
        <div style={{ userSelect: "none" }}>
          <Button
            onClick={this.zoomOut.bind(this)}
            className="mb-4 bg-blue-600 hover:bg-blue-700 text-white"
          >
            Zoom Out
          </Button>

          <ResponsiveContainer width="100%" height={400}>
            <LineChart
              data={data}
              margin={{ top: 20, right: 20, left: 20, bottom: 20 }}
              onMouseDown={(e) => e && this.setState({ refAreaLeft: e.activeLabel ? Number(e.activeLabel) : null })}
              onMouseMove={(e) => e && refAreaLeft !== null && this.setState({ refAreaRight: e.activeLabel ? Number(e.activeLabel) : null })}
              onMouseUp={this.zoom.bind(this)}
            >
              <CartesianGrid strokeDasharray="3 3" />
              <XAxis
                allowDataOverflow
                dataKey="timestamp"
                domain={[left, right]}
                type="number"
                tickFormatter={this.formatXAxis}
              />
              <YAxis
                allowDataOverflow
                domain={[bottom, top]}
                type="number"
                yAxisId="1"
                tickCount={5}
              />
              <ChartTooltip
                content={
                  <ChartTooltipContent
                    className="w-[200px]"
                    nameKey="kv_service"
                  />
                }
              />
              <Line
                yAxisId="1"
                type="monotone"
                dataKey="value"
                stroke={`hsl(var(--chart-1))`}
                animationDuration={300}
                dot={false}
              />
              {refAreaLeft && refAreaRight ? (
                <ReferenceArea
                  yAxisId="1"
                  x1={refAreaLeft}
                  x2={refAreaRight}
                  strokeOpacity={0.3}
                />
              ) : null}
            </LineChart>
          </ResponsiveContainer>
        </div>
      </ChartContainer>
    );
  }
}

