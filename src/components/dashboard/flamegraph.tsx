import "@pyroscope/flamegraph/dist/index.css";

import React, { useEffect } from "react";
import { useState } from "react";
import { FlamegraphRenderer, Box } from "@pyroscope/flamegraph";
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
import { pyroscopeApi } from "@/lib/api";

export const Flamegraph = () => {
  const [flamegraphDisplayType, setFlamegraphDisplayType] =
    useState<ViewTypes>("both");
  const [searchQueryToggle, setSearchQueryToggle] = useState(true); //dummy dispatch not used functionally
  const [query, setSearchQuery] = useState<
    FlamegraphRendererProps["sharedQuery"]
  >({
    searchQuery: "",
    onQueryChange: (value: any) => {},
    syncEnabled: true,
    toggleSync: setSearchQueryToggle,
  });
  function handleFlamegraphTypeChange(value: any) {
    setFlamegraphDisplayType(value);
  }

  function reserFlamegraph() {
    setFlamegraphDisplayType("both");
  }

  useEffect(() => {
    const fetchData = async () => {
      try {
        const data = await pyroscopeApi.get(
          "/render?query=cpp_client_1.cpu%7B%7D&from=now-5m&until=now&format=json",
          {
            withCredentials: false,
            headers: {
              "Content-Type": "application/json",
            },
          }
        );
        console.log(data);
      } catch (error) {
        console.log(error);
      }
    };
    fetchData();
  }, []);

  return (
    <>
      <div className="flex justify-between mb-4">
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
        <div className="flex flex-row space-x-2">
          <Button variant="outline" size="sm" disabled>
            Download
          </Button>
          <Button variant="outline" size="sm" disabled>
            Refresh Graph
          </Button>
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
          profile={ProfileData1}
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
