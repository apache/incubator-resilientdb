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

import { BarChart3, Cpu, HardDrive, Database, Network, Flame, FileText } from "lucide-react";
import { cn } from "@/lib/utils";

interface DashboardNavigationProps {
  activeTab: string;
  onTabChange: (value: string) => void;
}

const tabMapping = [
  {
    id: "cpu",
    tag: "Execution View",
    icon: Cpu,
    description: "CPU and execution metrics"
  },
  {
    id: "explorer",
    tag: "Data View",
    icon: Database,
    description: "Data exploration and analysis"
  },
  {
    id: "resview",
    tag: "Protocol View",
    icon: Network,
    description: "Protocol and network metrics"
  },
  {
    id: "memory_tracker",
    tag: "Memory View",
    icon: HardDrive,
    description: "Memory usage and metrics"
  },
  {
    id: "query_stats",
    tag: "Query Stats",
    icon: FileText,
    description: "GraphQL query statistics from Nexus"
  }
];

const dropdownItems = [
  {
    name: "Dashboard",
    href: "/dashboard",
    icon: BarChart3,
    description: "Overview with all views"
  },
  {
    name: "Execution View",
    href: "/cpu",
    icon: Cpu,
    description: "CPU and execution metrics"
  },
  {
    name: "Memory View",
    href: "/memory",
    icon: HardDrive,
    description: "Memory usage and metrics"
  },
  {
    name: "Data View",
    href: "/explorer",
    icon: Database,
    description: "Data exploration and analysis"
  },
  {
    name: "Protocol View",
    href: "/resview",
    icon: Network,
    description: "Protocol and network metrics"
  },
  {
    name: "Flamegraph",
    href: "/flamegraph",
    icon: Flame,
    description: "Stack trace visualization"
  },
  {
    name: "Query Statistics",
    href: "/query-stats",
    icon: FileText,
    description: "GraphQL query statistics from Nexus"
  }
];

export function DashboardNavigation({ activeTab, onTabChange }: DashboardNavigationProps) {
  return (
    <nav className="flex space-x-1">
      {tabMapping.map((tab) => {
        const isActive = activeTab === tab.id;
        return (
          <button
            key={tab.id}
            onClick={(e) => {
              e.stopPropagation();
              onTabChange(tab.id);
            }}
            className={cn(
              "px-3 py-2 text-sm font-medium rounded-md transition-colors",
              isActive
                ? "bg-blue-600 text-white"
                : "text-slate-400 hover:text-slate-100 hover:bg-slate-800"
            )}
            title={tab.description}
          >
            {tab.tag}
          </button>
        );
      })}
    </nav>
  );
} 