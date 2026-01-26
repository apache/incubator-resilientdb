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

/* eslint-disable import/prefer-default-export */
import { Result } from '@webapp/util/fp';
import type { ZodError } from 'zod';
import { Profile } from '@pyroscope/models/src';
import {
  FlamegraphDotComResponse,
  flamegraphDotComResponseScheme,
} from '@webapp/models/flamegraphDotComResponse';
import type { RequestError } from './base';
import { request, parseResponse } from './base';

interface shareWithFlamegraphDotcomProps {
  flamebearer: Profile;
  name?: string;
  groupByTag?: string;
  groupByTagValue?: string;
}

export async function shareWithFlamegraphDotcom({
  flamebearer,
  name,
  groupByTag,
  groupByTagValue,
}: shareWithFlamegraphDotcomProps): Promise<
  Result<FlamegraphDotComResponse, RequestError | ZodError>
> {
  const response = await request('/export', {
    method: 'POST',
    body: JSON.stringify({
      name,
      groupByTag,
      groupByTagValue,
      // TODO:
      // use buf.toString
      profile: btoa(JSON.stringify(flamebearer)),
      type: 'application/json',
    }),
  });

  if (response.isOk) {
    return parseResponse(response, flamegraphDotComResponseScheme);
  }

  return Result.err<FlamegraphDotComResponse, RequestError>(response.error);
}
