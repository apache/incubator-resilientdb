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
import "@pyroscope/flamegraph/dist/index.css";

import { useContext, useEffect } from "react";
import { useState } from "react";
import { FlamegraphRenderer } from "@pyroscope/flamegraph";
import { ProfileData1 } from "@/static/testFlamegraph";
import { Button } from "../ui/button";
import { ViewTypes } from "@pyroscope/flamegraph/dist/packages/pyroscope-flamegraph/src/FlameGraph/FlameGraphComponent/viewTypes";
import { Input } from "../ui/input";
import {
  Select,
  SelectContent,
  SelectItem,
  SelectTrigger,
  SelectValue,
} from "../ui/select";
import { Dialog, DialogContent, DialogHeader } from "@/components/ui/dialog";
import { TechnicalMarkdownRenderer } from "../ui/MarkdownRenderer";
import { FlamegraphRendererProps } from "@pyroscope/flamegraph/dist/packages/pyroscope-flamegraph/src/FlamegraphRenderer";
import { Download, RefreshCcw, Flame, Info, Map, Star, X, Play, BookOpen, Zap, Square } from "lucide-react";
import { Card, CardContent, CardHeader, CardTitle } from "@/components/ui/card";
import { Loader } from "../ui/loader";
import { NotFound } from "../ui/not-found";
import { useMode } from "@/contexts/ModeContext";
import { ModeType } from "../toggle";
import { useToast } from "@/hooks/use-toast";
import { useTour } from "@/hooks/use-tour";
import { Tour } from "../ui/tour";
import {
  DropdownMenu,
  DropdownMenuContent,
  DropdownMenuItem,
  DropdownMenuTrigger,
} from "@/components/ui/dropdown-menu";

const steps = [
  {
    content: <h2>This is a flamegraph to visualize the stack trace</h2>,
    placement: "center",
    target: "body",
  },
  {
    content: (
      <h2>
        Enter a function name for example "SetValue" to see the functions it
        invokes
      </h2>
    ),
    target: "#function-name",
  },
  {
    content: <h2>Click a function to investigate it in a different mode.</h2>,
    placement: "center",
    target: "body",
  },
  {
    content: (
      <h2>
        Choose the "sandwich" method to explore all functions made by it.
        Explore others as well!
      </h2>
    ),
    target: "#graph-type",
  },
  {
    content: <h2>Click on this button to refresh the data</h2>,
    target: "#refresh",
  },
  {
    content: (
      <h2>Click on this icon to get more info and context on this feature</h2>
    ),
    target: "#info-tip",
  },
];

const tourOptions = [
  // {
  //   name: "Explore Advanced Tools",
  //   description: "Learn about the API testing and utility features",
  //   action: () => {
  //     console.log("Advanced tools tour");
  //   }
  // }
];

const developmentTourSteps = [
  {
    content: <h2>Welcome to Development Mode! Let's explore the advanced utilities.</h2>,
    placement: "center",
    target: "body",
  },
  {
    content: <h2>This utility bar contains advanced tools for testing and data generation</h2>,
    target: ".utility-bar",
  },
  {
    content: <h2>Check the health status to ensure the seeding service is available</h2>,
    target: ".health-status",
  },
  {
    content: <h2>Monitor the seeding status to see if operations are running</h2>,
    target: ".seeding-status",
  },
  {
    content: <h2>Select the number of values you want to seed for testing</h2>,
    target: ".count-selector",
  },
  {
    content: <h2>Click Fire SetValues to start seeding data for flamegraph analysis</h2>,
    target: ".fire-setvalues",
  },
  {
    content: <h2>Watch the progress bar to track seeding completion</h2>,
    target: ".progress-bar",
  },
  {
    content: <h2>Use the Stop Seeding button to cancel ongoing operations</h2>,
    target: ".stop-seeding",
  },
  {
    content: <h2>You can abort operations at any time during the process</h2>,
    target: ".abort-button",
  },
  {
    content: <h2>Once seeding completes, the flamegraph will refresh with new data</h2>,
    placement: "center",
    target: "body",
  },
];

interface FlamegraphCardProps {
  from: string | number;
  until: string | number;
}
export const Flamegraph = (props: FlamegraphCardProps) => {
  const { toast } = useToast();
  const { mode, api, refreshTrigger } = useMode();
  const [clientName, setClientName] = useState("cpp_client_1");
  const [profilingData, setProfilingData] = useState(ProfileData1);

  function handleClientNameChange(value: string) {
    setClientName(value);
  }

  /**
   * UI Client State
   */
  const [flamegraphDisplayType, setFlamegraphDisplayType] =
    useState<ViewTypes>("both");
  const [flamegraphInterval, setFlamegraphInterval] =
    useState<string>("now-5m");
  const [_, setSearchQueryToggle] = useState(true); //dummy dispatch not used functionally
  const [refresh, setRefresh] = useState(false);
  const [error, setError] = useState("");
  const [loading, setLoading] = useState(false);
  const [query, setSearchQuery] = useState<
    FlamegraphRendererProps["sharedQuery"]
  >({
    searchQuery: "",
    onQueryChange: (value: any) => {},
    syncEnabled: true,
    toggleSync: setSearchQueryToggle,
  });
  const [explainFGLoading, setExplainFGLoading] = useState(false);
  const [explanationData, setExplanationData] = useState(null);
  const [dialogOpen, setDialogOpen] = useState(false);
  const [utilityBarOpen, setUtilityBarOpen] = useState(false);
  const [setValuesLoading, setSetValuesLoading] = useState(false);
  const [seedingStatus, setSeedingStatus] = useState<string>("stopped");
  const [healthStatus, setHealthStatus] = useState<string>("unknown");
  const [selectedCount, setSelectedCount] = useState<number>(1000);
  const [abortController, setAbortController] = useState<AbortController | null>(null);
  const [progress, setProgress] = useState<number>(0);
  const [resultCount, setResultCount] = useState<number>(0);
  const [pollingInterval, setPollingInterval] = useState<NodeJS.Timeout | null>(null);

  function handleFlamegraphTypeChange(value: string) {
    setFlamegraphDisplayType(value);
  }

  function handleFlamegraphIntervalChange(value: string) {
    setFlamegraphInterval(value);
  }

  async function explainFlamegraph() {
    const from = props.from || flamegraphInterval;
    const until = props.until || "now";

    console.log(`Using ${mode} API endpoint for flamegraph explanation`);

    // Send the serialized data to /explainFlamegraph
    try {
      setExplainFGLoading(true);
      const response = await api.post(
        "/pyroscope/explainFlamegraph",
        {
          query: clientName,
          from: from,
          until: until,
        }
      );
      setExplainFGLoading(false);
      console.log(response?.data);
      setExplanationData(response?.data);
      setDialogOpen(true);

      toast({
        title: "ExplainFlamegraph Response",
        description: `Received response: Success`,
        variant: "default",
      });
    } catch (error) {
      toast({
        title: "ExplainFlamegraph Response",
        description: `Received response: Failure. Unable to recieve response ${error.message}`,
        variant: "destructive",
      });
    } finally {
      setExplainFGLoading(false);
    }
  }

  // eslint-disable-next-line @typescript-eslint/no-unused-vars
  function base64ToBlob(
    base64Data: string,
    contentType: string = "application/octet-stream"
  ): Blob {
    const byteCharacters = atob(base64Data);
    const byteArrays = [];
    for (let offset = 0; offset < byteCharacters.length; offset++) {
      byteArrays.push(byteCharacters.charCodeAt(offset));
    }
    return new Blob([new Uint8Array(byteArrays)], { type: contentType });
  }

  function handleDownload() {
    const jsonData = JSON.stringify(profilingData, null, 2);
    const blob = new Blob([jsonData], { type: "application/json" });
    const url = URL.createObjectURL(blob);
    const link = document.createElement("a");
    link.href = url;
    link.download = "data.json";
    document.body.appendChild(link);
    link.click();
    document.body.removeChild(link);
  }

  function refreshFlamegraph() {
    setRefresh((prev) => !prev);
  }

  async function checkHealth() {
    try {
      const response = await fetch(`${import.meta.env.VITE_DEV_RESLENS_TOOLS_URL}/health`);
      const data = await response.json();
      setHealthStatus(data.status);
      return data.status === 'ok';
    } catch (error) {
      setHealthStatus('error');
      return false;
    }
  }

  async function checkStatus() {
    try {
      const response = await fetch(`${import.meta.env.VITE_DEV_RESLENS_TOOLS_URL}/status`);
      const data = await response.json();
      setSeedingStatus(data.status);
      setResultCount(data.results_count || 0);
      
      // Calculate progress
      if (data.status === 'running' && selectedCount > 0) {
        const currentProgress = Math.min((data.results_count / selectedCount) * 100, 100);
        setProgress(currentProgress);
      } else if (data.status === 'stopped') {
        setProgress(100);
      }
      
      return data;
    } catch (error) {
      setSeedingStatus('error');
      return null;
    }
  }

  function startPolling() {
    const interval = setInterval(async () => {
      const statusData = await checkStatus();
      
      if (statusData?.status === 'stopped') {
        // Seeding is complete
        clearInterval(interval);
        setPollingInterval(null);
        setSetValuesLoading(false);
        setAbortController(null);
        
        toast({
          title: "Seeding Complete",
          description: `Successfully seeded ${resultCount} values. Check the flamegraph for new data.`,
          variant: "default",
        });
        
        // Refresh the flamegraph to show new data
        refreshFlamegraph();
      } else if (statusData?.status === 'error') {
        // Error occurred
        clearInterval(interval);
        setPollingInterval(null);
        setSetValuesLoading(false);
        setAbortController(null);
        
        toast({
          title: "Seeding Failed",
          description: "An error occurred during seeding. Please try again.",
          variant: "destructive",
        });
      }
    }, 1000); // Poll every second
    
    setPollingInterval(interval);
  }

  function stopPolling() {
    if (pollingInterval) {
      clearInterval(pollingInterval);
      setPollingInterval(null);
    }
  }

  async function fireSetValues() {
    try {
      // First check health
      const isHealthy = await checkHealth();
      if (!isHealthy) {
        toast({
          title: "Service Unavailable",
          description: "The seeding service is not healthy. Please try again later.",
          variant: "destructive",
        });
        return;
      }

      // Reset progress
      setProgress(0);
      setResultCount(0);
      setSetValuesLoading(true);
      const controller = new AbortController();
      setAbortController(controller);
      
      // Trigger the seeding job
      const response = await fetch(`${import.meta.env.VITE_DEV_RESLENS_TOOLS_URL}/seed`, {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify({
          count: selectedCount
        }),
        signal: controller.signal
      });
      
      if (!response.ok) {
        throw new Error(`HTTP ${response.status}: ${response.statusText}`);
      }
      
      toast({
        title: "SetValues Seeding Started",
        description: `Started seeding ${selectedCount} values. Monitoring progress...`,
        variant: "default",
      });
      
      // Start polling for status updates
      startPolling();
      
    } catch (error) {
      if (error.name === 'AbortError') {
        toast({
          title: "SetValues Seeding Aborted",
          description: "The seeding operation was cancelled.",
          variant: "default",
        });
      } else {
        toast({
          title: "SetValues Seeding Failed",
          description: `${error.message || "Failed to start seeding"}. Please wait 30 seconds before trying again if needed.`,
          variant: "destructive",
        });
      }
      setSetValuesLoading(false);
      setAbortController(null);
    }
  }

  async function stopSeeding() {
    try {
      const response = await fetch(`${import.meta.env.VITE_DEV_RESLENS_TOOLS_URL}/stop`, {
        method: 'POST'
      });
      
      if (!response.ok) {
        throw new Error(`HTTP ${response.status}: ${response.statusText}`);
      }
      
      toast({
        title: "Seeding Stopped",
        description: "The seeding operation has been stopped.",
        variant: "default",
      });
      
      // Stop polling and update status
      stopPolling();
      setSetValuesLoading(false);
      setAbortController(null);
      await checkStatus();
    } catch (error) {
      toast({
        title: "Stop Failed",
        description: error.message || "Failed to stop seeding",
        variant: "destructive",
      });
    }
  }

  function abortSetValues() {
    if (abortController) {
      abortController.abort();
      setSetValuesLoading(false);
      setAbortController(null);
    }
    stopPolling();
  }

  const { startTour, setSteps } = useTour();

  useEffect(() => {
    setSteps(steps);
  }, []);

  const startDevelopmentTour = () => {
    setSteps(developmentTourSteps);
    startTour();
  };

  // Check initial status and health
  useEffect(() => {
    checkHealth();
    checkStatus();
  }, []);

  // Cleanup polling on unmount
  useEffect(() => {
    return () => {
      stopPolling();
    };
  }, []);

  useEffect(() => {
    console.log(`Flamegraph: Mode changed to ${mode}, refreshTrigger: ${refreshTrigger}`);
    
    const fetchData = async () => {
      try {
        setLoading(true);
        let from: string | number = flamegraphInterval;
        let until: string | number = "now";
        if (props.from && props.until) {
          from = props.from;
          until = props.until;
        }
        
        console.log(`Using ${mode} API endpoint for flamegraph data`);
        
        const response = await api.post("/pyroscope/getProfile", {
          query: clientName,
          from: from,
          until: until,
        });
        if (response?.data?.error) {
          setError(response?.data?.error);
          toast({
            title: "Error",
            description: response?.data?.error,
            variant: "destructive",
          });
          setError(response?.data?.error);
        } else {
          setProfilingData(response?.data);
          toast({
            title: "Data Updated",
            description: `Flamegraph data updated for ${clientName} (${mode})`,
            variant: "default",
          });
        }
        setLoading(false);
      } catch (error) {
        setError(error.message);
        setLoading(false);
        toast({
          title: "Error",
          description: error.message,
          variant: "destructive",
        });
        setError(error?.message);
      }
    };
    fetchData();
  }, [clientName, refresh, props.from, props.until, flamegraphInterval, refreshTrigger]);

  return (
    <Dialog open={dialogOpen} onOpenChange={setDialogOpen}>
      <Card className="w-full max-w-8xl mx-auto bg-gradient-to-br from-slate-900 to-slate-950 text-white shadow-xl">
        <Tour />
        <CardHeader>
          <div className="flex justify-between">
            <div className="flex items-center gap-2">
              <Flame className="w-6 h-6 text-blue-400" />
              <CardTitle className="text-2xl font-bold">Flamegraph</CardTitle>
              {mode === 'development' && (
                <div className="px-2 py-1 bg-yellow-600 text-yellow-100 text-xs font-medium rounded-full flex items-center gap-1">
                  <span className="w-2 h-2 bg-yellow-300 rounded-full animate-pulse"></span>
                  DEBUG MODE
                </div>
              )}
            </div>
            <div className="flex items-center gap-2">
              <Button variant="outline" size="icon" onClick={refreshFlamegraph}>
                <RefreshCcw id="refresh" />
              </Button>
              
              <DropdownMenu>
                <DropdownMenuTrigger asChild>
                  <button className="p-2 bg-slate-700 text-slate-400 hover:text-white hover:bg-slate-600 transition-colors duration-200 ease-in-out rounded flex items-center gap-1">
                    <BookOpen size={16} />
                    <span className="text-sm">Tour</span>
                  </button>
                </DropdownMenuTrigger>
                <DropdownMenuContent align="end" className="w-64 bg-slate-900 border-slate-700">
                  <DropdownMenuItem asChild>
                    <button
                      onClick={startTour}
                      className="flex items-center space-x-2 cursor-pointer w-full text-left px-2 py-1.5 rounded-sm text-slate-300 hover:text-slate-100 hover:bg-slate-800"
                    >
                      <Map size={16} />
                      <div>
                        <div className="font-medium">General Tour</div>
                        <div className="text-xs text-slate-400">Learn the basics of flamegraph</div>
                      </div>
                    </button>
                  </DropdownMenuItem>
                  {mode === 'development' && (
                    <DropdownMenuItem asChild>
                      <button
                        onClick={startDevelopmentTour}
                        className="flex items-center space-x-2 cursor-pointer w-full text-left px-2 py-1.5 rounded-sm text-slate-300 hover:text-slate-100 hover:bg-slate-800"
                      >
                        <Zap size={16} />
                        <div>
                          <div className="font-medium">Development Tour</div>
                          <div className="text-xs text-slate-400">Learn about advanced utilities and SetValues</div>
                        </div>
                      </button>
                    </DropdownMenuItem>
                  )}
                  {mode === 'development' && tourOptions.map((option) => (
                    <DropdownMenuItem key={option.name} asChild>
                      <button
                        onClick={option.action}
                        className="flex items-center space-x-2 cursor-pointer w-full text-left px-2 py-1.5 rounded-sm text-slate-300 hover:text-slate-100 hover:bg-slate-800"
                      >
                        <Play size={16} />
                        <div>
                          <div className="font-medium">{option.name}</div>
                          <div className="text-xs text-slate-400">{option.description}</div>
                        </div>
                      </button>
                    </DropdownMenuItem>
                  ))}
                </DropdownMenuContent>
              </DropdownMenu>
              
              <a
                target="_blank"
                href="https://pyroscope.io/blog/what-is-a-flamegraph/"
                className="p-2 bg-slate-700 text-slate-400 hover:text-white hover:bg-slate-600 transition-colors duration-200 ease-in-out rounded"
              >
                <Info id="info-tip" size={16} />
              </a>
              <Button
                id="analyze-graph"
                variant="outline"
                onClick={explainFlamegraph}
              >
                {explainFGLoading ? <Loader /> : <Star />}
                Explain this profile
              </Button>
              <DialogContent className="max-w-5xl bg-slate-800 border border-slate-700 text-white shadow-xl backdrop-blur-sm">
                <DialogHeader className="flex justify-between items-start pt-2">
                  <Button
                    variant="ghost"
                    size="icon"
                    className="h-8 w-8 rounded-full bg-slate-700 hover:bg-slate-600 absolute top-2 right-2"
                    onClick={() => setDialogOpen(false)}
                  >
                    <X />
                    <span className="sr-only">Close</span>
                  </Button>
                </DialogHeader>
                <div className="overflow-y-auto max-h-[70vh] w-full">
                  {explanationData ? (
                    <TechnicalMarkdownRenderer markdown={explanationData} />
                  ) : (
                    <div className="flex items-center justify-center p-8">
                      <Loader />
                    </div>
                  )}
                </div>
              </DialogContent>
            </div>
          </div>
        </CardHeader>
        <CardContent className="flex flex-col h-full gap-4">
          {error ? (
            <NotFound onRefresh={refreshFlamegraph} />
          ) : (
            <div>
              {/* Utility Bar - Only show in development mode */}
              {mode === "development" && (
                <div className="utility-bar bg-slate-800/50 border border-slate-700 rounded-lg p-3 mb-4">
                  <div className="flex items-center justify-between">
                    <div className="flex items-center space-x-4">
                      <span className="text-sm font-medium text-slate-300">Utility Tools:</span>
                      <div className="flex items-center space-x-2">
                        <button
                          onClick={() => setUtilityBarOpen(!utilityBarOpen)}
                          className="px-3 py-1 text-xs bg-slate-700 hover:bg-slate-600 text-slate-300 rounded transition-colors"
                        >
                          {utilityBarOpen ? "Hide" : "Show"} Advanced Tools
                        </button>
                      </div>
                    </div>
                    <div className="flex items-center space-x-2 text-xs text-slate-400">
                      <span>Mode: {mode}</span>
                      <span>•</span>
                      <span>Client: {clientName}</span>
                    </div>
                  </div>
                  
                  {utilityBarOpen && (
                    <div className="mt-3 pt-3 border-t border-slate-700">
                      <div className="space-y-3">
                        {/* Status Indicators */}
                        <div className="flex items-center space-x-4 text-xs">
                          <div className="health-status flex items-center space-x-2">
                            <span className="text-slate-400">Health:</span>
                            <span className={`px-2 py-1 rounded ${
                              healthStatus === 'ok' ? 'bg-green-600 text-green-100' : 
                              healthStatus === 'error' ? 'bg-red-600 text-red-100' : 
                              'bg-yellow-600 text-yellow-100'
                            }`}>
                              {healthStatus}
                            </span>
                          </div>
                          <div className="seeding-status flex items-center space-x-2">
                            <span className="text-slate-400">Status:</span>
                            <span className={`px-2 py-1 rounded ${
                              seedingStatus === 'running' ? 'bg-green-600 text-green-100' : 
                              seedingStatus === 'stopped' ? 'bg-gray-600 text-gray-100' : 
                              seedingStatus === 'error' ? 'bg-red-600 text-red-100' : 
                              'bg-yellow-600 text-yellow-100'
                            }`}>
                              {seedingStatus}
                            </span>
                          </div>
                        </div>

                        {/* Progress Bar */}
                        {setValuesLoading && seedingStatus === 'running' && (
                          <div className="progress-bar space-y-2">
                            <div className="flex items-center justify-between text-xs">
                              <span className="text-slate-400">Progress:</span>
                              <span className="text-slate-300">{resultCount} / {selectedCount} ({progress.toFixed(1)}%)</span>
                            </div>
                            <div className="w-full bg-slate-700 rounded-full h-2">
                              <div 
                                className="bg-blue-600 h-2 rounded-full transition-all duration-300"
                                style={{ width: `${progress}%` }}
                              ></div>
                            </div>
                          </div>
                        )}

                        {/* SetValues Controls */}
                        <div className="grid grid-cols-1 md:grid-cols-3 gap-3">
                          {/* Count Selection */}
                          <div className="count-selector flex items-center space-x-2">
                            <span className="text-xs text-slate-400">Count:</span>
                            <Select value={selectedCount.toString()} onValueChange={(value) => setSelectedCount(parseInt(value))}>
                              <SelectTrigger className="w-20 h-7 text-xs">
                                <SelectValue />
                              </SelectTrigger>
                              <SelectContent>
                                <SelectItem value="100">100</SelectItem>
                                <SelectItem value="500">500</SelectItem>
                                <SelectItem value="1000">1000</SelectItem>
                                <SelectItem value="5000">5000</SelectItem>
                                <SelectItem value="10000">10000</SelectItem>
                              </SelectContent>
                            </Select>
                          </div>

                          {/* Fire SetValues Button */}
                          <div className="fire-setvalues flex items-center space-x-2">
                            <button
                              onClick={setValuesLoading ? abortSetValues : fireSetValues}
                              disabled={healthStatus !== 'ok' || seedingStatus === 'running'}
                              className={`abort-button px-3 py-2 text-xs rounded transition-colors flex items-center space-x-1 ${
                                setValuesLoading 
                                  ? "bg-red-600 hover:bg-red-700 text-white" 
                                  : "bg-blue-600 hover:bg-blue-700 text-white"
                              } ${(healthStatus !== 'ok' || seedingStatus === 'running') ? "opacity-50 cursor-not-allowed" : ""}`}
                            >
                              {setValuesLoading ? <Square size={12} /> : <Zap size={12} />}
                              <span>{setValuesLoading ? "Abort SetValues" : "Fire SetValues"}</span>
                            </button>
                            <span className="text-xs text-slate-400">Seed data for analysis</span>
                          </div>

                          {/* Stop Button */}
                          <div className="stop-seeding flex items-center space-x-2">
                            <button
                              onClick={stopSeeding}
                              disabled={seedingStatus !== 'running'}
                              className={`px-3 py-2 text-xs rounded transition-colors flex items-center space-x-1 ${
                                seedingStatus === 'running' 
                                  ? "bg-orange-600 hover:bg-orange-700 text-white" 
                                  : "bg-gray-600 text-gray-400"
                              }`}
                            >
                              <Square size={12} />
                              <span>Stop Seeding</span>
                            </button>
                            <span className="text-xs text-slate-400">Cancel current operation</span>
                          </div>
                        </div>

                        {/* Coming Soon - GET Request */}
                        <div className="flex items-center space-x-2 opacity-50">
                          <button
                            disabled
                            className="px-3 py-2 text-xs rounded bg-gray-600 text-gray-400 cursor-not-allowed flex items-center space-x-1"
                          >
                            <Zap size={12} />
                            <span>Fire GET (Coming Soon)</span>
                          </button>
                          <span className="text-xs text-slate-400">Fetch data for analysis - Coming Soon</span>
                        </div>
                      </div>
                    </div>
                  )}
                </div>
              )}
              
              <div className="flex justify-between mb-4">
                <div className="flex flex-row space-x-2">
                  <Input
                    id="function-name"
                    placeholder="Filter by function name"
                    onChange={(e) =>
                      setSearchQuery((prev) => {
                        return {
                          ...prev,
                          searchQuery: e.target.value,
                        };
                      })
                    }
                    className="w-64 bg-slate-800 border-slate-700"
                  />
                </div>
                <div className="flex flex-row space-x-2">
                  <Button
                    id="download-graph"
                    variant="outline"
                    size="icon"
                    onClick={handleDownload}
                  >
                    <Download />
                  </Button>

                  <Select onValueChange={handleClientNameChange} value={clientName}>
                    <SelectTrigger className="w-[120px] h-[30px]">
                      <SelectValue placeholder="App Name" />
                    </SelectTrigger>
                    <SelectContent>
                      <SelectItem value="cpp_client_1">
                        Current Primary (P)
                      </SelectItem>
                      <SelectItem value="cpp_client_2" disabled>
                        Secondary-1
                      </SelectItem>
                      <SelectItem value="system" disabled>
                        Host
                      </SelectItem>
                    </SelectContent>
                  </Select>

                  <Select onValueChange={handleFlamegraphTypeChange}>
                    <SelectTrigger
                      className="w-[120px] h-[30px]"
                      id="graph-type"
                    >
                      <SelectValue placeholder="Graph Type" />
                    </SelectTrigger>
                    <SelectContent>
                      <SelectItem value="table">Table</SelectItem>
                      <SelectItem value="flamegraph">Flamegraph</SelectItem>
                      <SelectItem value="both">Both</SelectItem>
                      <SelectItem value="sandwich">Sandwich</SelectItem>
                    </SelectContent>
                  </Select>

                  <Select onValueChange={handleFlamegraphIntervalChange}>
                    <SelectTrigger className="w-[120px] h-[30px]">
                      <SelectValue placeholder="Interval" />
                    </SelectTrigger>
                    <SelectContent>
                      <SelectItem value="now-5m">Last 5 minutes</SelectItem>
                      <SelectItem value="now-30m">Last 30 minutes</SelectItem>
                      <SelectItem value="now-1h">Last 1 hour</SelectItem>
                      <SelectItem value="now-12h">Last 12 hours</SelectItem>
                      <SelectItem value="now-24h">Last 24 hours</SelectItem>
                    </SelectContent>
                  </Select>
                </div>
              </div>
              <div className="flex-1 bg-slate-800 rounded-lg flex items-center justify-center text-slate-400">
                {loading ? (
                  <Loader className="h-[800px] p-4" />
                ) : (
                  <FlamegraphRenderer
                    id="flamegraph"
                    key={flamegraphDisplayType} // Force a re-render when the type changes
                    profile={profilingData}
                    showCredit={false}
                    panesOrientation="horizontal"
                    onlyDisplay={flamegraphDisplayType}
                    showToolbar={true}
                    colorMode="dark"
                    sharedQuery={query}
                  />
                )}
              </div>
            </div>
          )}
        </CardContent>
      </Card>
    </Dialog>
  );
};
