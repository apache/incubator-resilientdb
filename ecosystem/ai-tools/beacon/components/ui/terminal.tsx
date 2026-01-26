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

import { forwardRef } from "react"
import { cn } from "@/app/lib/utils"

interface TerminalProps extends React.HTMLAttributes<HTMLDivElement> {
  children: React.ReactNode
}

export const Terminal = forwardRef<HTMLDivElement, TerminalProps>(
  ({ children, className, ...props }, ref) => {
    return (
      <div
        ref={ref}
        className={cn(
          "bg-black border border-slate-700 rounded-b-lg p-4 font-mono text-sm text-green-400 overflow-auto max-h-[400px]",
          className,
        )}
        {...props}
      >
        {children}
      </div>
    )
  }
)

Terminal.displayName = "Terminal"