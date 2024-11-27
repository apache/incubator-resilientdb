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

const getAxisYDomain = (
  data: any[],
  from: number,
  to: number,
  ref: string,
  offset: number
) => {
  const refData: any[] = data.slice(from, to);
  console.log("Ref Data", refData);
  let [bottom, top] = [refData[0][ref], refData[0][ref]];

  refData.forEach((d) => {
    if (d[ref] > top) top = d[ref];
    if (d[ref] < bottom) bottom = d[ref];
  });

  return [(bottom | 0) - offset, (top | 0) + offset];
};

const initialState = {
  left: "dataMin",
  right: "dataMax",
  refAreaLeft: "",
  refAreaRight: "",
  top: "dataMax+1",
  bottom: "dataMin-1",
  top2: "dataMax+20",
  bottom2: "dataMin-20",
  animation: true,
};

const chartConfig = {
  kv_service: {
    label: "KV Service",
    color: "hsl(var(--chart-1))",
  },
} satisfies ChartConfig;

export default class App extends Component<any, any> {
  constructor(props: any) {
    super(props);
    const chartsData = cpuChartData?.data?.result[0]?.values.map((val, idx) => {
      return {
        key: idx,
        timestamp: val[0],
        value: val[1],
      };
    });
    const startState = { ...initialState, data: chartsData };
    this.state = startState;
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
    let { refAreaLeft, refAreaRight } = this.state;
    const { data } = this.state;

    if (refAreaLeft === refAreaRight || refAreaRight === "") {
      this.setState(() => ({
        refAreaLeft: "",
        refAreaRight: "",
      }));
      return;
    }

    // xAxis domain
    if (refAreaLeft > refAreaRight)
      [refAreaLeft, refAreaRight] = [refAreaRight, refAreaLeft];

    // yAxis domain
    const [bottom, top] = getAxisYDomain(
      data,
      refAreaLeft,
      refAreaRight,
      "value",
      1
    );

    this.setState(() => ({
      refAreaLeft: "",
      refAreaRight: "",
      data: data.slice(),
      left: refAreaLeft,
      right: refAreaRight,
      bottom,
      top,
    }));
  }

  zoomOut() {
    const { data } = this.state;
    this.setState(() => ({
      data: data.slice(),
      refAreaLeft: "",
      refAreaRight: "",
      left: "dataMin",
      right: "dataMax",
      top: "dataMax+1",
      bottom: "dataMin",
    }));
  }

  render() {
    const { data, left, right, refAreaLeft, refAreaRight, top, bottom } =
      this.state;

    console.log(this.state);

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
              width={1500}
              height={400}
              data={data}
              margin={{
                top: 20,
                right: 20,
                left: 20,
                bottom: 20,
              }}
              onMouseDown={(e: any) =>
                this.setState({ refAreaLeft: e.activeLabel })
              }
              onMouseMove={(e: any) =>
                this.state.refAreaLeft &&
                this.setState({ refAreaRight: e.activeLabel })
              }
              // eslint-disable-next-line react/jsx-no-bind
              onMouseUp={this.zoom.bind(this)}
            >
              <CartesianGrid strokeDasharray="3 3" />
              <XAxis
                allowDataOverflow
                dataKey="key"
                tickLine={false}
                axisLine={true}
                tickMargin={8}
                minTickGap={32}
                // tickFormatter={this.formatXAxis.bind(this)}
                domain={[left, right]}
              />
              <YAxis
                allowDataOverflow
                domain={[bottom, top]}
                type="number"
                yAxisId="1"
                ticks={Array.from(
                  { length: Math.ceil(25 / 1) + 1 },
                  (_, i) => i * 1
                )}
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
                type="natural"
                dataKey="value"
                animationDuration={300}
                stroke={`hsl(var(--chart-1))`}
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
