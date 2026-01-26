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

import { z, ZodError } from 'zod';
import { Result } from '@webapp/util/fp';
import { modelToResult } from '@webapp/models/utils';

const fooModel = z.array(z.string());

describe('modelToResult', () => {
  it('parses unkown object', () => {
    const got = modelToResult(fooModel, [] as unknown);
    expect(got).toMatchObject(Result.ok([]));
  });

  it('gives an error when object cant be parsed', () => {
    const got = modelToResult(fooModel, null);

    expect(got.isErr).toBe(true);

    // We don't care exactly about the error format, only that it's a ZodError
    expect((got as any).error instanceof ZodError).toBe(true);
  });
});
