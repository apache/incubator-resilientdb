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

import userEvent from '@testing-library/user-event';

export * from '@testing-library/react';
export { render } from './render';
export { userEvent };
export async function loadPyodideInstance() {
    if (!window.loadPyodide) {
      const pyodideScript = document.createElement("script");
      pyodideScript.src = "https://cdn.jsdelivr.net/pyodide/v0.23.4/full/pyodide.js";
      document.head.appendChild(pyodideScript);
      await new Promise((resolve) => (pyodideScript.onload = resolve));
    }
  
    return await window.loadPyodide({ indexURL: "https://cdn.jsdelivr.net/pyodide/v0.23.4/full/" });
  }
  
