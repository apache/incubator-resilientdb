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
