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

// TODO: figure out the correct types

export function buildRenderURL(
  state: {
    from: string;
    until: string;
    query: string;
    refreshToken?: string;
    maxNodes?: string | number;
    groupBy?: string;
    groupByValue?: string;
  },
  fromOverride?: string,
  untilOverride?: string
) {
  const params = new URLSearchParams();
  params.set('query', state.query);
  params.set('from', fromOverride || state.from);
  params.set('until', untilOverride || state.until);
  state.refreshToken && params.set('refreshToken', state.refreshToken);
  if (state.maxNodes && state.maxNodes !== '0') {
    params.set('max-nodes', String(state.maxNodes));
  }
  state.groupBy && params.set('groupBy', state.groupBy);
  state.groupByValue && params.set('groupByValue', state.groupByValue);

  return `/render?${params}`;
}

export function buildMergeURLWithQueryID(state: {
  queryID: string;
  refreshToken?: string;
  maxNodes?: string | number;
}) {
  const params = new URLSearchParams();
  params.set('queryID', state.queryID);
  state.refreshToken && params.set('refreshToken', state.refreshToken);
  if (state.maxNodes && state.maxNodes !== '0') {
    params.set('max-nodes', String(state.maxNodes));
  }

  return `/merge?${params}`;
}
