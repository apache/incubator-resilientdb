import { useState, useEffect } from "react";
import { motion } from "framer-motion";
import { Button } from "@/components/ui/button";
import { Tabs, TabsList, TabsTrigger, TabsContent } from "@/components/ui/tabs";
import { Github, Star } from "lucide-react";
import { DependencyGraph } from "../graphs/dependency";
import { startCase } from "@/lib/utils";
import { CpuPage } from "../graphs/cpuCard";
import { MemoryTrackerPage } from "../graphs/MemorySpecs/memoryTrackerPage";
import { ToggleState, ModeType } from "../toggle";
import { ModeContext } from "@/hooks/context";
import { middlewareApi } from "@/lib/api";
import { useToast } from "@/hooks/use-toast";

const tabVariants = {
  hidden: { opacity: 0, y: 10 },
  visible: { opacity: 1, y: 0, transition: { duration: 0.3 } },
};

export function Dashboard() {
  const [activeTab, setActiveTab] = useState("memory_tracker");
  const [mode, setMode] = useState<ModeType>("live");
  const { toast } = useToast();

  async function getMode() {
    try {
      const response = await middlewareApi.get("/healthcheck");
      if (response?.status === 200) {
        setMode("live");
      } else {
        setMode("offline");
      }
    } catch (error) {
      setMode("offline");
      toast({
        title: "Error",
        description: "Failed to fetch mode. Assuming offline mode.",
        variant: "destructive",
      });
    }
  }

  useEffect(() => {
    getMode();
  }, []);

  return (
    <ModeContext.Provider value={mode}>
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
                  {["memory_tracker", "cpu", "bazel_build"].map((tab) => (
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
                <a
                href="https://github.com/Bismanpal-Singh/MemLens"
                target="_blank"
                rel="noopener noreferrer"
                className="inline-flex items-center px-4 py-2 text-sm font-medium text-slate-400 border border-slate-400 rounded hover:text-slate-100 hover:bg-slate-800 transition-colors"
                >
                <Github className="h-4 w-4 mr-2" />
                GitHub - MemLens
              </a>
                <a
                href="https://github.com/harish876/MemLens-middleware"
                target="_blank"
                rel="noopener noreferrer"
                className="inline-flex items-center px-4 py-2 text-sm font-medium text-slate-400 border border-slate-400 rounded hover:text-slate-100 hover:bg-slate-800 transition-colors"
              >
                <Github className="h-4 w-4 mr-2" />
                Github - MiddleWare
              </a>
                <div className="flex items-center space-x-2">
                  <ToggleState state={mode} setState={setMode} />
                </div>
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
              <TabsContent value="cpu" className="space-y-8">
                <CpuPage />
              </TabsContent>
              <TabsContent value="memory_tracker" className="space-y-8">
                <MemoryTrackerPage />
              </TabsContent>
            </motion.div>
          </main>
        </Tabs>
      </div>
    </ModeContext.Provider>
  );
}
