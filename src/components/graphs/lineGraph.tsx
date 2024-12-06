import { Component, useEffect, useState } from "react";
import {
  LineChart,
  Line,
  XAxis,
  YAxis,
  CartesianGrid,
  ReferenceArea,
  ResponsiveContainer,
} from "recharts";
import { Button } from "../ui/button";
import {
  ChartConfig,
  ChartContainer,
  ChartTooltip,
  ChartTooltipContent,
} from "../ui/LineGraphChart";
import { middlewareApi } from "@/lib/api";
import { Loader } from "../ui/loader";
import { Card, CardContent, CardHeader, CardTitle } from "../ui/card";
import { Cpu, RefreshCcw } from "lucide-react";
import { NotFound } from "../ui/not-found";
import { useToast } from "@/hooks/use-toast";

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
  loading: boolean;
  refresh: boolean;
  error: string;
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

  return [
    Math.max(0, Math.floor(bottom) - offset),
    Math.min(100, Math.ceil(top) + offset),
  ];
};

const chartConfig: ChartConfig = {
  kv_service: {
    label: "KV Service",
    color: "hsl(var(--chart-1))",
  },
};

interface CpuLineGraphProps {
  setDate: React.Dispatch<
    React.SetStateAction<{
      from: number;
      until: number;
    }>
  >;
}

export const CpuLineGraphFunc: React.FC<CpuLineGraphProps> = ({ setDate }) => {
  const { toast } = useToast();
  const [data, setData] = useState<DataPoint[]>([]);
  const [loading, setLoading] = useState<boolean>(true);
  const [refAreaLeft, setRefAreaLeft] = useState<number | null>(null);
  const [refAreaRight, setRefAreaRight] = useState<number | null>(null);
  const [left, setLeft] = useState<number>(0);
  const [right, setRight] = useState<number>(0);
  const [top, setTop] = useState<number>(0);
  const [bottom, setBottom] = useState<number>(0);
  const [refresh, setRefresh] = useState<boolean>(false);
  const [error, setError] = useState<string>("");

  const fetchData = async () => {
    try {
      const until = new Date().getTime();
      const from = new Date(until - 2 * 60 * 60 * 1000).getTime();

      const response = await middlewareApi.post("/nodeExporter/getCpuUsage", {
        query:
          "sum(rate(namedprocess_namegroup_cpu_seconds_total{groupname=~'.+'}[5m])) by (groupname) * 100",
        from: parseFloat((from / 1000).toFixed(3)),
        until: parseFloat((until / 1000).toFixed(3)),
        step: 28,
      });
      if (
        Array.isArray(response?.data?.data) &&
        response?.data?.data.length == 0
      ) {
        toast({
          title: "No Data",
          description:
            "No CPU usage data available for the selected time range.",
        });
        // eslint-disable-next-line @typescript-eslint/no-unused-expressions
        setError("No CPU usage data available for the selected time range"),
          setLoading(false);
        return;
      }
      if (response?.data?.error) {
        console.log("here");
        toast({
          title: "Error",
          description: response?.data?.error,
          variant: "destructive",
        });
        setLoading(false);
        setError(response?.data?.error);
        return;
      }
      const data = response?.data?.data.map((val, idx) => ({
        index: idx,
        timestamp: Number(val[0]) * 1000,
        value: parseFloat(String(val[1])),
      }));
      const [bottom, top] = getAxisYDomain(
        data,
        data[0].timestamp,
        data[data.length - 1].timestamp,
        1
      );
      setData(data);
      setBottom(bottom);
      setTop(top);
      setLeft(data[0].timestamp);
      setRight(data[data.length - 1].timestamp);
      setLoading(false);
    } catch (error) {
      console.log(error);
      setLoading(false);
      setError(error?.message);
      toast({
        title: "Error",
        description: "Unable to CPU Line Graph data",
        variant: "destructive",
      });
    }
  };

  function refreshLineGraph() {
    setRefresh((prev) => !prev);
  }

  useEffect(() => {
    fetchData();
  }, [refresh]);

  const formatXAxis = (tickItem: number) => {
    const date = new Date(tickItem);
    return date.toLocaleDateString("en-US", {
      month: "short",
      day: "numeric",
      hour: "numeric",
      minute: "numeric",
    });
  };

  const zoom = () => {
    if (
      refAreaLeft === null ||
      refAreaRight === null ||
      refAreaLeft === refAreaRight
    ) {
      setRefAreaLeft(null);
      setRefAreaRight(null);
      return;
    }

    const [newLeft, newRight] = [
      Math.min(refAreaLeft, refAreaRight),
      Math.max(refAreaLeft, refAreaRight),
    ];
    const [newBottom, newTop] = getAxisYDomain(data, newLeft, newRight, 1);

    setDate({
      from: newLeft,
      until: newRight,
    });

    setLeft(newLeft);
    setRight(newRight);
    setBottom(newBottom);
    setTop(newTop);
    setRefAreaLeft(null);
    setRefAreaRight(null);
  };

  const zoomOut = () => {
    const [newBottom, newTop] = getAxisYDomain(
      data,
      data[0].timestamp,
      data[data.length - 1].timestamp,
      1
    );
    setLeft(data[0].timestamp);
    setRight(data[data.length - 1].timestamp);
    setTop(newTop);
    setBottom(newBottom);
    setRefAreaLeft(null);
    setRefAreaRight(null);
    setDate({ from: "now-5m", until: "now" });
  };

  if (loading) {
    return <Loader />;
  }

  return (
    <Card className="w-full max-w-8xl mx-auto bg-gradient-to-br from-slate-900 to-slate-950 text-white shadow-xl">
      <CardHeader>
        <div className="flex justify-between">
          <div className="flex items-center gap-2">
            <Cpu className="w-6 h-6 text-blue-400" />
            <CardTitle className="text-2xl font-bold">CPU Usage</CardTitle>
          </div>
          <Button variant="outline" size="icon" onClick={refreshLineGraph}>
            <RefreshCcw />
          </Button>
        </div>
      </CardHeader>
      <CardContent>
        {error ? (
          <NotFound content="No data available" onRefresh={refreshLineGraph} />
        ) : (
          <>
            <div className="flex justify-between mb-4">
              <div className="flex flex-row space-x-2">
                <Button
                  onClick={zoomOut}
                  className="mb-4 bg-blue-600 hover:bg-blue-700 text-white"
                >
                  Zoom Out
                </Button>
              </div>
              <div className="flex flex-row space-x-2"></div>
            </div>
            <ChartContainer config={chartConfig} className="aspect-auto w-full">
              <div style={{ userSelect: "none" }}>
                <ResponsiveContainer width="100%" height={400}>
                  <LineChart
                    data={data}
                    margin={{ top: 20, right: 20, left: 20, bottom: 20 }}
                    onMouseDown={(e) =>
                      e &&
                      setRefAreaLeft(
                        e.activeLabel ? Number(e.activeLabel) : null
                      )
                    }
                    onMouseMove={(e) =>
                      e &&
                      refAreaLeft !== null &&
                      setRefAreaRight(
                        e.activeLabel ? Number(e.activeLabel) : null
                      )
                    }
                    onMouseUp={zoom}
                  >
                    <CartesianGrid strokeDasharray="3 3" />
                    <XAxis
                      allowDataOverflow
                      dataKey="timestamp"
                      domain={[left, right]}
                      type="number"
                      tickFormatter={formatXAxis}
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
                          // nameKey="kv_service"
                        />
                      }
                    />
                    <Line
                      yAxisId="1"
                      type="monotone"
                      dataKey="value"
                      name="KV Service"
                      stroke="hsl(var(--chart-1))"
                      dot={false}
                      strokeWidth={2}
                      activeDot={{ r: 8 }}
                      connectNulls
                    />
                    {refAreaLeft && refAreaRight && (
                      <ReferenceArea
                        yAxisId="1"
                        x1={refAreaLeft}
                        x2={refAreaRight}
                        strokeOpacity={0.3}
                      />
                    )}
                  </LineChart>
                </ResponsiveContainer>
              </div>
            </ChartContainer>
          </>
        )}
      </CardContent>
    </Card>
  );
};
