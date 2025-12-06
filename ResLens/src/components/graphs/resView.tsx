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

import { Cuboid } from "lucide-react";
import {
  Card,
  CardContent,
  CardDescription,
  CardHeader,
  CardTitle,
} from "../ui/card";

export function ResView() {
  return (
    <Card className="w-4/5 max-w-8xl mx-auto bg-gradient-to-br from-slate-900 to-slate-800 text-white shadow-xl">
      <CardHeader className="border-b border-slate-700">
        <div className="flex flex-col sm:flex-row items-center justify-between gap-4">
          <div className="flex items-center gap-2">
            <Cuboid className="w-6 h-6 text-blue-400" />
            <CardTitle className="text-2xl font-bold">Protocol View</CardTitle>
          </div>
        </div>
        <CardDescription className="mt-4 text-slate-300">
          Get Information on each and every step a transaction follows the PBFT
          protocol.
        </CardDescription>
      </CardHeader>
      <CardContent className="p-0 h-screen">
      <div className="w-full h-full overflow-hidden">
        <iframe
          src="https://resview.resilientdb.com/pages/visualizer"
          className="w-full h-full"
          style={{
            border: "none",
            transform: "scale(0.75)", // Scale down the content to fit
            transformOrigin: "top left", // Ensure scaling starts from the top-left
            width: "133%", // Increase width to compensate for scaling
            height: "133%", // Increase height to compensate for scaling
          }}
          title="Visualizer"
        ></iframe>
      </div>
    </CardContent>
    </Card>
  );
}
