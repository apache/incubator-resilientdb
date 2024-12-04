import { StorageEngineMetrics } from "./storageEngineMetrics";
import { MemoryMetricsGrid } from "./memoryMetricsGrid";
import { Info } from "lucide-react";
import {
  Tooltip,
  TooltipContent,
  TooltipProvider,
  TooltipTrigger,
} from "@/components/ui/tooltip";
import { TerminalController } from "./terminal";

export function MemoryTrackerPage() {
  return (
    <div className="space-y-8">
      <div className="relative">
        <div className="absolute top-2 right-2 z-10">
          <TooltipProvider>
            <Tooltip>
              <TooltipTrigger asChild>
                <button
                  className="p-2 bg-slate-700 text-slate-400 hover:text-white hover:bg-slate-600 transition-colors duration-200 ease-in-out rounded"
                  onClick={() => window.open("https://google.com", "_blank")}
                >
                  <Info size={24} />
                </button>
              </TooltipTrigger>
              <TooltipContent>
                <p>Click for more information about these metrics</p>
              </TooltipContent>
            </Tooltip>
          </TooltipProvider>
        </div>
        <div className="space-y-4">
          <TerminalController />
          <StorageEngineMetrics />
          <MemoryMetricsGrid />
        </div>
      </div>
    </div>
  );
}
