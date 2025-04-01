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

//@ts-nocheck
/* eslint-disable react-hooks/exhaustive-deps */
import { Card, CardContent, CardHeader, CardTitle } from "@/components/ui/card";
import GaugeChart from "react-gauge-chart";
import { Progress } from "@/components/ui/progress";
import { Database, Info, InfoIcon, RefreshCcw, RefreshCw } from "lucide-react";
import { useContext, useEffect, useState } from "react";
import { middlewareApi } from "@/lib/api";
import { Loader } from "@/components/ui/loader";
import { Button } from "@/components/ui/button";
import { Link } from "react-router-dom";
import {
  Tooltip,
  TooltipContent,
  TooltipProvider,
  TooltipTrigger,
} from "@/components/ui/tooltip";
import {
  Table,
  TableBody,
  TableCell,
  TableHead,
  TableHeader,
  TableRow,
} from "@/components/ui/table";
import { NotFound } from "@/components/ui/not-found";
import { useToast } from "@/hooks/use-toast";
import { ModeType } from "@/components/toggle";
import { ModeContext } from "@/hooks/context";

interface LevelDBStat {
  Level: string;
  Files: string;
  "Size(MB)": string;
  "Time(sec)": string;
  "Read(MB)": string;
  "Write(MB)": string;
}

interface StorageMetrics {
  ext_cache_hit_ratio: number;
  level_db_approx_mem_size: number;
  level_db_stats: LevelDBStat[];
  max_resident_set_size: number;
  resident_set_size: number;
  num_reads: number;
  num_writes: number;
}

interface MemoryMetric {
  name: string;
  value: string | number;
  description: string;
  link: string;
}

interface CacheHitRatioProps {
  ratio: number;
}

interface LevelDbCardProps {
  levelDbSize: number;
  rssSize: number;
  numReads: number;
  numWrites: number;
}

interface LevelDBStatsTableProps {
  stats: LevelDBStat[];
}
const CacheHitGauge = () => {
  const [isCalculating, setIsCalculating] = useState(false);
  const [p99Value, setP99Value] = useState(0);
  const [animatedValue, setAnimatedValue] = useState(0);

  const calculateP99 = async () => {
    setIsCalculating(true);
    setP99Value(0);
    setAnimatedValue(0);

    try {
      const response = await middlewareApi.post("/transactions/calculateP99", {
        samples: 100,
      });
      const data = response?.data;
      const finalValue = data.p99;
      setP99Value(finalValue);
      setIsCalculating(false);
    } catch (error) {
      console.error("Error calculating P99:", error);
      setIsCalculating(false);
    }
  };

  function calculatPercent() {
    return p99Value / 5000;
  }

  return (
    <Card className="flex-1">
      <CardHeader>
        <CardTitle>P99 Latency Test (Set Method)</CardTitle>
      </CardHeader>
      <CardContent className="p-6 flex flex-col items-center justify-center">
        {!isCalculating && p99Value === 0 ? (
          <Button
            onClick={calculateP99}
            className="w-32 h-32 rounded-full text-2xl font-bold"
            disabled={isCalculating}
          >
            GO
          </Button>
        ) : (
          <div className="w-full h-48 relative">
            <GaugeChart
              style={{ height: "12rem" }}
              id="p99-gauge-chart"
              nrOfLevels={10}
              colors={["#10B981", "#FBBF24", "#EF4444"]}
              percent={calculatPercent()}
              textColor="#000000"
              needleColor="#5C6BC0"
              needleBaseColor="#3949AB"
              arcWidth={0.2}
              formatTextValue={() => ""}
            />
            <div className="absolute inset-0 flex items-center justify-center">
              <div className="text-xl font-bold">
                {isCalculating
                  ? `${animatedValue.toFixed(1)}s`
                  : `${p99Value.toFixed(1)}ms`}
              </div>
            </div>
          </div>
        )}
        {isCalculating && (
          <p className="mt-1 text-center text-muted-foreground">
            Calculating P99 latency...
          </p>
        )}
        {!isCalculating && p99Value > 0 && (
          <Button onClick={calculateP99} className="mt-1">
            Recalculate
          </Button>
        )}
      </CardContent>
    </Card>
  );
};

export function StorageEngineMetrics() {
  const mode = useContext<ModeType>(ModeContext);
  const { toast } = useToast();
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState(null);
  const [refresh, setRefresh] = useState(false);
  const [data, setData] = useState<StorageMetrics>(null);

  useEffect(() => {
    if (mode === "offline") {
      toast({
        title: "Offline Mode",
        description: "Storage Engine Metrics Cannot be fetched in offline mode",
      });
      return;
    }
    fetchTransactionData();
  }, [refresh]);

  async function fetchTransactionData() {
    try {
      setLoading(true);
      const response = await middlewareApi.post(
        "/statsExporter/getTransactionData"
      );
      setData(response?.data);
      toast({
        title: "Data Updated",
        description: "Storage Engine Metrics Fetched Successfully",
      });
      setLoading(false);
    } catch (error) {
      setLoading(false);
      setError(error);
    }
  }

  return (
    <Card className="flex-1 w-full max-w-8xl mx-auto bg-gradient-to-br from-slate-900 to-slate-950 text-white shadow-xl">
      <CardHeader className="flex flex-row items-center justify-between pb-2">
        <div className="flex items-center gap-2">
          <Database className="w-6 h-6 text-blue-400" />
          <CardTitle className="text-2xl font-bold">
            {" "}
            Storage Engine Metrics
          </CardTitle>
        </div>
        <div className="flex items-center gap-2">
          <Button
            variant="outline"
            size="icon"
            onClick={() => setRefresh((prev) => !prev)}
          >
            <RefreshCcw />
          </Button>
          <TooltipProvider>
            <Tooltip>
              <TooltipTrigger asChild>
                <button
                  className="p-2 bg-slate-700 text-slate-400 hover:text-white hover:bg-slate-600 transition-colors duration-200 ease-in-out rounded"
                  //onClick={() => window.open("https://gmail.com", "_blank")}
                >
                  <Info size={18.5} />
                </button>
              </TooltipTrigger>
              <TooltipContent>
                <p>Click for more information about these metrics</p>
              </TooltipContent>
            </Tooltip>
          </TooltipProvider>
        </div>
      </CardHeader>
      {loading ? (
        <Loader className="h-[400px]" />
      ) : (
        <CardContent className="space-y-4">
          <div className="flex flex-row justify-between space-x-2">
            <CacheHitRatio ratio={data?.ext_cache_hit_ratio} />
            <LevelDBSizeCard size={data?.level_db_approx_mem_size} />
            <LevelDbCard
              levelDbSize={data?.level_db_approx_mem_size}
              rssSize={data?.resident_set_size || 0}
              numReads={data?.num_reads}
              numWrites={data?.num_writes}
            />
            <CacheHitGauge />
          </div>
          <div className="flex flex-row justify-between space-x-2">
            {/* <LevelDBStatsTable stats={data?.level_db_stats} /> */}
          </div>
        </CardContent>
      )}
    </Card>
  );
}

export function CacheHitRatio({ ratio }: CacheHitRatioProps) {
  const percentage = Math.round(ratio * 100);
  return (
    <Card className="w-full max-w-md flex-1">
      <CardHeader>
        <CardTitle>Cache Hit Ratio</CardTitle>
      </CardHeader>
      <CardContent>
        <div className="flex items-center justify-between mb-2">
          <span className="text-sm font-medium">Hit Rate</span>
          <span className="text-2xl font-bold">{percentage || 0}%</span>
        </div>
        <Progress value={percentage || 0} className="h-2" />
        <p className="mt-2 text-sm text-muted-foreground">
          {percentage || 0}% of requests are served from the LRU cache.
        </p>
      </CardContent>
    </Card>
  );
}

export function LevelDBStatsTable({ stats }: LevelDBStatsTableProps) {
  if (!stats) {
    return (
      <Card>
        <NotFound
          onRefresh={function (): void {
            throw new Error("Function not implemented.");
          }}
        />
      </Card>
    );
  }
  return (
    <Card className="w-full max-w-md flex-1">
      <CardHeader>
        <CardTitle>Level DB Stats</CardTitle>
      </CardHeader>
      <CardContent>
        <Table>
          <TableHeader>
            <TableRow>
              <TableHead>Level</TableHead>
              <TableHead>Files</TableHead>
              <TableHead>Size (MB)</TableHead>
              <TableHead>Time (sec)</TableHead>
              <TableHead>Read (MB)</TableHead>
              <TableHead>Write (MB)</TableHead>
            </TableRow>
          </TableHeader>
          <TableBody>
            {stats.map((stat) => (
              <TableRow key={stat.Level}>
                <TableCell>{stat.Level}</TableCell>
                <TableCell>{stat.Files}</TableCell>
                <TableCell>{stat["Size(MB)"]}</TableCell>
                <TableCell>{stat["Time(sec)"]}</TableCell>
                <TableCell>{stat["Read(MB)"]}</TableCell>
                <TableCell>{stat["Write(MB)"]}</TableCell>
              </TableRow>
            ))}
          </TableBody>
        </Table>
      </CardContent>
    </Card>
  );
}

//TODO: refactor
export function LevelDbCard({
  levelDbSize,
  rssSize,
  numReads,
  numWrites,
}: LevelDbCardProps) {
  const size = Math.round(levelDbSize / 1000);

  const memoryMetrics: MemoryMetric[] = [
    // {
    //   name: "Level DB Approx Size",
    //   value: String(size) + " KB",
    //   description: "Estimated size of the LevelDB database",
    //   link: "https://github.com/google/leveldb",
    // },
    {
      name: "RSS",
      value: rssSize ? String(rssSize || 0) + " MB" : 0,
      description: "Current Resident Set Size (physical memory used)",
      link: "https://en.wikipedia.org/wiki/Resident_set_size",
    },
    {
      name: "Number of Reads by process",
      value: numReads,
      description:
        "Number of times the file system had to read from the disk on behalf of processes.",
      link: "https://www.gnu.org/software/libc/manual/html_node/Resource-Usage.html",
    },
    {
      name: "Number of Writes by process",
      value: numWrites,
      description:
        "The number of times the file system had to write to the disk on behalf of processes.",
      link: "https://www.gnu.org/software/libc/manual/html_node/Resource-Usage.html",
    },
  ];

  return (
    <Card className="w-full max-w-2xl">
      <CardHeader>
        <CardTitle>Memory Footprint</CardTitle>
      </CardHeader>
      <CardContent>
        <Table>
          <TableHeader>
            <TableRow>
              <TableHead>Metric</TableHead>
              <TableHead>Value</TableHead>
              <TableHead>Info</TableHead>
            </TableRow>
          </TableHeader>
          <TableBody>
            {memoryMetrics.map((metric) => (
              <TableRow key={metric.name}>
                <TableCell>{metric.name}</TableCell>
                <TableCell>{metric.value || 0}</TableCell>
                <TableCell>
                  <TooltipProvider>
                    <Tooltip>
                      <TooltipTrigger asChild>
                        <Link
                          to={metric.link}
                          target="_blank"
                          rel="noopener noreferrer"
                        >
                          <InfoIcon className="h-4 w-4 text-muted-foreground hover:text-foreground" />
                        </Link>
                      </TooltipTrigger>
                      <TooltipContent>
                        <p>{metric.description}</p>
                      </TooltipContent>
                    </Tooltip>
                  </TooltipProvider>
                </TableCell>
              </TableRow>
            ))}
          </TableBody>
        </Table>
      </CardContent>
    </Card>
  );
}

interface LevelDBSizeCardProps {
  size: number; // Size of the database, e.g., "256 MB"
}

export function LevelDBSizeCard({ size }: LevelDBSizeCardProps) {
  return (
    <Card className="w-full max-w-xs">
      <CardHeader className="flex flex-row items-center justify-between space-y-0 pb-2">
        <CardTitle className="text-sm font-medium">Database Size</CardTitle>
        <Database className="h-4 w-4 text-muted-foreground" />
      </CardHeader>
      <div className="flex flex-col justify-center items-center flex-grow p-6">
        <div className="text-3xl sm:text-4xl md:text-5xl font-bold text-center">
          {Math.floor((size || 0) / 1000)} KB
        </div>
        <p className="text-xs text-muted-foreground mt-2">LevelDB Storage</p>
        <p className="text-sm text-center mt-4 max-w-xs text-muted-foreground">
          Size of data present in LevelDB with compaction in place
        </p>
      </div>
    </Card>
  );
}

interface LevelDBStatsProps {
  totalSize: string;
  stats: LevelDBStat[];
  className?: string;
}

export function LevelDBStats({
  totalSize,
  stats,
  className,
}: LevelDBStatsProps) {
  const totalFiles = stats.reduce((sum, stat) => sum + stat.Files, 0);
  const totalSizeMB = stats.reduce((sum, stat) => sum + stat["Size(MB)"], 0);

  return (
    <div className={`space-y-4 ${className}`}>
      <Card className="w-full">
        <CardHeader className="flex flex-row items-center justify-between space-y-0 pb-2">
          <CardTitle className="text-2xl font-bold">
            LevelDB Statistics
          </CardTitle>
          <Database className="h-6 w-6 text-muted-foreground" />
        </CardHeader>
        <CardContent>
          <div className="grid grid-cols-1 md:grid-cols-3 gap-4">
            <StatsCard
              title="Total Size"
              value={totalSize}
              description="Overall database size"
            />
            <StatsCard
              title="Total Files"
              value={totalFiles.toString()}
              description="Number of SSTable files"
            />
            <StatsCard
              title="Total Size (MB)"
              value={`${totalSizeMB.toFixed(2)} MB`}
              description="Sum of all level sizes"
            />
          </div>
        </CardContent>
      </Card>

      <Card className="w-full">
        <CardHeader>
          <CardTitle>Level DB Stats</CardTitle>
        </CardHeader>
        <CardContent>
          <Table>
            <TableHeader>
              <TableRow>
                <TableHead>Level</TableHead>
                <TableHead>Files</TableHead>
                <TableHead>Size (MB)</TableHead>
                <TableHead>Time (sec)</TableHead>
                <TableHead>Read (MB)</TableHead>
                <TableHead>Write (MB)</TableHead>
              </TableRow>
            </TableHeader>
            <TableBody>
              {stats.map((stat) => (
                <TableRow>
                  <TableCell>{stat.Level}</TableCell>
                  <TableCell>{stat.Files}</TableCell>
                  <TableCell>{stat["Size(MB)"]}</TableCell>
                  <TableCell>{stat["Time(sec)"]}</TableCell>
                  <TableCell>{stat["Read(MB)"]}</TableCell>
                  <TableCell>{stat["Write(MB)"]}</TableCell>
                </TableRow>
              ))}
            </TableBody>
          </Table>
        </CardContent>
      </Card>
    </div>
  );
}

function StatsCard({
  title,
  value,
  description,
}: {
  title: string;
  value: string;
  description: string;
}) {
  return (
    <Card>
      <CardHeader className="flex flex-row items-center justify-between space-y-0 pb-2">
        <CardTitle className="text-sm font-medium">{title}</CardTitle>
      </CardHeader>
      <CardContent>
        <div className="text-2xl font-bold">{value}</div>
        <p className="text-xs text-muted-foreground">{description}</p>
      </CardContent>
    </Card>
  );
}
