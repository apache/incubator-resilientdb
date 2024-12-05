import { useState, useCallback, useEffect } from "react";
import {
  Card,
  CardContent,
  CardHeader,
  CardTitle,
} from "@/components/ui/card";
import {
  LineChart,
  Line,
  XAxis,
  YAxis,
  Tooltip as RechartsTooltip,
  ResponsiveContainer,
} from "recharts";
import { Button } from "@/components/ui/button";
import { RefreshCw, Info, MemoryStick } from "lucide-react";
import {
  Tooltip,
  TooltipContent,
  TooltipProvider,
  TooltipTrigger,
} from "@/components/ui/tooltip";
import { middlewareApi } from "@/lib/api";

const MetricChart = ({ title, data, yAxisLabel, onRefresh }) => (
  <Card className="w-full max-w-8xl mx-auto bg-gradient-to-br from-slate-900 to-slate-950 text-white shadow-xl">
    <CardHeader>
      <CardTitle className="text-lg font-semibold text-white">
        {title}
      </CardTitle>
    </CardHeader>
    <CardContent>
      <ResponsiveContainer width="100%" height={200}>
        <LineChart
          data={data}
          margin={{ top: 5, right: 30, left: 20, bottom: 5 }}
        >
          <XAxis
            dataKey="name"
            stroke="#888888"
            fontSize={12}
            tickLine={false}
            axisLine={{ stroke: "#888888" }}
          />
          <YAxis
            stroke="#888888"
            fontSize={12}
            tickLine={false}
            axisLine={{ stroke: "#888888" }}
            tickFormatter={(value) => `${value}${yAxisLabel}`}
          />
          <RechartsTooltip
            contentStyle={{ backgroundColor: "#334155", border: "none" }}
            labelStyle={{ color: "#94a3b8" }}
            itemStyle={{ color: "#e2e8f0" }}
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
      <Button onClick={onRefresh} variant="outline" size="sm" className="mt-2">
        Refresh
      </Button>
    </CardContent>
  </Card>
);

export function MemoryMetricsGrid() {
  const [ioTimeData, setIoTimeData] = useState([]);
  const [diskRWData, setDiskRWData] = useState([]);
  const [diskIOPSData, setDiskIOPSData] = useState([]);
  const [diskWaitTimeData, setDiskWaitTimeData] = useState([]);
  const [loadingDiskIOPS, setLoadingDiskIOPS] = useState(false); // Loading state
  const [errorDiskIOPS, setErrorDiskIOPS] = useState(null); // Error state
  
  const fetchIoTimeData = async () => {
    try {
        setLoadingDiskIOPS(true); // Set loading state
        setErrorDiskIOPS(null); // Clear errors

        const until = new Date().getTime(); // Current timestamp in ms
        const from = new Date(until - 1 * 60 * 60 * 1000).getTime(); // 1 hour ago

        // Fetch data from the backend
        const response = await middlewareApi.post("/nodeExporter/getTimeSpentDoingIO", {
            from: parseFloat((from / 1000).toFixed(3)),
            until: parseFloat((until / 1000).toFixed(3)),
            step: 14,
        });

        const data = response?.data?.data.map((item) => ({
            name: new Date(item.time).toLocaleTimeString(), // Format time
            value: Math.floor(item.value * 1000), // Convert seconds to milliseconds
        }));

        console.log("Disk IO Time Data:", data);
        setIoTimeData(data); // Update state with data
        setLoadingDiskIOPS(false); // Reset loading state
    } catch (error) {
        console.error("Error fetching Disk IO Time data:", error);
        setErrorDiskIOPS(error?.message || "Failed to fetch Disk IO Time data.");
        setLoadingDiskIOPS(false); // Reset loading on error
    }
};

  const fetchDiskRWData = async () => {
    // Replace this with actual API call
    setDiskRWData(generateRandomData(20));
};

  const fetchDiskIOPSData = async () => {
    try {
      setLoadingDiskIOPS(true); // Indicate that loading has started
      setErrorDiskIOPS(null); // Clear any previous errors
  
      const until = new Date().getTime();
      const from = new Date(until - 1 * 60 * 60 * 1000).getTime();
      const step = 14; // Step size in seconds
  
      // Make the API call
      const response = await middlewareApi.post("/nodeExporter/getDiskIOPS", {
        query: "rate(node_disk_writes_completed_total{device='sda',job='node_exporter'}[5m])",
        from: parseFloat((from / 1000).toFixed(3)),
        until: parseFloat((until / 1000).toFixed(3)),
        step,
      });
  
      // Process the response to format data for the chart
      const data = response?.data?.data.map((val) => {
        // Round the timestamp to the nearest minute (floor seconds)
        const timestamp = Math.floor(Number(val[0]) / 60) * 60;
        
        // Round the value to two decimal places
        const roundedValue = Math.floor(parseFloat(val[1]) * 100) / 100;
        
        return {
          name: new Date(timestamp * 1000).toLocaleTimeString(), // Format timestamp
          value: roundedValue, // Round IOPS rate value
        };
      });

      console.log("Disk IOPS Data:", data);
      setDiskIOPSData(data); // Update state with the new data
      setLoadingDiskIOPS(false); // Loading is complete
    } catch (error) {
      console.error("Error fetching DiskIOPS data:", error);
      setErrorDiskIOPS(error?.message || "Failed to fetch DiskIOPS data.");
      setLoadingDiskIOPS(false); // Ensure loading is complete even on error
    }
};

const fetchDiskWaitTimeData = async () => {
  try {
    setLoadingDiskIOPS(true); // Indicate loading state
    setErrorDiskIOPS(null); // Clear previous errors

    const until = new Date().getTime(); // Current time
    const from = new Date(until - 1 * 60 * 60 * 1000).getTime(); // Last 1 hour
    const step = 14; // Step size in seconds

    // Call the backend API
    const response = await middlewareApi.post("/nodeExporter/getDiskWaitTime", {
      from: parseFloat((from / 1000).toFixed(3)),
      until: parseFloat((until / 1000).toFixed(3)),
      step,
    });

    // Format the data for the chart
    const processMetricData = (metricData, label) =>
      metricData.map(({ time, value }) => ({
        name: new Date(time).toLocaleTimeString(), // Format timestamp
        value: Math.floor(value * 1000), // Convert to milliseconds
        metric: label, // Identify metric type (read/write)
      }));

    const readData = processMetricData(response?.data?.data?.readWaitTime, "Read");
    const writeData = processMetricData(response?.data?.data?.writeWaitTime, "Write");

    // Combine data and update state
    setDiskWaitTimeData([...readData, ...writeData]);
    setLoadingDiskIOPS(false); // Reset loading state
  } catch (error) {
    console.error("Error fetching Disk Wait Time data:", error);
    setErrorDiskIOPS(error?.message || "Failed to fetch Disk Wait Time data.");
    setLoadingDiskIOPS(false); // Reset loading state on error
  }
};

  useEffect(() => {
    fetchIoTimeData();
    fetchDiskRWData();
    fetchDiskIOPSData();
    fetchDiskWaitTimeData();
  }, []);

  const metrics = [
    {
      title: "Time Spent Doing I/O",
      yAxisLabel: "ms",
      data: ioTimeData,
      onRefresh: fetchIoTimeData,
    },
    {
      title: "Disk R/W Data",
      yAxisLabel: "B",
      data: diskRWData,
      onRefresh: fetchDiskRWData,
    },
    {
      title: "Disk IOPS",
      yAxisLabel: "",
      data: diskIOPSData,
      onRefresh: fetchDiskIOPSData,
    },
    {
      title: "Disk Average Wait Time",
      yAxisLabel: "ms",
      data: diskWaitTimeData,
      onRefresh: fetchDiskWaitTimeData,
    },
  ];

  return (
    <Card className="bg-slate-900 p-6">
      <CardHeader className="flex flex-row items-center justify-between pb-2">
        <div className="flex items-center gap-2">
          <MemoryStick className="w-6 h-6 text-blue-400" />
          <CardTitle className="text-2xl font-bold">Memory Metrics</CardTitle>
        </div>
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
      </CardHeader>
      <CardContent>
        <div className="grid grid-cols-2 gap-6">
          {metrics.map((metric) => (
            <MetricChart
              key={metric.title}
              title={metric.title}
              data={metric.data}
              yAxisLabel={metric.yAxisLabel}
              onRefresh={metric.onRefresh}
            />
          ))}
        </div>
      </CardContent>
    </Card>
  );
}

const generateRandomData = (length: number) =>
  Array.from({ length }, (_, i) => ({
    name: i.toString(),
    value: Math.floor(Math.random() * 100),
  }));