//@ts-nocheck
import React, { useEffect, useState } from "react";
import { select } from "d3-selection";
import * as d3 from "d3";
import "d3-graphviz";
import {
  Card,
  CardContent,
  CardDescription,
  CardHeader,
  CardTitle,
} from "@/components/ui/card";
import { dots2, dots3, dots4 } from "@/static/dependencyGraphData";

import {
  Select,
  SelectContent,
  SelectItem,
  SelectTrigger,
  SelectValue,
} from "@/components/ui/select";
import { GitGraph } from "lucide-react";
import { CopyableCommand } from "../ui/copyToClipboard";

export const DependencyGraph = () => {
  const [isRendering, setIsRendering] = useState(true); // State to control rendering
  const [depthSize, setDepthSize] = useState(2);
  const [dots, setDots] = useState(dots2);

  useEffect(() => {
    // Set up graphviz element using d3-graphviz
    const graphvizElement = select("#graph")
      .graphviz()
      .logEvents(true)
      // .transition(() =>
      //   d3.transition("main").ease(d3.easeCubicIn).duration(700)
      // ) // Smooth transition
      // .tweenShapes(false)
      .on("initEnd", render);

    // Function to update the graph size and attributes
    function attributer(datum, index, nodes) {
      const selection = select(this);
      if (datum.tag === "svg") {
        const width = window.innerWidth;
        const height = window.innerHeight;
        const x = 200;
        const y = 10;
        const scale = 0.75;
        selection
          .attr("width", `${width}pt`)
          .attr("height", `${height}pt`)
          .attr("viewBox", `${-x} ${-y} ${width / scale} ${height / scale}`);
        datum.attributes.width = `${width}pt`;
        datum.attributes.height = `${height}pt`;
        datum.attributes.viewBox = `${-x} ${-y} ${width / scale} ${
          height / scale
        }`;
      }
    }

    let dotIndex = 0;

    // Function to render the graph incrementally
    function render() {
      if (!isRendering) return; // Exit if rendering is stopped

      const dotLines = dots[dotIndex % dots.length];
      const dot = dotLines.join("");
      graphvizElement.renderDot(dot).on("end", function () {
        dotIndex += 1;
        if (dotIndex < dots.length) {
          render(); // Only recurse if there are more dots to render
        } else {
          setIsRendering(false); // Stop rendering after all steps are done
        }
      });
    }

    render();

    return () => {
      setIsRendering(false);
    };
  }, [isRendering]);

  function handleDepSizeChange(value: string) {
    console.log(value);
    switch (parseInt(value)) {
      case 2:
        setDots(dots2);
        break;
      case 3:
        setDots(dots3);
        break;
      case 4:
        setDots(dots4);
        break;
      default:
        setDots(dots2);
        break;
    }
    setDepthSize(parseInt(value));
    setIsRendering((prev) => !prev);
  }

  return (
    <Card className="w-full max-w-8xl mx-auto bg-gradient-to-br from-slate-900 to-slate-800 text-white shadow-xl">
      <CardHeader className="border-b border-slate-700">
        <div className="flex flex-col sm:flex-row items-center justify-between gap-4">
          <div className="flex items-center gap-2">
            <GitGraph className="w-6 h-6 text-blue-400" />
            <CardTitle className="text-2xl font-bold">
              Dependency Graph
            </CardTitle>
          </div>
          <Select onValueChange={handleDepSizeChange}>
            <SelectTrigger
              className=" w-[160px] bg-slate-800 border-slate-700 text-white"
              aria-label="Select dependency depth"
            >
              <SelectValue placeholder="Depth" />
            </SelectTrigger>
            <SelectContent className="bg-slate-800 border-slate-700 text-white">
              <SelectItem value="2">2 Levels</SelectItem>
              <SelectItem value="3">3 Levels</SelectItem>
              <SelectItem value="4" disabled>
                4 Levels
              </SelectItem>
            </SelectContent>
          </Select>
        </div>
        <CardDescription className="mt-4 text-slate-300">
          Visualize ResDB's KV Service dependencies
        </CardDescription>
      </CardHeader>
      <CardContent className="p-6">
        <div className="mb-6">
          <h3 className="text-lg font-semibold mb-2 text-blue-300">Command</h3>
          <CopyableCommand
            command={`bazel query --notool_deps --noimplicit_deps "deps(//service/kv:kv_service, ${depthSize})"`}
          />
        </div>
        <div
          id="graph"
          className="w-full p-4 h-[600px] bg-slate-800 rounded-lg shadow-inner flex justify-center items-center border border-slate-700"
        ></div>
      </CardContent>
    </Card>
  );
};

export default DependencyGraph;
