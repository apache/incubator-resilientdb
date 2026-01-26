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

import { App, appsModel } from '@webapp/models/app';
import { Result } from '@webapp/util/fp';
import type { ZodError } from 'zod';
import type { RequestError } from './base';
import { parseResponse, request } from './base';

export interface FetchAppsError {
  message?: string;
}

export async function fetchApps(): Promise<
  Result<App[], RequestError | ZodError>
> {
  const response = await request('/api/apps');

  if (response.isOk) {
    return parseResponse(response, appsModel);
  }

  return Result.err<App[], RequestError>(response.error);
}

export async function deleteApp(data: {
  name: string;
}): Promise<Result<boolean, RequestError | ZodError>> {
  const { name } = data;
  const response = await request(`/api/apps`, {
    method: 'DELETE',
    body: JSON.stringify({ name }),
  });

  if (response.isOk) {
    return Result.ok(true);
  }

  return Result.err<boolean, RequestError>(response.error);
}
