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

import { Query } from '@webapp/models/query';

/**
 * takes a page name and 2 optional queries
 * handling it appropriately when they are not preset
 * and returns only the page title if no query is set
 */
export function formatTitle(
  pageName: string,
  leftQuery?: Query,
  rightQuery?: Query
) {
  const separator = ' | ';

  if (leftQuery && rightQuery) {
    return [pageName, formatTwoQueries(leftQuery, rightQuery)]
      .filter(Boolean)
      .join(separator);
  }

  if (leftQuery) {
    return [pageName, leftQuery].filter(Boolean).join(separator);
  }

  if (rightQuery) {
    return [pageName, rightQuery].filter(Boolean).join(separator);
  }

  // None of them is defined, this may happen when there's no query in the URL
  return pageName;
}

/** formatTwoQueries assumes they both are defined and non empty */
function formatTwoQueries(leftQuery: Query, rightQuery: Query) {
  const separator = ' and ';
  if (leftQuery === rightQuery) {
    return leftQuery;
  }

  return [leftQuery, rightQuery].join(separator);
}
