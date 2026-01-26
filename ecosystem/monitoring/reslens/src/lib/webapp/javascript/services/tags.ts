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

import {
  Tags,
  TagsValuesSchema,
  TagsValues,
  TagsSchema,
} from '@webapp/models/tags';
import { request, parseResponse } from './base';

export async function fetchTags(query: string, from: number, until: number) {
  const params = new URLSearchParams({
    query,
    from: from.toString(10),
    until: until.toString(10),
  });
  const response = await request(`/labels?${params.toString()}`);
  return parseResponse<Tags>(response, TagsSchema);
}

export async function fetchLabelValues(
  label: string,
  query: string,
  from: number,
  until: number
) {
  const params = new URLSearchParams({
    query,
    label,
    from: from.toString(10),
    until: until.toString(10),
  });
  const response = await request(`/label-values?${params.toString()}`);
  return parseResponse<TagsValues>(response, TagsValuesSchema);
}
