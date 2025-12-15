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

import { getEncoding, TiktokenEncoding } from "js-tiktoken";

// using this to estimate token count for the code reranker
export interface TokenEstimator {
    estimate(text: string): number;
  }
  
  export class TikTokenEstimator implements TokenEstimator {
    private encoder: any;
  
    constructor() {
      try {
        const encoding = "cl100k_base"; // default encoding for deepseek-chat
        this.encoder = getEncoding(encoding as TiktokenEncoding);
      } catch {
        this.encoder = null;
      }
    }
  
    estimate(text: string): number {
      try {
        const tokens = this.encoder.encode(text);
        return tokens.length;
      } catch {
        return Math.ceil(text.length / 4);
      }
    }
  
    dispose(): void {
      if (this.encoder && typeof this.encoder.free === 'function') {
        try {
          this.encoder.free();
        } catch {
        }
      }
    }
  }
  