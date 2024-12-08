import { useState, useEffect, useContext } from "react";
import { Card, CardContent, CardHeader, CardTitle } from "@/components/ui/card";
import {
  LineChart,
  Line,
  XAxis,
  YAxis,
  Tooltip as RechartsTooltip,
  ResponsiveContainer,
} from "recharts";
import { Button } from "@/components/ui/button";
import {
  MemoryStick,
  MemoryStickIcon,
  RefreshCcw,
  InfoIcon,
  Info,
} from "lucide-react";
import {
  Tooltip,
  TooltipContent,
  TooltipProvider,
  TooltipTrigger,
} from "@/components/ui/tooltip";
import { middlewareApi } from "@/lib/api";
import { ModeType } from "@/components/toggle";
import { ModeContext } from "@/hooks/context";
import { useToast } from "@/hooks/use-toast";
import {
  DiskRWData,
  TimeDuringIOData,
  DiskIOPSData,
  DiskWaitTimeData,
} from "@/static/MemoryStats";

const MetricChart = ({ title, data, yAxisLabel, onRefresh, info }) => {
  return (
    <Card className="w-full max-w-8xl mx-auto bg-gradient-to-br from-slate-900 to-slate-950 text-white shadow-xl">
      <CardHeader>
        <CardTitle className="text-lg font-semibold text-white">
          <div className="flex justify-between">
            <div className="flex items-center gap-2">
              <MemoryStickIcon className="w-6 h-6 text-blue-400" />
              <CardTitle className="text-2xl font-bold">{title}</CardTitle>
            </div>
            <div className="flex items-center gap-2">
              <Button variant="outline" size="icon" onClick={onRefresh}>
                <RefreshCcw />
              </Button>
              <TooltipProvider>
                <Tooltip>
                  <TooltipTrigger asChild>
                    <button
                      className="p-2 bg-slate-700 text-slate-400 hover:text-white hover:bg-slate-600 transition-colors duration-200 ease-in-out rounded"
                    >
                      <Info size={18.5} />
                    </button>
                  </TooltipTrigger>
                  <TooltipContent>
                    <p className="w-full max-w-sm break-words">{info}</p>
                  </TooltipContent>
                </Tooltip>
              </TooltipProvider>
            </div>
          </div>
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
              // tickFormatter={formatXAxis}
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
      </CardContent>
    </Card>
  );
};

export function MemoryMetricsGrid() {
  const { toast } = useToast();
  const mode = useContext<ModeType>(ModeContext);
  const [ioTimeData, setIoTimeData] = useState([]);
  const [diskRWData, setDiskRWData] = useState([]);
  const [diskIOPSData, setDiskIOPSData] = useState([]);
  const [diskWaitTimeData, setDiskWaitTimeData] = useState([]);
  const [loadingDiskIOPS, setLoadingDiskIOPS] = useState(false); // Loading state
  const [errorDiskIOPS, setErrorDiskIOPS] = useState(null); // Error state

  const fetchIoTimeData = async () => {
    if (mode === "offline") {
      setIoTimeData(TimeDuringIOData?.data);
      toast({
        title: "Offline Mode",
        description: "Using sample data in offline mode.",
        variant: "default",
      });
      return;
    }

    try {
      setLoadingDiskIOPS(true); // Set loading state
      setErrorDiskIOPS(null); // Clear errors

      const until = new Date().getTime(); // Current timestamp in ms
      const from = new Date(until - 1 * 60 * 60 * 1000).getTime(); // 1 hour ago

      // Fetch data from the backend
      const response = await middlewareApi.post(
        "/nodeExporter/getTimeSpentDoingIO",
        {
          from: parseFloat((from / 1000).toFixed(3)),
          until: parseFloat((until / 1000).toFixed(3)),
          step: 14,
        }
      );

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
    if (mode === "offline") {
      setDiskRWData(DiskRWData?.data?.writeMerged);
      return;
    }
    try {
      setLoadingDiskIOPS(true); // Indicate loading state
      setErrorDiskIOPS(null); // Clear previous errors

      const until = new Date().getTime(); // Current time
      const from = new Date(until - 1 * 60 * 60 * 1000).getTime(); // Last 1 hour
      const step = 14; // Step size in seconds

      // Call the backend API
      const response = await middlewareApi.post("/nodeExporter/getDiskRWData", {
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

      const readMergedData = processMetricData(
        response?.data?.data?.readMerged,
        "Read Merged"
      );
      const writeMergedData = processMetricData(
        response?.data?.data?.writeMerged,
        "Write Merged"
      );

      // Combine data and update state
      setDiskRWData([...readMergedData, ...writeMergedData]);
      setLoadingDiskIOPS(false); // Reset loading state
    } catch (error) {
      console.error("Error fetching Disk Merged Data:", error);
      setErrorDiskIOPS(error?.message || "Failed to fetch Disk Merged Data.");
      setLoadingDiskIOPS(false); // Reset loading state on error
    }
  };

const fetchDiskIOPSData = async () => {
  if (mode === "offline") {
    setDiskIOPSData(DiskIOPSData?.data) 
    return
  }
  try {
    setLoadingDiskIOPS(true); // Indicate loading state
    setErrorDiskIOPS(null); // Clear previous errors
  
    const until = new Date().getTime(); // Current time
    const from = new Date(until - 1 * 60 * 60 * 1000).getTime(); // Last 1 hour
    const step = 14; // Step size in seconds
  
    // Call the backend API
    const response = await middlewareApi.post("/nodeExporter/getDiskIOPS", {
      from: parseFloat((from / 1000).toFixed(3)),
      until: parseFloat((until / 1000).toFixed(3)),
      step,
    });
  
    // Format the data for the chart
    const processMetricData = (metricData) =>
      metricData.map(({ time, value }) => ({
        name: new Date(time).toLocaleTimeString(), // Format timestamp for X-axis
        value: parseFloat(value).toFixed(2), // Limit to 2 decimal places
      }));
  
    const formattedData = processMetricData(response?.data?.data || []);
  
    console.log("Formatted Disk IOPS Data:", formattedData);
    setDiskIOPSData(formattedData); // Update state with formatted data
    setLoadingDiskIOPS(false); // Reset loading state
  } catch (error) {
    console.error("Error fetching Disk IOPS data:", error);
    setErrorDiskIOPS(error?.message || "Failed to fetch Disk IOPS data.");
    setLoadingDiskIOPS(false); // Reset loading state on error
  }        
};

  const fetchDiskWaitTimeData = async () => {
    if (mode === "offline") {
      setDiskWaitTimeData(DiskWaitTimeData?.data?.writeWaitTime);
      return;
    }
    try {
      setLoadingDiskIOPS(true); // Indicate loading state
      setErrorDiskIOPS(null); // Clear previous errors

      const until = new Date().getTime(); // Current time
      const from = new Date(until - 1 * 60 * 60 * 1000).getTime(); // Last 1 hour
      const step = 14; // Step size in seconds

      // Call the backend API
      const response = await middlewareApi.post(
        "/nodeExporter/getDiskWaitTime",
        {
          from: parseFloat((from / 1000).toFixed(3)),
          until: parseFloat((until / 1000).toFixed(3)),
          step,
        }
      );

      // Format the data for the chart
      const processMetricData = (metricData, label) =>
        metricData.map(({ time, value }) => ({
          name: new Date(time).toLocaleTimeString(), // Format timestamp
          value: Math.floor(value * 1000), // Convert to milliseconds
          metric: label, // Identify metric type (read/write)
        }));

      const readData = processMetricData(
        response?.data?.data?.readWaitTime,
        "Read"
      );
      const writeData = processMetricData(
        response?.data?.data?.writeWaitTime,
        "Write"
      );

      // Combine data and update state
      setDiskWaitTimeData([...readData, ...writeData]);
      setLoadingDiskIOPS(false); // Reset loading state
    } catch (error) {
      console.error("Error fetching Disk Wait Time data:", error);
      setErrorDiskIOPS(
        error?.message || "Failed to fetch Disk Wait Time data."
      );
      setLoadingDiskIOPS(false); // Reset loading state on error
    }
  };

  useEffect(() => {
    fetchIoTimeData();
    fetchDiskRWData();
    fetchDiskIOPSData();
    fetchDiskWaitTimeData();
  }, [mode]);

  const metrics = [
    {
      title: "Time Spent Doing I/O",
      yAxisLabel: "ms",
      data: ioTimeData,
      info: "Percentage of elapsed time during which I/O requests were issued to the device (bandwidth utilization for the device). Device saturation occurs when this value is close to 100% for devices serving requests serially. But for devices serving requests in parallel, such as RAID arrays and modern SSDs, this number does not reflect their performance limits.",
      onRefresh: fetchIoTimeData,
    },
    {
      title: "Disk R/W Data",
      yAxisLabel: "B",
      data: diskRWData,
      info: "The number of bytes read from or written to the device per second",
      onRefresh: fetchDiskRWData,
    },
    {
      title: "Disk IOPS",
      yAxisLabel: "",
      data: diskIOPSData,
      info: "The number (after merges) of I/O requests completed per second for the device",
      onRefresh: fetchDiskIOPSData,
    },
    {
      title: "Disk Average Wait Time",
      yAxisLabel: "ms",
      data: diskWaitTimeData,
      info: "The average time for requests issued to the device to be served. This includes the time spent by the requests in queue and the time spent servicing them.",
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
              info={metric.info}
            />
          ))}
        </div>
      </CardContent>
    </Card>
  );
}
