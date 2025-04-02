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
import {
  Link,
  Database,
  Cuboid,
  Hourglass,
  AlertCircle,
  RefreshCw,
} from "lucide-react";
import {
  Card,
  CardContent,
  CardDescription,
  CardHeader,
  CardTitle,
} from "../ui/card";
import { NotFound } from "../ui/not-found";
import { useEffect, useState } from "react";
import { middlewareApi } from "@/lib/api";
import { Skeleton } from "../ui/skeleton";
import { formatSeconds } from "@/lib/utils";
import { Block, BlockchainTable } from "./table";
import {
  ChartContainer,
  ChartTooltipContent,
  ChartTooltip,
} from "../ui/LineGraphChart";
import { Bar, BarChart, CartesianGrid, XAxis, YAxis } from "recharts";
import { transactionHistoryData } from "@/static/transactionHistory";
import { Button } from "../ui/button";
import { decodeDeltaEncoding } from "@/static/encoding";

type BlockchainConfig = {
  blockNum: number;
  chainAge: number;
  checkpointWaterMark: number;
  clientBatchNum: number;
  clientBatchWaitTime: number;
  clientTimeoutMs: number;
  inputWorkerNum: number;
  maxMaliciousReplicaNum: number;
  maxProcessTxn: number;
  minDataReceiveNum: number;
  outputWorkerNum: number;
  replicaNum: number;
  transactionNum: number;
  workerNum: number;
};

type ExplorerCardProps = {
  loading: boolean;
  data: BlockchainConfig;
};

export function Explorer() {
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState(null);
  const [configData, setData] = useState<BlockchainConfig>(
    {} as BlockchainConfig
  );

  async function fetchExplorerData() {
    try {
      setLoading(true);
      const response = await middlewareApi.get("/explorer/getExplorerData");
      setData(response?.data[0]);
      setLoading(false);
    } catch (error) {
      setLoading(false);
      setError(error);
    }
  }

  useEffect(() => {
    fetchExplorerData();
  }, []);

  return (
    <div className="flex flex-col space-y-4">
      <Card className="w-4/5 max-w-8xl mx-auto bg-gradient-to-br from-slate-900 to-slate-800 text-white shadow-xl">
        <CardHeader className="border-b border-slate-700">
          <div className="flex flex-col sm:flex-row items-center justify-between gap-4">
            <div className="flex items-center gap-2">
              <Cuboid className="w-6 h-6 text-blue-400" />
              <CardTitle className="text-2xl font-bold">Data View</CardTitle>
            </div>
          </div>
          <CardDescription className="mt-4 text-slate-300">
            Get Information on ResDB's internal blockchain data
          </CardDescription>
        </CardHeader>
        <CardContent>
          <main className="p-4 space-y-4">
            <div className="grid md:grid-cols-2 gap-4">
              <DatabaseCard loading={loading} data={configData} />
              <ChainInfoCard loading={loading} data={configData} />
            </div>
            <div className="grid md:grid-cols-1 gap-4">
              <MiscellaneousDataCard loading={loading} data={configData} />
              <TransactionHistoryCard />
            </div>
          </main>
        </CardContent>
      </Card>
      {configData ? (
        <BlocksData metadata={configData} />
      ) : (
        <div>Loading...</div>
      )}
    </div>
  );
}
function DatabaseCard({ loading, data }: ExplorerCardProps) {
  return (
    <Card className="h-[400px] w-full max-w-8xl mx-auto bg-gradient-to-br from-slate-900 to-slate-950 text-white shadow-xl">
      <CardHeader>
        <div className="flex justify-between">
          <div className="flex items-center gap-2">
            <Database className="w-6 h-6 text-blue-400" />
            <CardTitle className="text-2xl font-bold">
              Resilient DB Data
            </CardTitle>
          </div>
        </div>
      </CardHeader>
      <CardContent>
        {!loading ? (
          <div className="grid grid-cols-2 gap-4 p-4">
            <div>
              <div className="text-sm text-gray-300">Active Replicas</div>
              <div className="text-2xl font-semibold text-white">
                {data?.replicaNum}
              </div>
            </div>
            <div>
              <div className="text-sm text-gray-300">Workers</div>
              <div className="text-2xl font-semibold text-white">
                {data?.workerNum}
              </div>
            </div>
            <div>
              <div className="text-sm text-gray-300">Input Workers</div>
              <div className="text-2xl font-semibold text-white">
                {data?.inputWorkerNum}
              </div>
            </div>
            <div>
              <div className="text-sm text-gray-300">Output Workers</div>
              <div className="text-2xl font-semibold text-white">
                {data?.outputWorkerNum}
              </div>
            </div>
            <div>
              <div className="text-sm text-gray-300">
                Max Malicious Replicas
              </div>
              <div className="text-2xl font-semibold text-white">
                {data?.maxMaliciousReplicaNum}
              </div>
            </div>
            <div>
              <div className="text-sm text-gray-300">Minimum Data Received</div>
              <div className="text-2xl font-semibold text-white">
                {data?.minDataReceiveNum}
              </div>
            </div>
          </div>
        ) : (
          <div className="grid grid-cols-2 gap-4 p-4">
            {[...Array(6)].map((_, i) => (
              <div key={i}>
                <Skeleton className="h-4 w-24 mb-2" />
                <Skeleton className="h-8 w-16" />
              </div>
            ))}
          </div>
        )}
      </CardContent>
    </Card>
  );
}
function ChainInfoCard({ loading, data }: ExplorerCardProps) {
  return (
    <Card className="h-[400px] w-full max-w-8xl mx-auto bg-gradient-to-br from-slate-900 to-slate-950 text-white shadow-xl">
      <CardHeader>
        <div className="flex justify-between">
          <div className="flex items-center gap-2">
            <Link className="w-6 h-6 text-blue-400" />
            <CardTitle className="text-2xl font-bold">
              Chain Information
            </CardTitle>
          </div>
        </div>
      </CardHeader>
      <CardContent>
        {!loading ? (
          <>
            <div className="space-y-4 p-4 flex flex-col">
              <div className="flex flex-row space-x-4 justify-between w-3/4">
                <div>
                  <div className="text-sm text-gray-300">Blocks</div>
                  <div className="text-2xl font-semibold text-white">
                    {data?.blockNum}
                  </div>
                </div>
                <div>
                  <div className="text-sm text-gray-300">
                    Transactions Committed
                  </div>
                  <div className="text-2xl font-semibold text-white">
                    {data?.transactionNum}
                  </div>
                </div>
              </div>
            </div>
            <div className="mt-4 p-4">
              <div className="h-2 w-3/4 bg-blue-500/20 rounded-full overflow-hidden">
                <div className="h-full w-3/4 bg-blue-500 rounded-full" />
              </div>
              <div className="mt-2 text-sm text-gray-300">
                {formatSeconds(data?.chainAge)} ({data?.chainAge}s)
              </div>
            </div>
          </>
        ) : (
          <>
            <div className="space-y-4 p-4 flex flex-col">
              <div className="flex flex-row space-x-4 justify-between w-3/4">
                {[...Array(2)].map((_, i) => (
                  <div key={i}>
                    <Skeleton className="h-4 w-24 mb-2" />
                    <Skeleton className="h-8 w-16" />
                  </div>
                ))}
              </div>
            </div>
            <div className="mt-4 p-4">
              <Skeleton className="h-2 w-3/4 mb-2" />
              <Skeleton className="h-4 w-full" />
            </div>
          </>
        )}
      </CardContent>
    </Card>
  );
}
function MiscellaneousDataCard({ loading, data }: ExplorerCardProps) {
  return (
    <Card className="h-full w-full max-w-8xl mx-auto bg-gradient-to-br from-slate-900 to-slate-950 text-white shadow-xl">
      <CardHeader>
        <div className="flex justify-between">
          <div className="flex items-center gap-2">
            <Database className="w-6 h-6 text-blue-400" />
            <CardTitle className="text-2xl font-bold">Other Data</CardTitle>
          </div>
        </div>
      </CardHeader>
      <CardContent>
        {!loading ? (
          <div className="grid grid-cols-3 gap-12 p-4">
            <div>
              <div className="text-sm text-gray-300">Client Batch Size</div>
              <div className="text-2xl font-semibold text-white">
                {data?.clientBatchNum}
              </div>
            </div>
            <div>
              <div className="text-sm text-gray-300">
                Client Batch Wait Time (MS)
              </div>
              <div className="text-2xl font-semibold text-white">
                {data?.clientBatchWaitTime}
              </div>
            </div>
            <div>
              <div className="text-sm text-gray-300">Client Timeout (MS)</div>
              <div className="text-2xl font-semibold text-white">
                {data?.clientTimeoutMs}
              </div>
            </div>
            <div>
              <div className="text-sm text-gray-300">Checkpoint Water Mark</div>
              <div className="text-2xl font-semibold text-white">
                {data?.checkpointWaterMark}
              </div>
            </div>
            <div>
              <div className="text-sm text-gray-300">Maximum TXN Process</div>
              <div className="text-2xl font-semibold text-white">
                {data?.maxProcessTxn}
              </div>
            </div>
          </div>
        ) : (
          <div className="grid grid-cols-3 gap-12 p-4">
            {[...Array(5)].map((_, i) => (
              <div key={i}>
                <Skeleton className="h-4 w-32 mb-2" />
                <Skeleton className="h-8 w-24" />
              </div>
            ))}
          </div>
        )}
      </CardContent>
    </Card>
  );
}
function TransactionHistoryCard() {
  const [chartData, setChartData] = useState([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState(null);

  async function fetchTransactionHistory() {
    try {
      setLoading(true);
      setError(null);
      const response = await middlewareApi.get("/explorer/getEncodedBlocks");
      const data = decodeDeltaEncoding(response?.data);
      setChartData(data);
    } catch (error) {
      setError(error);
      console.error("Failed to fetch transaction history:", error);
    } finally {
      setLoading(false);
    }
  }

  useEffect(() => {
    fetchTransactionHistory();
  }, []);

  const chartConfig = {
    desktop: {
      label: "CreatedAt",
      color: "hsl(var(--chart-1))",
    },
  } satisfies ChartConfig;

  const handleRefresh = () => {
    fetchTransactionHistory();
  };

  return (
    <Card className="h-auto w-full max-w-8xl bg-gradient-to-br from-slate-900 to-slate-950 text-white shadow-xl">
      <CardHeader>
        <div className="flex justify-between">
          <div className="flex items-center gap-2">
            <Hourglass className="w-6 h-6 text-blue-400" />
            <CardTitle className="text-2xl font-bold">
              Resilient Transaction History
            </CardTitle>
          </div>
          <Button
            variant="ghost"
            size="icon"
            onClick={handleRefresh}
            className="text-gray-300 hover:text-white hover:bg-slate-800"
            title="Refresh data"
          >
            <RefreshCw className={`h-5 w-5 ${loading ? "animate-spin" : ""}`} />
            <span className="sr-only">Refresh</span>
          </Button>
        </div>
      </CardHeader>
      <CardContent className="h-full">
        {loading ? (
          <div className="flex flex-col space-y-3 w-full h-[300px] animate-pulse">
            <div className="flex items-center justify-center h-full">
              <div className="space-y-4 w-full">
                <div className="h-4 bg-slate-800 rounded w-3/4"></div>
                <div className="h-[200px] bg-slate-800 rounded"></div>
                <div className="h-4 bg-slate-800 rounded w-1/2"></div>
              </div>
            </div>
          </div>
        ) : error ? (
          <div className="flex flex-col items-center justify-center h-[300px] text-center p-6">
            <AlertCircle className="h-12 w-12 text-red-400 mb-4" />
            <h3 className="text-lg font-semibold mb-2">
              Failed to load transaction data
            </h3>
            <p className="text-gray-400 mb-4">
              {error.message ||
                "An unexpected error occurred while fetching transaction history."}
            </p>
            <Button
              onClick={handleRefresh}
              variant="outline"
              className="border-blue-500 text-blue-400 hover:bg-blue-950"
            >
              <RefreshCw className="mr-2 h-4 w-4" />
              Try Again
            </Button>
          </div>
        ) : chartData.length === 0 ? (
          <div className="flex flex-col items-center justify-center h-[300px] text-center">
            <Database className="h-12 w-12 text-gray-500 mb-4" />
            <h3 className="text-lg font-semibold mb-2">
              No transaction data available
            </h3>
            <p className="text-gray-400 mb-4">
              There are currently no transactions to display.
            </p>
            <Button
              onClick={handleRefresh}
              variant="outline"
              className="border-blue-500 text-blue-400 hover:bg-blue-950"
            >
              <RefreshCw className="mr-2 h-4 w-4" />
              Refresh
            </Button>
          </div>
        ) : (
          <ChartContainer config={chartConfig}>
            <BarChart accessibilityLayer data={chartData} margin={{ top: 10 }}>
              <CartesianGrid vertical={false} />
              <XAxis
                dataKey="createdAt"
                axisLine={false}
                tickFormatter={(value) => value.slice(0, 6)}
              />
              <YAxis
                tick={{ fontSize: 12, fill: "#6b7280" }}
                tickLine={false}
                axisLine={{ stroke: "bg-inherit" }}
              />
              <ChartTooltip
                cursor={false}
                content={<ChartTooltipContent indicator="dashed" />}
              />
              <Bar dataKey="volume" fill="var(--color-desktop)" radius={4} />
            </BarChart>
          </ChartContainer>
        )}
      </CardContent>
    </Card>
  );
}

function BlocksData({ metadata }: { metadata: BlockchainConfig }) {
  return (
    <Card className="w-4/5 max-w-8xl mx-auto bg-gradient-to-br from-slate-900 to-slate-800 text-white shadow-xl">
      <CardHeader className="border-b border-slate-700">
        <div className="flex flex-col sm:flex-row items-center justify-between gap-4">
          <div className="flex items-center gap-2">
            <Cuboid className="w-6 h-6 text-blue-400" />
            <CardTitle className="text-2xl font-bold">Blocks</CardTitle>
          </div>
        </div>
        <CardDescription className="mt-4 text-slate-300">
          View Blocks Data
        </CardDescription>
      </CardHeader>
      <CardContent>
        <BlockchainTable total={metadata?.blockNum || 175} />
      </CardContent>
    </Card>
  );
}
