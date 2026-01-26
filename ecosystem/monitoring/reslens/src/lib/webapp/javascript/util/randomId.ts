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

/**
 * generates a random ID to be used outside react-land (eg jquery)
 * it's mandatory to generate it once, preferrably on a function's body
 * IMPORTANT: it does NOT:
 * - generate unique ids across server/client
 *   use `useId` instead (https://reactjs.org/docs/hooks-reference.html#useid)
 * - guarantee no collisions will happen (although it's unlikely)
 */
export function randomId(prefix?: string) {
  const letters = 'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ';
  const num = 5;

  const id = Array(num)
    .fill(0)
    .map(() => letters.substr(Math.floor(Math.random() * num + 1), 1))
    .join('');

  if (prefix) {
    return `${prefix}-${id}`;
  }

  return id;
}
