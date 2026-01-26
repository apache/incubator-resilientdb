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

import { Maybe } from '@webapp/util/fp';

// Nominal typing
// https://basarat.gitbook.io/typescript/main-1/nominaltyping
enum QueryBrand {
  _ = '',
}
export type Query = QueryBrand & string;

export function brandQuery(query: string) {
  return query as unknown as Query;
}

export function queryFromAppName(appName: string): Query {
  return `${appName}{}` as unknown as Query;
}

export function queryToAppName(q: Query): Maybe<string> {
  const query: string = q;

  if (!query || !query.length) {
    return Maybe.nothing();
  }

  const rep = query.replace(/\{.*/g, '');

  if (!rep.length) {
    return Maybe.nothing();
  }

  return Maybe.just(rep);
}
