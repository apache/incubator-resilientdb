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

import { C } from "../constants";

export default function JsonHighlight({ json }) {
  const highlighted = json.replace(
    /("(\\u[a-zA-Z0-9]{4}|\\[^u]|[^\\"])*"(\s*:)?|\b(true|false|null)\b|-?\d+(?:\.\d*)?(?:[eE][+-]?\d+)?)/g,
    (match) => {
      let color = C.purple;
      if (/^"/.test(match)) color = /:$/.test(match) ? C.accent : C.green;
      return `<span style="color:${color}">${match}</span>`;
    }
  );
  return (
    <pre style={{ fontSize: 11, lineHeight: 1.6, color: C.text2, overflow: "auto", whiteSpace: "pre" }} dangerouslySetInnerHTML={{ __html: highlighted }} />
  );
}
