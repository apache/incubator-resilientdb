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

import React from "react";
import { Loader2 } from "lucide-react";
import { cn } from "@/lib/utils";

interface LoaderProps extends React.HTMLAttributes<HTMLDivElement> {
  size?: "sm" | "md" | "lg";
  text?: string;
  loading?: boolean;
}

export const Loader: React.FC<LoaderProps> = ({
  size = "md",
  text,
  className,
  ...props
}) => {
  const sizeClasses = {
    sm: "w-4 h-4",
    md: "w-6 h-6",
    lg: "w-8 h-8",
  };

  return (
    <div
      className={cn("flex flex-col items-center justify-center", className)}
      {...props}
    >
      <Loader2 className={cn("animate-spin text-primary", sizeClasses[size])} />
      {text && <p className="mt-2 text-sm text-muted-foreground">{text}</p>}
    </div>
  );
};
