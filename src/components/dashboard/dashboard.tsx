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
import { useState, useEffect } from "react";
import { motion } from "framer-motion";
import { Tabs, TabsList, TabsTrigger, TabsContent } from "@/components/ui/tabs";
import { Github } from "lucide-react";
import { DependencyGraph } from "../graphs/dependency";
import { startCase } from "@/lib/utils";
import { CpuPage } from "../graphs/cpuCard";
import { MemoryTrackerPage } from "../graphs/MemorySpecs/memoryTrackerPage";
import { ToggleState, ModeType } from "../toggle";
import { ModeContext } from "@/hooks/context";
import { middlewareApi } from "@/lib/api";
import Banner from "../ui/banner";
import { TourProvider, useTour } from "@/hooks/use-tour";
import { Tour } from "../ui/tour";
import { Explorer } from "../graphs/explorer";
import { ResView } from "../graphs/resView";

const tabVariants = {
  hidden: { opacity: 0, y: 10 },
  visible: { opacity: 1, y: 0, transition: { duration: 0.3 } },
};

const steps = [
  {
    content: <h2>Welcome to ResLens. Let's begin our journey!</h2>,
    placement: "center",
    target: "body",
  },
  {
    content: (
      <h2>Click on this icon to take a guided tour of this component</h2>
    ),
    target: "#tour-tip",
  },
  {
    content: (
      <h2>Click on this icon to get more info and context on this feature</h2>
    ),
    target: "#info-tip",
  },
];

const tabMapping = [
  {
    id: "cpu",
    tag: "Execution View",
  },
  {
    id: "explorer",
    tag: "Data View",
  },
  {
    id: "resview",
    tag: "Protocol View",
  },
  {
    id: "memory_tracker",
    tag: "Memory View",
  },
];

export function Dashboard() {
  return (
    <TourProvider autoStart={true}>
      <DashboardComponent />
    </TourProvider>
  );
}

function DashboardComponent() {
  const { setSteps } = useTour();
  const [activeTab, setActiveTab] = useState("cpu");
  const [mode, setMode] = useState<ModeType>("offline");

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
    }
  }

  useEffect(() => {
    setSteps(steps);
  }, [setSteps]);

  useEffect(() => {
    getMode();
  }, []);

  useEffect(() => {
    if (mode === "offline") {
      setActiveTab("cpu");
    }
  }, [mode]);

  return (
    <ModeContext.Provider value={mode}>
      {mode === "offline" && <Banner />}
      <Tour />
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
                    src="/Reslens logo.png"
                    alt="Company Logo"
                    className="h-6 mb-0"
                  />
                </a>
                <TabsList className="bg-slate-900/50 backdrop-blur-sm">
                  {tabMapping.map((tab) => (
                    <TabsTrigger
                      key={tab.id}
                      value={tab.id}
                      className="data-[state=active]:bg-blue-600 data-[state=active]:text-white"
                    >
                      {tab.tag}
                    </TabsTrigger>
                  ))}
                </TabsList>
              </div>
              <div className="flex items-center space-x-4">
                <span id="version" className="text-sm text-slate-400">
                  v1.0.0
                </span>
                <a
                  href="https://github.com/harish876/MemLens"
                  target="_blank"
                  rel="noopener noreferrer"
                  className="inline-flex items-center px-4 py-2 text-sm font-medium text-slate-400 border border-slate-400 rounded hover:text-slate-100 hover:bg-slate-800 transition-colors"
                >
                  <Github className="h-4 w-4 mr-2" />
                  GitHub - ResLens
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
              <TabsContent value="explorer" className="space-y-8">
                <Explorer />
              </TabsContent>
              <TabsContent value="resview" className="space-y-8">
                <ResView />
              </TabsContent>
            </motion.div>
          </main>
        </Tabs>
      </div>
    </ModeContext.Provider>
  );
}
