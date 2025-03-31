//@ts-nocheck
import { Link, Database, Cuboid, Hourglass } from "lucide-react";
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
              {/* <TransactionHistoryCard loading={loading} data={data} /> */}
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
    <Card className="h-[400px] w-full max-w-8xl mx-auto bg-gradient-to-br from-slate-900 to-slate-950 text-white shadow-xl">
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
function TransactionHistoryCard({ loading, data }: ExplorerCardProps) {
  const chartData = [
    { month: "January", desktop: 186 },
    { month: "February", desktop: 305 },
    { month: "March", desktop: 237 },
    { month: "April", desktop: 73 },
    { month: "May", desktop: 209 },
    { month: "June", desktop: 214 },
  ];
  return (
    <Card className="h-auto w-full max-w-8xl mx-auto bg-gradient-to-br from-slate-900 to-slate-950 text-white shadow-xl">
      <CardHeader>
        <div className="flex justify-between">
          <div className="flex items-center gap-2">
            <Hourglass className="w-6 h-6 text-blue-400" />
            <CardTitle className="text-2xl font-bold">
              {" "}
              Resilient Transaction History
            </CardTitle>
          </div>
        </div>
      </CardHeader>
      <CardContent>
        {!loading ? (
          <NotFound content="" onRefresh={null} />
        ) : (
          <div className="space-y-4">
            <div className="h-[300px] w-full relative">
              <Skeleton className="absolute left-0 top-0 bottom-0 w-2" />
              <div className="absolute left-8 right-0 top-4 bottom-8 flex flex-col justify-between">
                {[...Array(5)].map((_, i) => (
                  <Skeleton key={i} className="h-0.5 w-full" />
                ))}
              </div>
              <Skeleton className="absolute left-12 right-0 bottom-0 h-2" />
            </div>
            <div className="flex justify-center space-x-4">
              {[...Array(3)].map((_, i) => (
                <div key={i} className="flex items-center">
                  <Skeleton className="w-4 h-4 mr-2" />
                  <Skeleton className="h-4 w-20" />
                </div>
              ))}
            </div>
          </div>
        )}
      </CardContent>
    </Card>
  );
}
{
  /* <ResponsiveContainer width="100%" height={350}>
          <LineChart
            data={chartData}
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
              tickFormatter={(value) => `${value}default`}
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
        </ResponsiveContainer> */
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
