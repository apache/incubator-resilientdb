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
  left: number | 'dataMin';
  right: number | 'dataMax';
  refAreaLeft: string | null | undefined;
  refAreaRight: string | null | undefined;
  top: number | 'dataMax+1';
  bottom: number | 'dataMin-1';
  animation: boolean;
}

const getAxisYDomain = (
  data: DataPoint[],
  from: number,
  to: number,
  offset: number
): [number, number] => {
  const refData = data.slice(from, to);
  let [bottom, top] = [refData[0].value, refData[0].value];

  refData.forEach((d) => {
    if (d.value > top) top = d.value;
    if (d.value < bottom) bottom = d.value;
  });

  return [(bottom | 0) - offset, (top | 0) + offset];
};

const initialState: ChartState = {
  data: [],
  left: 'dataMin',
  right: 'dataMax',
  refAreaLeft: null,
  refAreaRight: null,
  top: 'dataMax+1',
  bottom: 'dataMin-1',
  animation: true,
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
    this.state = { ...initialState, data: chartsData };
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

  if (!refAreaLeft || !refAreaRight || refAreaLeft === refAreaRight) {
    this.setState({ refAreaLeft: null, refAreaRight: null });
    return;
  }

  const leftIndex = data.findIndex((d) => d.timestamp === Number(refAreaLeft));
  const rightIndex = data.findIndex((d) => d.timestamp === Number(refAreaRight));

  if (leftIndex === -1 || rightIndex === -1) {
    this.setState({ refAreaLeft: null, refAreaRight: null });
    return;
  }

  const [minIndex, maxIndex] = [Math.min(leftIndex, rightIndex), Math.max(leftIndex, rightIndex)];
  const [bottom, top] = getAxisYDomain(data, minIndex, maxIndex, 1);

  this.setState({
    refAreaLeft: null,
    refAreaRight: null,
    left: data[minIndex].timestamp,
    right: data[maxIndex].timestamp,
    bottom,
    top,
  });
}

  zoomOut() {
    const { data } = this.state;
    this.setState({
      data: data.slice(),
      refAreaLeft: null,
      refAreaRight: null,
      left: 'dataMin',
      right: 'dataMax',
      top: 'dataMax+1',
      bottom: 'dataMin-1',
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
              onMouseDown={(e) => e && this.setState({ refAreaLeft: e.activeLabel })}
              onMouseMove={(e) => e && refAreaLeft && this.setState({ refAreaRight: e.activeLabel })}
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
