"use client";

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
*/

import { Button } from "@/components/ui/button";
import { LavaLamp } from "@/components/ui/fluid-blob";
import { useRouter } from "next/navigation";

export default function WelcomePage() {
  const router = useRouter();
  return (
    <div className="h-screen w-screen flex flex-col justify-center items-center relative">
        <LavaLamp />
        <h1 className="text-6xl md:text-8xl font-bold tracking-tight mix-blend-exclusion text-white whitespace-nowrap">
        Welcome to Nexus
        </h1>
        <p className="text-lg md:text-xl text-center text-white mix-blend-exclusion max-w-2xl leading-relaxed">
          Explore ResilientDB’s research, consensus protocols, and innovations. Instantly turn new ideas into working code.
        </p>
        <div className="flex flex-wrap justify-center gap-4 mt-8 z-10">
          <Button
            onClick={() => {
              router.push("/research");
            }}
          >
            Start Chatting
          </Button>
          <Button
            variant="secondary"
            onClick={() => {
              window.open("https://resilientdb.apache.org/", "_blank");
            }}
          >
            Official Website
          </Button>
        </div>
    </div>
  );
}
