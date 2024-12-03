import { Card, CardContent, CardHeader, CardTitle } from "@/components/ui/card"
import { CacheHitOdometers } from "./cacheHitOdometers"
import { MemoryMetricsGrid } from "./memoryMetricsGrid"
import { Info } from 'lucide-react'
// import { useState } from "react"
// import {
//   Tooltip,
//   TooltipContent,
//   TooltipProvider,
//   TooltipTrigger,
// } from "@/components/ui/tooltip"

export function MemoryTrackerPage() {
  // const [showTooltip1, setShowTooltip1] = useState(false)

  return (
    <div className="space-y-8">
      <div className="relative">
        <div className="absolute top-2 right-2 z-10">
          {/* <TooltipProvider>
            <Tooltip open={showTooltip1}>
              <TooltipTrigger asChild> */}
                <button
                  className="p-2 bg-slate-700 text-slate-400 rounded"
                  onClick={() => window.open("https://google.com", "_blank")}
                >
                  <Info size={24} />
                </button>
          {/* </TooltipTrigger>
              <TooltipContent>
                <p>Click on the i button to understand these metrics.</p>
              </TooltipContent>
            </Tooltip>
          </TooltipProvider> */}
        </div>
        <Card className="bg-slate-900 text-white">
          <CardHeader>
            <CardTitle className="text-2xl font-bold">Cache Hit Gauges</CardTitle>
          </CardHeader>
          <CardContent>
            <CacheHitOdometers />
          </CardContent>
        </Card>
      </div>
      <div>
        <MemoryMetricsGrid />
      </div>
    </div>
  )
}