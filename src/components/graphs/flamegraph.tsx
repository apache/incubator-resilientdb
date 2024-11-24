import "@pyroscope/flamegraph/dist/index.css";

import React, { useEffect } from "react";
import { useState } from "react";
import { FlamegraphRenderer } from "@pyroscope/flamegraph";
import { ProfileData1 } from "@/mock/test_flamegraph";
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
import { FlamegraphRendererProps } from "@pyroscope/flamegraph/dist/packages/pyroscope-flamegraph/src/FlamegraphRenderer";
import { middlewareApi } from "@/lib/api";
import { Download, RefreshCcw } from "lucide-react";
import { Card, CardContent, CardHeader, CardTitle } from "@/components/ui/card";

const Flamegraph = () => {
  /**
   * UI Client Data
   */

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
  const [_, setSearchQueryToggle] = useState(true); //dummy dispatch not used functionally
  const [refresh, setRefresh] = useState(false);
  const [query, setSearchQuery] = useState<
    FlamegraphRendererProps["sharedQuery"]
  >({
    searchQuery: "",
    onQueryChange: (value: any) => {},
    syncEnabled: true,
    toggleSync: setSearchQueryToggle,
  });

  function handleFlamegraphTypeChange(value: string) {
    setFlamegraphDisplayType(value);
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

  useEffect(() => {
    const fetchData = async () => {
      try {
        const response = await middlewareApi.post("/pyroscope/getProfile", {
          query: clientName,
        });
        setProfilingData(response?.data);
        // setProfilingData(ProfileData1);
        console.log(response?.data);
      } catch (error) {
        console.log(error);
        setProfilingData(ProfileData1);
      }
    };
    fetchData();
  }, [clientName, refresh]);

  return (
    <>
      <div className="flex justify-between mb-4">
        <div className="flex flex-row space-x-2">
          <Button variant="outline" size="icon" onClick={refreshFlamegraph}>
            <RefreshCcw />
          </Button>
          <Input
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
          <Button variant="outline" size="icon" onClick={handleDownload}>
            <Download />
          </Button>

          <Select onValueChange={handleClientNameChange}>
            <SelectTrigger className="w-[120px] h-[30px]">
              <SelectValue placeholder="App Name" />
            </SelectTrigger>
            <SelectContent>
              <SelectItem value="cpp_client_1">Current Primary (P)</SelectItem>{" "}
              {/* TODO: remove static coding */}
              <SelectItem value="cpp_client_2">Secondary-1</SelectItem>{" "}
              {/* TODO: remove static coding */}
              <SelectItem value="system">Host</SelectItem>
            </SelectContent>
          </Select>
          <Select onValueChange={handleFlamegraphTypeChange}>
            <SelectTrigger className="w-[120px] h-[30px]">
              <SelectValue placeholder="Graph Type" />
            </SelectTrigger>
            <SelectContent>
              <SelectItem value="table">Table</SelectItem>
              <SelectItem value="flamegraph">Flamegraph</SelectItem>
              <SelectItem value="both">Both</SelectItem>
              <SelectItem value="sandwich">Sandwich</SelectItem>
            </SelectContent>
          </Select>
        </div>
      </div>
      <div className="flex-1 bg-slate-800 rounded-lg flex items-center justify-center text-slate-400">
        <FlamegraphRenderer
          key={flamegraphDisplayType} // Force a re-render when the type changes
          profile={profilingData}
          showCredit={false}
          panesOrientation="horizontal"
          onlyDisplay={flamegraphDisplayType}
          showToolbar={true}
          colorMode="dark"
          sharedQuery={query}
        />
      </div>
    </>
  );
};

export const FlamegraphCard = () => {
  return (
    <Card className="bg-slate-900 border-slate-800 flex-1 min-h-0">
      <CardHeader>
        <CardTitle>Flamegraph</CardTitle>
      </CardHeader>
      <CardContent className="flex flex-col h-full">
        <Flamegraph />
      </CardContent>
    </Card>
  );
};
