import { StorageEngineMetrics } from "./storageEngineMetrics";
import { MemoryMetricsGrid } from "./memoryMetricsGrid";
import { Info, Map } from "lucide-react";
import { TerminalController } from "./terminal";
import { useContext } from "react";
import { ModeContext } from "@/hooks/context";

export function MemoryTrackerPage() {
  const mode = useContext(ModeContext);
  return (
    <div className="space-y-8">
      <div className="relative">
        <div className="absolute top-2 right-2 z-10">
          <div className="flex flex-row space-x-2 m-4">
            <button
              disabled
              className="p-2 bg-slate-700 text-slate-400 hover:text-white hover:bg-slate-600 transition-colors duration-200 ease-in-out rounded"
            >
              <Map id="tour-tip" size={16} />
            </button>
            <button
              disabled
              className="p-2 bg-slate-700 text-slate-400 hover:text-white hover:bg-slate-600 transition-colors duration-200 ease-in-out rounded"
            >
              <Info id="info-tip" size={16} />
            </button>
          </div>
        </div>
        <div className="space-y-4">
          <TerminalController />
          {!(mode === "offline") && <StorageEngineMetrics />}
          {/* {import.meta.env.VITE_DISK_METRICS_FLAG === "true" && (
            <MemoryMetricsGrid />
          )} */}
          <MemoryMetricsGrid />
        </div>
      </div>
    </div>
  );
}
