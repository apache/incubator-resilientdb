import { useState } from "react";
import { motion } from "framer-motion";
import { Button } from "@/components/ui/button";
import { Tabs, TabsList, TabsTrigger, TabsContent } from "@/components/ui/tabs";
import { Github, Star } from 'lucide-react';
import { FlamegraphCard } from "../graphs/flamegraph";
import { DependencyGraph } from "../graphs/dependency";
import { startCase } from "@/lib/utils";
import { CpuGraph } from "../graphs/cpu";
import { CpuPage } from "../graphs/cpuCard";
import { MemoryTrackerPage } from "../graphs/MemorySpecs/memoryTrackerPage";

const tabVariants = {
  hidden: { opacity: 0, y: 10 },
  visible: { opacity: 1, y: 0, transition: { duration: 0.3 } },
};

export function Dashboard() {
  const [activeTab, setActiveTab] = useState("CPU");

  return (
    <div className="min-h-screen bg-gradient-to-br from-slate-950 to-slate-900 text-slate-50">
      <Tabs
        value={activeTab}
        onValueChange={setActiveTab}
        className="w-full space-y-4"
      >
        <nav className="sticky top-0 z-10 backdrop-blur-md bg-slate-950/80 border-b border-slate-800 px-4 py-3">
          <div className="max-w-8xl mx-auto flex items-center justify-between">
            <div className="flex items-center space-x-8">
              <a href="/">
                <img
                  src="/Memlens.png" // Changed to PNG logo
                  alt="Company Logo"
                  className="h-6 mb-0"
                />
              </a>
              <TabsList className="bg-slate-900/50 backdrop-blur-sm">
                {["bazel_build", "CPU", "Memory Tracker", "targets", "help"].map((tab) => (
                  <TabsTrigger
                    key={tab}
                    value={tab}
                    className="data-[state=active]:bg-blue-600 data-[state=active]:text-white"
                  >
                    {startCase(tab)}
                  </TabsTrigger>
                ))}
              </TabsList>
            </div>
            <div className="flex items-center space-x-4">
              <span className="text-sm text-slate-400">v1.0.0</span>
              <Button
                variant="outline"
                size="sm"
                className="text-slate-400 hover:text-slate-100 hover:bg-slate-800 transition-colors"
              >
                <Star className="h-4 w-4 mr-2" />
                Star
              </Button>
              <Button
                variant="outline"
                size="sm"
                className="text-slate-400 hover:text-slate-100 hover:bg-slate-800 transition-colors"
              >
                <Github className="h-4 w-4 mr-2" />
                GitHub
              </Button>
            </div>
          </div>
        </nav>

        <main className="max-w-8xl mx-auto px-4 py-8 space-y-8">
          <motion.div
            initial="hidden"
            animate="visible"
            variants={tabVariants}
            key={activeTab}
          >
            <TabsContent value="bazel_build">
              <DependencyGraph />
            </TabsContent>
            <TabsContent value="CPU" className="space-y-8">
              <CpuPage />
            </TabsContent>
            <TabsContent value="Memory Tracker" className="space-y-8">
              <MemoryTrackerPage />
            </TabsContent>
          </motion.div>
        </main>
      </Tabs>
    </div>
  );
}