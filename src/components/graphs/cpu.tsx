import { useState, useCallback, useMemo } from "react";
import { Cpu, Search } from "lucide-react";
import {
  Card,
  CardContent,
  CardDescription,
  CardHeader,
  CardTitle,
} from "../ui/card";
import * as React from "react";
import {
  CartesianGrid,
  Line,
  LineChart,
  Bar,
  BarChart,
  XAxis,
  YAxis,
  ReferenceArea,
  ResponsiveContainer,
} from "recharts";
import {
  Select,
  SelectContent,
  SelectItem,
  SelectTrigger,
  SelectValue,
} from "../ui/select";
import { Button } from "../ui/button";
import { cpuChartData } from "@/static/processCpuChartData";
import {
  ChartConfig,
  ChartContainer,
  ChartTooltip,
  ChartTooltipContent,
} from "../ui/chart";
import App from "./lineGraph";

const chartConfig = {
  kv_service: {
    label: "KV Service",
    color: "hsl(var(--chart-1))",
  },
} satisfies ChartConfig;

const yAxisSteps = [5, 10, 20];

export function LineGraph({ cpuChartData }) {
  const [activeChart] = useState<keyof typeof chartConfig>("kv_service");
  const [yAxisStep, setYAxisStep] = useState(5);

  const chartsData = useMemo(() => {
    return cpuChartData?.data?.result[0].values.map((val) => ({
      timestamp: val[0] * 1000,
      kv_service: val[1],
    }));
  }, [cpuChartData]);

  const [state, setState] = useState({
    data: chartsData,
    left: "dataMin",
    right: "dataMax",
    refAreaLeft: "",
    refAreaRight: "",
    top: "dataMax+1",
    bottom: "dataMin-1",
    animation: true,
  });

  const getAxisYDomain = useCallback(
    (from: number, to: number, ref: string, offset: number) => {
      const refData = chartsData.filter(
        (item) => item.timestamp >= from && item.timestamp <= to
      );
      console.log(refData);
      console.log(ref);
      let [bottom, top] = [refData[0][ref], refData[0][ref]];

      refData.forEach((d) => {
        if (d[ref] > top) top = d[ref];
        if (d[ref] < bottom) bottom = d[ref];
      });

      const returnVal = [(bottom | 0) - offset, (top | 0) + offset];
      console.log(returnVal);
      return returnVal;
    },
    [chartsData]
  );

  const zoom = useCallback(() => {
    let { refAreaLeft, refAreaRight } = state;

    if (refAreaLeft === refAreaRight || refAreaRight === "") {
      setState((prevState) => ({
        ...prevState,
        refAreaLeft: "",
        refAreaRight: "",
      }));
      return;
    }

    if (refAreaLeft > refAreaRight)
      [refAreaLeft, refAreaRight] = [refAreaRight, refAreaLeft];

    const [bottom, top] = getAxisYDomain(
      refAreaLeft,
      refAreaRight,
      "kv_service",
      5
    );

    setState((prevState) => ({
      ...prevState,
      refAreaLeft: "",
      refAreaRight: "",
      left: refAreaLeft,
      right: refAreaRight,
      bottom,
      top,
    }));
  }, [state, getAxisYDomain]);

  const zoomOut = useCallback(() => {
    setState((prevState) => ({
      ...prevState,
      refAreaLeft: "",
      refAreaRight: "",
      left: "dataMin",
      right: "dataMax",
      top: "dataMax+1",
      bottom: "dataMin",
    }));
  }, []);

  const { left, right, refAreaLeft, refAreaRight, top, bottom } = state;

  const formatXAxis = (tickItem: number) => {
    const date = new Date(tickItem);
    return date.toLocaleDateString("en-US", {
      month: "short",
      day: "numeric",
      hour: "numeric",
      minute: "numeric",
    });
  };

  return (
    <Card>
      <CardHeader className="flex flex-col items-stretch space-y-0 border-b p-0 sm:flex-row">
        <div className="flex flex-1 flex-col justify-center gap-1 px-6 py-5 sm:py-6">
          <CardTitle>CPU Usage</CardTitle>
          <CardDescription>
            Percentage CPU used by ResDB ecosystem
          </CardDescription>
        </div>
        <div className="flex items-center px-6 py-5 sm:py-6">
          <Select
            value={yAxisStep.toString()}
            onValueChange={(value) => setYAxisStep(Number(value))}
          >
            <SelectTrigger className="w-[180px]">
              <SelectValue placeholder="Select Y-axis step" />
            </SelectTrigger>
            <SelectContent>
              {yAxisSteps.map((step) => (
                <SelectItem key={step} value={step.toString()}>
                  {step} units
                </SelectItem>
              ))}
            </SelectContent>
          </Select>
        </div>
      </CardHeader>
      <CardContent className="px-2 sm:p-6">
        <Button
          onClick={zoomOut}
          className="mb-4 bg-blue-600 hover:bg-blue-700 text-white"
        >
          Zoom Out
        </Button>
        <ChartContainer
          config={chartConfig}
          className="aspect-auto h-[250px] w-full"
        >
          <LineChart
            data={chartsData}
            onMouseDown={(e) =>
              e &&
              setState((prevState) => ({
                ...prevState,
                refAreaLeft: e.activeLabel,
              }))
            }
            onMouseMove={(e) =>
              e &&
              state.refAreaLeft &&
              setState((prevState) => ({
                ...prevState,
                refAreaRight: e.activeLabel,
              }))
            }
            onMouseUp={zoom}
            margin={{
              top: 20,
              right: 20,
              left: 20,
              bottom: 20,
            }}
          >
            <CartesianGrid vertical={false} />
            <XAxis
              dataKey="timestamp"
              tickLine={false}
              axisLine={true}
              tickMargin={8}
              minTickGap={32}
              tickFormatter={formatXAxis}
              domain={[left, right]}
            />
            <YAxis
              dataKey="kv_service"
              tickLine={true}
              axisLine={true}
              tickMargin={8}
              domain={[bottom, top]}
              ticks={Array.from(
                { length: Math.ceil(20 / yAxisStep) + 1 },
                (_, i) => i * yAxisStep
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
              dataKey={activeChart}
              type="monotone"
              stroke={`var(--color-${activeChart})`}
              strokeWidth={2}
              dot={false}
            />
            {refAreaLeft && refAreaRight ? (
              <ReferenceArea
                x1={refAreaLeft}
                x2={refAreaRight}
                strokeOpacity={0.3}
                fill="hsl(var(--primary))"
                fillOpacity={0.3}
              />
            ) : null}
          </LineChart>
        </ChartContainer>
      </CardContent>
    </Card>
  );
}

export function CpuGraph() {
  return (
    <Card className="w-full max-w-8xl mx-auto bg-gradient-to-br from-slate-900 to-slate-950 text-white shadow-xl">
      <CardHeader>
        <div className="flex items-center gap-2">
          <Cpu className="w-6 h-6 text-blue-400" />
          <CardTitle className="text-2xl font-bold">CPU Usage</CardTitle>
        </div>
      </CardHeader>
      <CardContent>
        <div className="grid grid-cols-1 md:grid-cols-4 gap-4 mb-6">
          <Select>
            <SelectTrigger className="bg-slate-800 border-slate-700">
              <SelectValue placeholder="Profile type" />
            </SelectTrigger>
            <SelectContent>
              <SelectItem value="cpu">CPU</SelectItem>
              <SelectItem value="memory">Memory</SelectItem>
            </SelectContent>
          </Select>
          <Select>
            <SelectTrigger className="bg-slate-800 border-slate-700">
              <SelectValue placeholder="Select label name" />
            </SelectTrigger>
            <SelectContent>
              <SelectItem value="name">Name</SelectItem>
              <SelectItem value="id">ID</SelectItem>
            </SelectContent>
          </Select>
          <Select>
            <SelectTrigger className="bg-slate-800 border-slate-700">
              <SelectValue placeholder="Select label value" />
            </SelectTrigger>
            <SelectContent>
              <SelectItem value="value1">Value 1</SelectItem>
              <SelectItem value="value2">Value 2</SelectItem>
            </SelectContent>
          </Select>
          <Button className="bg-blue-600 hover:bg-blue-700 text-white">
            <Search className="h-4 w-4 mr-2" />
            Search
          </Button>
        </div>
        <App />
        {/* <LineGraph cpuChartData={cpuChartData} /> */}
        {/* <BarChartGraph /> */}
      </CardContent>
    </Card>
  );
}
