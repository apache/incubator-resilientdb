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

import { AlertCircle, RefreshCcw } from "lucide-react";
import { Card, CardContent } from "@/components/ui/card";
import { Button } from "@/components/ui/button";

interface CustomNoDataChartProps {
  onRefresh: () => void;
  content?: string;
}

export function NotFound({ onRefresh, content }: CustomNoDataChartProps) {
  return (
    <Card className="w-full h-full flex items-center justify-center bg-background">
      <CardContent className="flex flex-col items-center py-10">
        <div className="relative mb-6">
          <AlertCircle className="h-20 w-20 text-muted-foreground" />
          <RefreshCcw className="h-8 w-8 text-primary absolute -bottom-2 -right-2" />
        </div>
        <h2 className="text-2xl font-semibold mb-2 text-foreground">
          No Chart Data Available
        </h2>
        {content ? (
          <p className="text-muted-foreground text-center mb-6 max-w-sm">
            {content}
          </p>
        ) : (
          <p className="text-muted-foreground text-center mb-6 max-w-sm">
            We couldn't load the chart data at this time. This could be due to a
            connection issue or a temporary glitch.
          </p>
        )}
        <Button onClick={onRefresh} className="flex items-center gap-2">
          <RefreshCcw className="h-4 w-4" />
          Refresh Data
        </Button>
      </CardContent>
    </Card>
  );
}
