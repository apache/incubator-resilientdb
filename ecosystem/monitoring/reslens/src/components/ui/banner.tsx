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

import { useState } from "react";
import { Info, X } from "lucide-react";
import { Alert } from "./alert";

export default function Banner() {
  const [isVisible, setIsVisible] = useState(true);

  if (!isVisible) return null;

  return (
    <Alert className="bg-blue-600  text-white px-4 py-3 flex items-center justify-between">
      <div className="flex items-center">
        <Info className="h-5 w-5 mr-2" />
        <span>
          <strong>You're exploring offline mode!</strong>
          <br />
          Enjoy the static data for now. Stay tuned—live data and interactive
          features are coming your way soon!✨
        </span>
      </div>
      <button
        onClick={() => setIsVisible(false)}
        className="text-white hover:text-blue-100 transition-colors"
        aria-label="Close banner"
      >
        <X className="h-5 w-5" />
      </button>
    </Alert>
  );
}
