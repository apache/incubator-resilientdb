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
import { modelToResult } from './utils';

const zDateTime = z.string().transform((value: string | number | Date) => {
  if (typeof value === 'string') {
    const date = Date.parse(value);
    if (Number.isInteger(date)) {
      return date;
    }
    return value;
  }
  if (typeof value === 'number') {
    return new Date(value);
  }
  return value;
});

export const userModel = z.object({
  id: z.number(),
  name: z.string(),
  email: z.optional(z.string()),
  fullName: z.optional(z.string()),
  role: z.string(),
  isDisabled: z.boolean(),
  isExternal: z.optional(z.boolean()),
  createdAt: zDateTime,
  updatedAt: zDateTime,
  passwordChangedAt: zDateTime,
});

export const usersModel = z.array(userModel);

export type Users = z.infer<typeof usersModel>;
export type User = z.infer<typeof userModel>;

export function parse(a: unknown): Result<Users, ZodError> {
  return modelToResult(usersModel, a);
}

export const passwordEncode = (p: string) =>
  btoa(unescape(encodeURIComponent(p)));
