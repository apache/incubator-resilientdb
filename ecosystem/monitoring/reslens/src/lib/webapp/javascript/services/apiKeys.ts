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

import { Result } from '@webapp/util/fp';
import type { ZodError } from 'zod';
import {
  APIKeys,
  apikeyModel,
  apiKeysSchema,
  APIKey,
} from '@webapp/models/apikeys';
import { request, parseResponse } from './base';
import type { RequestError } from './base';

export interface FetchAPIKeysError {
  message?: string;
}

export async function fetchAPIKeys(): Promise<
  Result<APIKeys, RequestError | ZodError>
> {
  const response = await request('/api/keys');
  return parseResponse<APIKeys>(response, apiKeysSchema);
}

export async function createAPIKey(data: {
  name: string;
  role: string;
  ttlSeconds: number;
}): Promise<Result<APIKey, RequestError | ZodError>> {
  const response = await request('/api/keys', {
    method: 'POST',
    body: JSON.stringify(data),
  });

  return parseResponse(response, apikeyModel);
}

export async function deleteAPIKey(data: {
  id: number;
}): Promise<Result<unknown, RequestError | ZodError>> {
  const response = await request(`/api/keys/${data.id}`, {
    method: 'DELETE',
  });

  if (response.isOk) {
    return Result.ok();
  }

  return Result.err<APIKeys, RequestError>(response.error);
}
