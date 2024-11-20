import { useState } from "react";
import { Button } from "@/components/ui/button";
import { Card, CardContent, CardHeader, CardTitle } from "@/components/ui/card";
import { Input } from "@/components/ui/input";
import {
  Select,
  SelectContent,
  SelectItem,
  SelectTrigger,
  SelectValue,
} from "@/components/ui/select";
import { Tabs, TabsList, TabsTrigger } from "@/components/ui/tabs";
import { Github, Star } from "lucide-react";
import { Flamegraph } from "./flamegraph";

export function Dashboard() {
  const [activeTab, setActiveTab] = useState("explorer");

  return (
    <div className="justify-center min-h-screen bg-slate-950 text-slate-50 flex flex-col">
      <nav className="border-b border-slate-800 px-4 py-3">
        <div className="mx-auto flex items-center justify-between">
          <div className="flex items-center space-x-8">
            <a href="/" className="text-xl font-bold">
              MemLens
            </a>
            <Tabs
              value={activeTab}
              onValueChange={setActiveTab}
              className="w-auto"
            >
              <TabsList className="bg-slate-900">
                <TabsTrigger value="explorer">Explorer</TabsTrigger>
                <TabsTrigger value="compare">Compare</TabsTrigger>
                <TabsTrigger value="targets">Targets</TabsTrigger>
                <TabsTrigger value="help">Help</TabsTrigger>
              </TabsList>
            </Tabs>
          </div>
          <div className="flex items-center space-x-4">
            <span className="text-sm text-slate-400">v1.0.0</span>
            <Button
              variant="ghost"
              size="sm"
              className="text-slate-400 hover:text-slate-100"
            >
              <Star className="h-4 w-4 mr-2" />
              Star
            </Button>
            <Button
              variant="ghost"
              size="sm"
              className="text-slate-400 hover:text-slate-100"
            >
              <Github className="h-4 w-4 mr-2" />
              GitHub
            </Button>
          </div>
        </div>
      </nav>

      {/* Main Content */}
      <main className="flex-1 flex flex-col p-4 space-y-4 overflow-hidden">
        {/* First Big Section */}
        <Card className="bg-slate-900 border-slate-800 flex-1 min-h-0">
          <CardHeader>
            <CardTitle>Query Builder</CardTitle>
          </CardHeader>
          <CardContent className="flex flex-col h-full">
            <div className="grid grid-cols-4 gap-4 mb-4">
              <Select>
                <SelectTrigger>
                  <SelectValue placeholder="Profile type" />
                </SelectTrigger>
                <SelectContent>
                  <SelectItem value="cpu">CPU</SelectItem>
                  <SelectItem value="memory">Memory</SelectItem>
                </SelectContent>
              </Select>
              <Select>
                <SelectTrigger>
                  <SelectValue placeholder="Select label name" />
                </SelectTrigger>
                <SelectContent>
                  <SelectItem value="name">Name</SelectItem>
                  <SelectItem value="id">ID</SelectItem>
                </SelectContent>
              </Select>
              <Select>
                <SelectTrigger>
                  <SelectValue placeholder="Select label value" />
                </SelectTrigger>
                <SelectContent>
                  <SelectItem value="value1">Value 1</SelectItem>
                  <SelectItem value="value2">Value 2</SelectItem>
                </SelectContent>
              </Select>
              <Button>Search</Button>
            </div>
            {/* Placeholder for chart or graph */}
            <div className="flex-1 bg-slate-800 rounded-lg flex items-center justify-center text-slate-400">
              Chart or Graph Placeholder
            </div>
          </CardContent>
        </Card>

        {/* Second Big Section */}
        <Card className="bg-slate-900 border-slate-800 flex-1 min-h-0">
          <CardHeader>
            <CardTitle>Flamegraph</CardTitle>
          </CardHeader>
          <CardContent className="flex flex-col h-full">
            <Flamegraph />
          </CardContent>
        </Card>
      </main>
    </div>
  );
}
