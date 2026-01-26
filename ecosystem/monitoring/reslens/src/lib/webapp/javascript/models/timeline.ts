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

import { z } from 'zod';

// Since the backend may return 'null': https://github.com/pyroscope-io/pyroscope/issues/930
// We create an empty object so that the defaults kick in
export const TimelineSchema = z.preprocess(
  (val: unknown) => {
    return val ?? {};
  },
  z.object({
    startTime: z.number().default(0),
    samples: z.array(z.number()).default([]),
    durationDelta: z.number().default(0),
  })
);

export type Timeline = z.infer<typeof TimelineSchema>;
