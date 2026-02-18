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

import { ReactNode } from "react";
import { useMode } from "@/contexts/ModeContext";
import Banner from "@/components/ui/banner";
import { Github, AlertTriangle } from "lucide-react";
import { ToggleState } from "@/components/toggle";
import { Navigation } from "@/components/navigation/Navigation";
import { TourProvider } from "@/hooks/use-tour";
import { Toaster } from "../ui/toaster";
import { Loading } from "@/components/ui/loading";

interface PageLayoutProps {
  children: ReactNode;
}

export function PageLayout({ children }: PageLayoutProps) {
  const { mode, isLoading, setMode, error } = useMode();

  if (isLoading) {
    return <Loading />;
  }

  if (error) {
    return (
      <div className="min-h-screen bg-gradient-to-br from-slate-950 to-slate-900 text-slate-50 flex items-center justify-center">
        <div className="max-w-2xl mx-auto p-6">
          <div className="bg-gradient-to-br from-red-900/20 to-red-800/20 border border-red-700 rounded-lg p-6 shadow-xl">
            <div className="flex items-center gap-3 mb-4">
              <div className="w-10 h-10 bg-red-500/20 border border-red-500/30 rounded-full flex items-center justify-center">
                <AlertTriangle className="h-5 w-5 text-red-400" />
              </div>
              <div>
                <h3 className="text-lg font-semibold text-slate-100">API Configuration Error</h3>
                <p className="text-sm text-slate-400">Invalid instance configuration detected</p>
              </div>
            </div>
            
            <div className="bg-slate-800/50 border border-slate-700 rounded-md p-4">
              <p className="text-slate-300 mb-3">
                {error}
              </p>
              <p className="text-slate-400 text-sm">
                Please use a valid instance parameter in the URL.
              </p>
            </div>
            
            <div className="mt-4">
              <h4 className="text-slate-200 font-medium mb-3">Valid Instance URLs:</h4>
              <div className="space-y-2">
                <div className="flex items-center gap-2">
                  <div className="w-2 h-2 bg-emerald-400 rounded-full"></div>
                  <code className="bg-slate-800 border border-slate-600 text-emerald-300 px-3 py-2 rounded text-sm font-mono">
                    /flamegraph?instance=development
                  </code>
                </div>
                <div className="flex items-center gap-2">
                  <div className="w-2 h-2 bg-emerald-400 rounded-full"></div>
                  <code className="bg-slate-800 border border-slate-600 text-emerald-300 px-3 py-2 rounded text-sm font-mono">
                    /flamegraph?instance=production
                  </code>
                </div>
                <div className="flex items-center gap-2">
                  <div className="w-2 h-2 bg-blue-400 rounded-full"></div>
                  <code className="bg-slate-800 border border-slate-600 text-blue-300 px-3 py-2 rounded text-sm font-mono">
                    /flamegraph
                  </code>
                  <span className="text-slate-500 text-xs">(defaults to production)</span>
                </div>
              </div>
            </div>
          </div>
        </div>
      </div>
    );
  }

  return (
    <TourProvider autoStart={true}>
      <Toaster />
      <div className="min-h-screen bg-gradient-to-br from-slate-950 to-slate-900 text-slate-50">
        <nav className="sticky top-0 z-10 backdrop-blur-md bg-slate-950/80 border-b border-slate-800 px-4 py-3">
          <div className="max-w-8xl mx-auto flex items-center justify-between">
            <div className="flex items-center space-x-8">
              <a href="/">
                <img
                  src="/Reslens.png"
                  alt="Company Logo"
                  className="h-6 mb-0"
                />
              </a>
              <Navigation />
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
                {mode === "prod" && (
                  <span className="text-xs text-slate-400 px-2 py-1 bg-slate-800 rounded">
                    {mode}
                  </span>
                )}
              </div>
            </div>
          </div>
        </nav>
        <div className="max-w-8xl mx-auto px-4 py-8 space-y-8">
          {children}
        </div>
      </div>
    </TourProvider>
  );
} 